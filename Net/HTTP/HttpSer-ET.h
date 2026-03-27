#ifndef _YY_NET_HTTP_HTTPSER_ET_H_
#define _YY_NET_HTTP_HTTPSER_ET_H_
#include "http.h"
#include "../TcpServer.h"
namespace yy 
{
namespace net 
{
namespace Http 
{
class HTTPSerET {
public:
    typedef std::function<void(Http::HttpRequest&, Http::HttpResponse&)> HttpCallback;
    
    HTTPSerET(const Address& addr, int thread_num, EventLoop* loop)
        : server_(addr, thread_num, loop),
        loop_(loop)
        {
        
        server_.setConnectCallBack([this](TcpConnectionPtr conn)
        {
            onConnection(conn);
        });
        server_.setMessageCallBack([this](TcpConnectionPtr conn)
        {
            onMessage(conn);
        });
        server_.setCloseCallBack([this](TcpConnectionPtr conn)
        {
            onClose(conn);
        });
    }
    
    // 路由注册
    void get(const std::string& path, HttpCallback cb) {
        router_.get(path, cb);
    }
    
    void post(const std::string& path, HttpCallback cb) {
        router_.post(path, cb);
    }
    
    void put(const std::string& path, HttpCallback cb) {
        router_.put(path, cb);
    }
    
    void del(const std::string& path, HttpCallback cb) {
        router_.del(path, cb);
    }
    
    // 设置默认处理器（404）
    void setDefaultHandler(HttpCallback cb) {
        router_.setDefaultHandler(cb);
    }
    
    void start() {
        server_.loop();
    }
    
    void stop() {
        server_.stop();
    }
    
    ~HTTPSerET() {
        LOG_SYSTEM_INFO("HTTP server stop!");
    }

private:
    struct HTTPConnectionContext {
        Http::HttpParser parser;
        bool keepAlive = true;
    };
    
    void onConnection(TcpConnectionPtr conn) {
        auto addr = conn->addr();
        LOG_SYSTEM_INFO("HTTP connection! " << addr.sockaddrToString());
        conn->setTcpNoDelay(true);
        conn->context<HTTPConnectionContext>()=HTTPConnectionContext();
        conn->setEvent(Event(LogicEvent::Read|LogicEvent::Edge));
    }
    void onMessage(TcpConnectionPtr conn) {
            TcpBuffer& buffer = conn->recvBuffer();
        HTTPConnectionContext& ctx=conn->context<HTTPConnectionContext>();
        while(true)
        {
            Http::ParseResult result = ctx.parser.parseRequest(
                buffer.peek(), 
                buffer.readable_size(),
                true  // copy body
            );
            
            if (result == Http::ParseResult::Error) {
                // 400 Bad Request
                Http::HttpResponse resp;
                resp.setStatus(Http::HttpResponse::Status::BAD_REQUEST, "Bad Request");
                resp.body_ = "400 Bad Request";
                resp.headers_["Content-Type"] = "text/plain";
                resp.headers_["Connection"] = "close";
                
                std::string response = ctx.parser.buildResponse(resp);
                conn->send(std::move(response));
                conn->disconnect();
                return;
            }
            
            if (result == Http::ParseResult::Continue100) {
                // 发送100 Continue
                const char* cont = "HTTP/1.1 100 Continue\r\n\r\n";
                conn->send(cont, strlen(cont));
                return;
            }
            
            if (result == Http::ParseResult::NotComplete) {
                // 数据不完整，等待更多数据
                return;
            }
            
            if (result == Http::ParseResult::Complete) {
                // 解析完成，处理请求
                Http::HttpRequest& req = ctx.parser.getRequest();
                Http::HttpResponse resp;
                
                // 获取Connection头判断是否保持连接
                std::string connection = req.getHeader("connection");
                ctx.keepAlive = (connection != "close");
                
                // 路由处理
                router_.routeRequest(req, resp);
                
                // 设置Connection头
                if (ctx.keepAlive) {
                    resp.headers_["Connection"] = "keep-alive";
                } else {
                    resp.headers_["Connection"] = "close";
                }
                
                // 构建响应并发送
                std::string response = ctx.parser.buildResponse(resp);
                conn->send(std::move(response));
                
                // 消费缓冲区中的数据
                buffer.consume(ctx.parser.getRequest().getByte());
                
                // 如果不保持连接，则关闭
                if (!ctx.keepAlive) {
                    conn->disconnect();
                } else {
                    // 重置解析器，准备处理下一个请求
                    ctx.parser.clear();
                }
            }            
        }

    }
    
    void onClose(TcpConnectionPtr conn) {
        auto addr = conn->addr();   
        LOG_SYSTEM_INFO("HTTP connection closed! " << addr.sockaddrToString());

    }
    
    TcpServer server_;
    Http::HttpRouter router_;
    EventLoop* loop_;
};
}
}    
}

#endif 