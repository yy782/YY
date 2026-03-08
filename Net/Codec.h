#ifndef _YY_NET_PROTOCOLCODEC_H_
#define _YY_NET_PROTOCOLCODEC_H_

#include <google/protobuf/message.h>
#include "TcpBuffer.h"
#include "../Common/noncopyable.h"
#include "ProtoBufBuffer.h"
#include "string_view.h"
namespace yy
{
namespace net
{
struct CodecBase {
    // > 0 解析出完整消息，消息放在msg中，返回已扫描的字节数
    // == 0 解析部分消息
    // < 0 解析错误
    virtual int tryDecode(string_view data, string_view &msg) = 0;
    virtual void encode(string_view msg, TcpBuffer &buf) = 0;
    virtual CodecBase *clone() = 0;
    virtual ~CodecBase() = default;
};

//以\r\n结尾的消息
struct LineCodec : public CodecBase {
    int tryDecode(string_view data, string_view &msg) override;
    void encode(string_view msg, TcpBuffer &buf) override;
    CodecBase *clone() override { return new LineCodec(); }
};

//给出长度的消息
struct LengthCodec : public CodecBase {
    int tryDecode(string_view data, string_view &msg) override;
    void encode(string_view msg, TcpBuffer &buf) override;
    CodecBase *clone() override { return new LengthCodec(); }
};

}
}

#endif