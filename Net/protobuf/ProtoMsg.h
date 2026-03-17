#ifndef YY_NET_PROTOBUF_PROTOMSG_H_
#define YY_NET_PROTOBUF_PROTOMSG_H_
#include <google/protobuf/message.h>
#include "../TcpConnection.h"
#include <map>

namespace yy
{
namespace net
{

typedef TcpConnection::Buffer  Buffer; 
typedef ::google::protobuf::Message Message;
typedef ::google::protobuf::Descriptor Descriptor;
typedef std::function<void(TcpConnectionPtr con, Message* msg)> ProtoCallBack;

struct ProtoMsgCodec 
{
    static void encode(Message* msg, Buffer& buf);
    static Message* decode(Buffer& buf);
    static bool msgComplete(Buffer& buf);
    static Message* createMessage(const std::string& typeName);
    static const size_t MAX_MESSAGE_SIZE=64*1024*1024;  // 64MB
    static const size_t MAX_NAME_LENGTH=256;  // 256字节
};

struct ProtoMsgDispatcher 
{
    void handle(TcpConnectionPtr con,Message* msg);
    template <typename M>
    void onMsg(std::function<void(TcpConnectionPtr con,M* msg)> cb) 
    {
        protocbs_[M::descriptor()]=[cb](TcpConnectionPtr con,Message* msg){cb(con,dynamic_cast<M*>(msg)); };
    }

   private:
    std::map<const Descriptor*, ProtoCallBack> protocbs_;
};

inline bool ProtoMsgCodec::msgComplete(Buffer& buf) 
{
#ifndef NDEBUG
    const uint32_t num=*reinterpret_cast<const uint32_t*>(buf.peek());
    (void)num;
#endif

    return buf.get_readable_size()>=4&&buf.get_readable_size()>=*reinterpret_cast<const uint32_t*>(buf.peek());
}     
}   
}
#endif