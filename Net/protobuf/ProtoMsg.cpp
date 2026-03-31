#include "ProtoMsg.h"
#include <google/protobuf/descriptor.h>
#include <string>
#include "../../Common/LogFilter.h"
#include "ProtoBufBuffer.h"

typedef  google::protobuf::DescriptorPool DescriptorPool;
typedef google::protobuf::MessageFactory MessageFactory;

namespace yy
{
namespace net 
{
void ProtoMsgCodec::encode(Message* msg,Buffer& buf) 
{
    size_t len_pos=buf.readable_size();
    
    // 2. 先占位长度字段（4字节）和名字长度字段（4字节）
    buf.FluentAppend(htonl(0)).  // 总长度占位
        append(htonl(0));  // 名字长度占位
    
    // 3. 获取类型名
    const std::string& typeName=msg->GetDescriptor()->full_name();
    
    // 4. 写入类型名
    buf.append(typeName.data(),typeName.size());
    
    // 5. 使用 ZeroCopyOutputStream 序列化消息体
    ProtoBufOutputStream output_stream(&buf);
    if (!msg->SerializeToZeroCopyStream(&output_stream)) 
    {
        LOG_WARN("Failed to serialize message");
        return;
    }
    
    // 6. 计算各部分长度
    size_t total_len=buf.readable_size()-len_pos;
    size_t name_len=typeName.size();
    
    // 7. 回头修改长度字段
    char* p=buf.ModifyData()+len_pos;
    *(reinterpret_cast<uint32_t*>(p))=htonl(static_cast<uint32_t>(total_len));
    *(reinterpret_cast<uint32_t*>(p+4))=htonl(static_cast<uint32_t>(name_len));
}
Message* ProtoMsgCodec::createMessage(const std::string& typeName) 
{
    const Descriptor* des=DescriptorPool::generated_pool()
        ->FindMessageTypeByName(typeName);
    if(!des) return NULL;
    
    const Message* proto=MessageFactory::generated_factory()
        ->GetPrototype(des);
    if(!proto) return NULL;
    
    return proto->New();
}    
// 4 byte total msg len, including this 4 bytes
// 4 byte name len
// name string not null ended
// protobuf data
Message* ProtoMsgCodec::decode(Buffer& buf) 
{
    if(buf.readable_size()<8) 
    {
        return NULL;
    }
    char* p = buf.ModifyData();
    
    // 1. 读取长度字段（注意网络字节序转换）
    uint32_t total_len=ntohl(*reinterpret_cast<uint32_t*>(p));
    uint32_t name_len=ntohl(*reinterpret_cast<uint32_t*>(p+4));
    
    // 2. 验证长度
    if (total_len>MAX_MESSAGE_SIZE||name_len>MAX_NAME_LENGTH) 
    {
        LOG_WARN("Invalid length: total="<<total_len<<" name="<<name_len);
        return NULL;
    }
    
    if (buf.readable_size()<total_len) 
    {
        return NULL;  // 数据不足
    }
    
    // 3. 读取类型名
    std::string typeName(p+8,name_len);
    
    // 4. 创建消息对象
    Message* msg=createMessage(typeName);
    if(!msg) 
    {
        LOG_WARN("Cannot create message for type: "<<typeName.c_str());
        return NULL;
    }
    
    // 5. 使用 ZeroCopyInputStream 解析消息体
    ProtoBufInputStream input_stream(&buf);
    
    // 跳过头部（长度字段和类型名）
    if (!input_stream.Skip(8+name_len)) 
    {
        delete msg;
        return NULL;
    }
    
    // 解析消息体
    if (!msg->ParseFromZeroCopyStream(&input_stream)) 
    {
        LOG_WARN("Failed to parse protobuf message");
        delete msg;
        return NULL;
    }
    
    // 6. 消费整个消息（包括头部）
    input_stream.consume();
    
    return msg;
}
void ProtoMsgDispatcher::handle(TcpConnectionPtr con,Message* msg) 
{
    auto p=protocbs_.find(msg->GetDescriptor());
    if(p!=protocbs_.end()) 
    {
        assert(p->second);
        p->second(con, msg);
    } 
    else 
    {
        LOG_WARN("unknown message type :"<<msg->GetTypeName().c_str());
    }
    delete msg;
}

}
}