
#ifndef _YY_NET_HTTP_H_
#define  _YY_NET_HTTP_H_
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unordered_map>
namespace yy 
{
namespace net 
{

namespace Http{
    enum struct Version{
        UNKNOWN,
        HTTP_1_0,
        HTTP_1_1
    };
    enum struct Connection{
        UNKNOWN,
        keep_alive,
        close
    };
    struct HttpRequest{
        enum struct Method{
            UNKNOWN,
            GET,
            POST,
            PUT
        };
        Method method;
        const char* url;
        Version version;
        const char* host;
        Connection conn;
        int content_length=-1;
        const char* body;
    };
    struct HttpResponse{
        enum struct Status{
            UNKNOWN=0,
            OK=200,
            NOT_FOUND=404
        };
        
        Version version;
        Status status;
        Connection conn;
        int content_length=-1;
        const char* body;
    };
    typedef void (*HttpHandler)(HttpRequest& req,HttpResponse& res);
    struct HttpRoute{//路由表项
        const char* url;
        HttpRequest::Method method;
        HttpHandler handler;
    };
}
}
}
#endif