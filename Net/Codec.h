#ifndef _YY_NET_PROTOCOLCODEC_H_
#define _YY_NET_PROTOCOLCODEC_H_

#include <google/protobuf/message.h>
#include "TcpBuffer.h"
#include "../Common/noncopyable.h"
#include "../Common/stringPiece.h"
namespace yy
{
namespace net
{
// struct CodecBase 
// {
//     virtual int tryDecode(stringPiece data,stringPiece& msg)=0;
//     virtual void encode(stringPiece msg,TcpBuffer& buf)=0;
//     virtual CodecBase* clone()=0;
//     virtual ~CodecBase()=default;
// };

// struct LineCodec:public CodecBase 
// {
//     int tryDecode(stringPiece data,stringPiece& msg)override;
//     void encode(stringPiece msg,TcpBuffer& buf)override;
//     CodecBase* clone()override{return new LineCodec();}
// };
// struct LengthCodec:public CodecBase {
//     int tryDecode(stringPiece data,stringPiece& msg)override;
//     void encode(stringPiece msg,TcpBuffer& buf)override;
//     CodecBase *clone()override{return new LengthCodec();}
// };
struct LineCodec
{
    static int tryDecode(stringPiece data,stringPiece& msg);
    static void encode(stringPiece msg,TcpBuffer& buf);
};
struct LengthCodec
{
    static int tryDecode(stringPiece data,stringPiece& msg);
    static void encode(stringPiece msg,TcpBuffer& buf);
};

}
}

#endif