#include <iostream>
#include "../Codec.h"
#include <arpa/inet.h>
using namespace yy;
using namespace yy::net;

//./CodecTestLength
void testLengthCodec() {
    std::cout << "=== 测试 LengthCodec ===" << std::endl;
    TcpBuffer buffer;
    
    // 测试用例1: 正常编码解码
    std::string msg1 = "hello, this is a test message";
    LengthCodec::encode(msg1, buffer);
    
    std::cout << "编码后buffer大小: " << buffer.get_readable_size() << " 字节" << std::endl;
    
    string_view data=buffer.getReadView();
    string_view decoded;
    int ret = LengthCodec::tryDecode(data, decoded);
    
    assert(ret > 0);
    assert(decoded == msg1);
    std::cout << "解码成功: \"" << decoded.toString() << "\"" << std::endl;
    std::cout << "解码消耗 " << ret << " 字节" << std::endl;
    
    // 测试用例2: 空消息
    buffer.clear();
    LengthCodec::encode("", buffer);
    data=buffer.getReadView();
    ret = LengthCodec::tryDecode(data, decoded);
    assert(ret > 0);
    assert(decoded.empty());
    std::cout << "空消息解码成功" << std::endl;
    
    // 测试用例3: 消息太长（超过限制）
    buffer.clear();
    std::string tooLong(2 * 1024 * 1024, 'a');  // 2MB > 1MB
   LengthCodec::encode(tooLong, buffer);
    data =buffer.getReadView();
    ret = LengthCodec::tryDecode(data, decoded);
    assert(ret == -1);  // 应该返回错误
    std::cout << "太长的消息被正确拒绝" << std::endl;
    
    // 测试用例4: 魔数错误
    buffer.clear();
    buffer.append("xxxx", 4);  // 错误的魔数
    uint32_t len = htonl(10);
    buffer.append(reinterpret_cast<char*>(&len), 4);
    buffer.append("0123456789", 10);
    
    data =buffer.getReadView();
    ret = LengthCodec::tryDecode(data, decoded);
    assert(ret == -1);
    std::cout << "错误魔数被正确拒绝" << std::endl;
    
    // 测试用例5: 数据不足
    buffer.clear();
    len = htonl(100);
    buffer.append("mBdT", 4);
    buffer.append(reinterpret_cast<char*>(&len), 4);
    // 只放50字节，不是完整的100字节
    
    data =buffer.getReadView();
    ret = LengthCodec::tryDecode(data, decoded);
    assert(ret == 0);  // 应该返回0，表示需要更多数据
    std::cout << "数据不足检测正确" << std::endl;
    


    std::cout << "=== 测试多个消息粘包 ===" << std::endl;
    buffer.clear();

    
    // 1. 编码两个消息到同一个buffer
    LengthCodec::encode("first", buffer);
    LengthCodec::encode("second", buffer);
    
    // 2. 获取数据视图
    data = buffer.getReadView();
    string_view msg;
    
    // 3. 解码第一个消息
    ret = LengthCodec::tryDecode(data, msg);
    msg1=std::string(msg.data(), msg.size());
    std::cout << "第一个消息: " << msg1 << std::endl;
    assert(msg1 == "first");
    
    // 4. 移动视图，解码第二个消息
    data = string_view(data.data() + ret, data.size() - ret);
    ret = LengthCodec::tryDecode(data, msg);
    std::string msg2(msg.data(), msg.size());
    std::cout << "第二个消息: " << msg2 << std::endl;
    assert(msg2 == "second");
    
    std::cout << "测试通过！" << std::endl;
}
int main()
{
    testLengthCodec();
    return 0;
}