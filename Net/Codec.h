#ifndef _YY_NET_PROTOCOLCODEC_H_
#define _YY_NET_PROTOCOLCODEC_H_

#include <google/protobuf/message.h>
#include "TcpBuffer.h"
#include "../Common/noncopyable.h"
#include "string_view.h"
namespace yy
{
namespace net
{
struct CodecBase 
{
    virtual int tryDecode(string_view data,string_view& msg)=0;
    virtual void encode(string_view msg,TcpBuffer& buf)=0;
    virtual CodecBase* clone()=0;
    virtual ~CodecBase()=default;
};

struct LineCodec:public CodecBase 
{
    int tryDecode(string_view data,string_view& msg)override;
    void encode(string_view msg,TcpBuffer& buf)override;
    CodecBase* clone()override{return new LineCodec();}
};
struct LengthCodec:public CodecBase {
    int tryDecode(string_view data,string_view& msg)override;
    void encode(string_view msg,TcpBuffer& buf)override;
    CodecBase *clone()override{return new LengthCodec();}
};

}
}

#endif