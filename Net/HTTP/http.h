
#ifndef _YY_NET_HTTP_H_
#define  _YY_NET_HTTP_H_
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <map>
#include "../../Common/string_view.h"
#include <functional>
namespace yy 
{
namespace net 
{

namespace Http
{
 // HTTP解析结果状态
enum class ParseResult {
    Error,
    Complete,
    NotComplete,
    Continue100,
};

// HTTP版本
enum struct Version {
    UNKNOWN,
    HTTP_1_0,
    HTTP_1_1
};

// Connection类型
enum struct Connection {
    UNKNOWN,
    keep_alive,
    close
};


// HTTP消息基类
class HttpMsg {
public:
    HttpMsg();
    virtual ~HttpMsg() {}
    
    virtual int encode(std::string& buf) = 0;
    virtual ParseResult tryDecode(string_view buf, bool copyBody = true) = 0;
    virtual void clear();
    
    std::string getHeader(const std::string& name);
    string_view getBody();
    size_t getByte() const;
    
    std::map<std::string, std::string> headers_;
    std::string version_;
    std::string body_;
    string_view bodyView_;

protected:
    bool complete_;
    size_t contentLen_;
    size_t scanned_;
    
    ParseResult tryDecode_(string_view buf, bool copyBody, string_view* line1);
};

// HTTP请求
class HttpRequest : public HttpMsg {
public:
    HttpRequest();
    
    enum struct Method {
        UNKNOWN,
        GET,
        POST,
        PUT,
        DELETE,
        HEAD
    };
    
    Method method_;
    std::string url_;
    std::string queryUrl_;
    std::map<std::string, std::string> args_;
    
    std::string getArg(const std::string& name);
    static std::string methodToString(Method m);
    
    virtual int encode(std::string& buf) override;
    virtual ParseResult tryDecode(string_view buf, bool copyBody = true) override;
    virtual void clear() override;

private:
    static Method stringToMethod(const std::string& s);
    void parseUrlParams();
};

// HTTP响应
class HttpResponse : public HttpMsg {
public:
    HttpResponse();
    
    enum struct Status {
        UNKNOWN = 0,
        OK = 200,
        BAD_REQUEST = 400,
        NOT_FOUND = 404,
        INTERNAL_ERROR = 500
    };
    
    Status status_;
    std::string statusMsg_; 
    
    void setStatus(Status st, const std::string& msg = "");
    void setNotFound();
    
    virtual int encode(std::string& buf) override;
    virtual ParseResult tryDecode(string_view buf, bool copyBody = true) override;
    virtual void clear() override;

private:
    static std::string statusToString(Status s);
};

// HTTP解析器类
class HttpParser {
public:
    HttpParser();
    
    ParseResult parseRequest(const char* data, size_t len, bool copyBody = true);
    std::string buildResponse(HttpResponse& resp);
    
    HttpRequest& getRequest();
    HttpResponse& getResponse();
    void clear();

private:
    enum class ParseState {
        REQUEST_LINE,
        HEADERS,
        BODY,
        COMPLETE
    };
    
    HttpRequest req_;
    HttpResponse resp_;
    std::string inputBuffer_;
    ParseState parseState_;
};

typedef std::function<void(HttpRequest&, HttpResponse&)> HttpHandler;

class HttpRouter {
public:
    HttpRouter();
    
    // 注册路由
    void route(const std::string& url, HttpRequest::Method method, HttpHandler handler);
    void get(const std::string& url, HttpHandler handler);
    void post(const std::string& url, HttpHandler handler);
    void put(const std::string& url, HttpHandler handler);
    void del(const std::string& url, HttpHandler handler);  // delete是关键字，用del
    void head(const std::string& url, HttpHandler handler);
    
    // 设置默认处理器
    void setDefaultHandler(HttpHandler handler);
    
    // 路由请求
    bool routeRequest(HttpRequest& req, HttpResponse& resp) const;
    
    // 清空路由
    void clear();
    
    // 获取路由数量
    size_t size() const;

private:
    struct RouteEntry {
        std::string url_;
        HttpRequest::Method method_;
        HttpHandler handler_;
    };
    
    std::vector<RouteEntry> routes_;
    HttpHandler defaultHandler_;
    
    static void defaultNotFoundHandler(HttpRequest& req, HttpResponse& resp);
};



// ParseResult parseHttpRequest(const char* data, size_t len, HttpRequest& req);
// std::string buildHttpResponse(HttpResponse& resp);
// bool routeHttpRequest(HttpRequest& req, HttpResponse& resp);


}
}
}
#endif