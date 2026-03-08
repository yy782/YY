#include "Codec.h"

#include <arpa/inet.h>
namespace yy
{
namespace net
{
int LineCodec::tryDecode(string_view data, string_view &msg) {
    if (data.size() == 1 && data[0] == 0x04) {
        msg = data;
        return 1;
    }
    for (size_t i = 0; i < data.size(); i++) {
        if (data[i] == '\n') {
            if (i > 0 && data[i - 1] == '\r') {
                msg = string_view(data.data(), i - 1);
                return static_cast<int>(i + 1);
            } else {
                msg = string_view(data.data(), i);
                return static_cast<int>(i + 1);
            }
        }
    }
    return 0;
}
void LineCodec::encode(string_view msg, TcpBuffer &buf) {
    buf.append(msg).append("\r\n");
}

int LengthCodec::tryDecode(string_view data, string_view &msg) {
    if (data.size() < 8) {
        return 0;
    }
    int len=::ntohl(*(int32_t *) (data.data() + 4));
    if (len > 1024 * 1024 || memcmp(data.data(), "mBdT", 4) != 0) {
        return -1;
    }
    if ((int) data.size() >= len + 8) {
        msg = string_view(data.data() + 8, len);
        return len + 8;
    }
    return 0;
}
void LengthCodec::encode(string_view msg, TcpBuffer &buf) {
    buf.append("mBdT").appendValue(::htonl((int32_t) msg.size())).append(msg);
}

}    
}