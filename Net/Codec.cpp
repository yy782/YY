#include "Codec.h"

#include <arpa/inet.h>
namespace yy
{
namespace net
{
int LineCodec::tryDecode(stringPiece data,stringPiece& msg) {
    if(data.size()==1&&data[0]==0x04) // 0x04是EOF
    {
        msg=data;
        return 1;
    }
    for(size_t i=0;i<data.size();i++) 
    {
        if(data[i]=='\n') 
        {
            size_t msg_end=(i>0&&data[i-1]=='\r')?i-1:i;  //兼容不同平台
            msg=stringPiece(data.data(),msg_end);
            return static_cast<int>(i+1);
        }
    }
    return 0;
}
void LineCodec::encode(stringPiece msg, TcpBuffer& buf) 
{
    buf.FluentAppend(msg)
        .append("\r\n",2);
}

int LengthCodec::tryDecode(stringPiece data, stringPiece &msg) 
{
    if(data.size()<8) 
    {
        return 0;
    }
    size_t len=static_cast<size_t>(::ntohl(*(reinterpret_cast<int32_t*>(data.data()+4))));
    if(len>1024*1024||memcmp(data.data(),"mBdT",4)!=0) 
    {
        return -1;
    }
    if(data.size()>=len+8) 
    {
        msg=stringPiece(data.data()+8,len);
        return static_cast<int>(len+8);
    }
    return 0;
}
void LengthCodec::encode(stringPiece msg, TcpBuffer &buf) {
    buf.FluentAppend("mBdT",4)
        .FluentAppend(::htonl(static_cast<uint32_t>(msg.size())))
        .append(msg);
}

}    
}