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