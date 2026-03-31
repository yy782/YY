/**
 * @file HttpServer.h
 * @brief HTTP服务器的定义
 * 
 * 本文件定义了HTTP服务器类，用于处理HTTP请求并返回HTTP响应。
 */

#ifndef _YY_NET_HTTP_HTTPSERVER_H_
#define _YY_NET_HTTP_HTTPSERVER_H_
#include "http.h"
#include "../../Common/LogFilter.h"
#include "../include/TcpServer.h"
namespace yy 
{
namespace net 
{
namespace Http 
{
/**
 * @brief HTTP服务器类
 * 
 * HTTPServer用于处理HTTP请求并返回HTTP响应，支持GET、POST、PUT、DELETE等HTTP方法。
 */
class HTTPServer 
{
public:
    /**
     * @brief HTTP回调函数类型
     */
    typedef std::function<void(Http::HttpRequest&, Http::HttpResponse&)> HttpCallback;
    /**
     * @brief 连接回调函数类型
     */
    typedef TcpServer::ServicesConnectedCallBack ServicesConnectedCallBack;
    /**
     * @brief 构造函数
     * 
     * @param addr 服务器地址
     * @param AcceptorNum 接收器数量
     * @param WorkThreadnum 工作线程数量
     */
    HTTPServer(const Address& addr,int AcceptorNum,int WorkThreadnum): 
    server_(addr,AcceptorNum,WorkThreadnum)
        {
        
        server_.setConnectCallBack([this](int Cfd,const Address& Caddr,EventLoop* Cloop)
        {
            assert(SconCb_);
            auto conn=SconCb_(Cfd,Caddr,Cloop);
            onConnection(conn);
            conn->setMessageCallBack([this](TcpConnectionPtr con){
                onMessage(con);
            });
            return conn;
        });
    }
    /**
     * @brief 设置连接回调函数
     * 
     * @param cb 连接回调函数
     */
    void setConCallback(ServicesConnectedCallBack cb) {
        SconCb_=cb;
    }    
    /**
     * @brief 注册GET路由
     * 
     * @param path 请求路径
     * @param cb 回调函数
     */
    void get(const std::string& path, HttpCallback cb) {
        router_.get(path, cb);
    }
    
    /**
     * @brief 注册POST路由
     * 
     * @param path 请求路径
     * @param cb 回调函数
     */
    void post(const std::string& path, HttpCallback cb) {
        router_.post(path, cb);
    }
    
    /**
     * @brief 注册PUT路由
     * 
     * @param path 请求路径
     * @param cb 回调函数
     */
    void put(const std::string& path, HttpCallback cb) {
        router_.put(path, cb);
    }
    
    /**
     * @brief 注册DELETE路由
     * 
     * @param path 请求路径
     * @param cb 回调函数
     */
    void del(const std::string& path, HttpCallback cb) {
        router_.del(path, cb);
    }
    
    /**
     * @brief 设置默认处理器（404）
     * 
     * @param cb 回调函数
     */
    void setDefaultHandler(HttpCallback cb) {
        router_.setDefaultHandler(cb);
    }
    
    /**
     * @brief 启动服务器
     */
    void start() {
        server_.loop();
    }
    /**
     * @brief 等待服务器停止
     */
    void wait(){
        server_.wait();
    }
    /**
     * @brief 停止服务器
     */
    void stop() {
        server_.stop();
    }
    
    /**
     * @brief 析构函数
     */
    ~HTTPServer() {
        LOG_SYSTEM_INFO("HTTP server stop!");
    }

private:
    /**
     * @brief HTTP连接上下文
     */
    struct HTTPConnectionContext {
        Http::HttpParser parser; ///< HTTP解析器
        bool keepAlive=true;   ///< 是否保持连接
    };
    
    /**
     * @brief 处理连接建立
     * 
     * @param conn TCP连接
     */
    void onConnection(TcpConnectionPtr conn) 
    {
        conn->context<HTTPConnectionContext>()=HTTPConnectionContext();
    }
    
    /**
     * @brief 处理收到的消息
     * 
     * @param conn TCP连接
     */
    void onMessage(TcpConnectionPtr conn) 
    {
        TcpBuffer& buffer=conn->recvBuffer();
        HTTPConnectionContext& ctx=conn->context<HTTPConnectionContext>();
        while(true)
        {
            Http::ParseResult result=ctx.parser.parseRequest(
                buffer.peek(), 
                buffer.readable_size(),
                true  // copy body
            );
            
            if (result==Http::ParseResult::Error) 
            {
                // 400 Bad Request
                Http::HttpResponse resp;
                resp.setStatus(Http::HttpResponse::Status::BAD_REQUEST,"Bad Request");
                resp.body_="400 Bad Request";
                resp.headers_["Content-Type"]="text/plain";
                resp.headers_["Connection"]="close";
                
                std::string response = ctx.parser.buildResponse(resp);
                conn->send(std::move(response));
                conn->disconnect();
                return;
            }
            
            if (result==Http::ParseResult::Continue100) 
            {
                // 发送100 Continue
                const char* cont = "HTTP/1.1 100 Continue\r\n\r\n";
                conn->send(cont, strlen(cont));
                return;
            }
            
            if (result==Http::ParseResult::NotComplete) 
            {
                // 数据不完整，等待更多数据
                return;
            }
            
            if (result==Http::ParseResult::Complete) 
            {
                // 解析完成，处理请求
                Http::HttpRequest& req=ctx.parser.getRequest();
                Http::HttpResponse resp;
                
                // 获取Connection头判断是否保持连接
                std::string connection=req.getHeader("connection");
                ctx.keepAlive=(connection!="close");
                
                // 路由处理
                router_.routeRequest(req, resp);
                
                // 设置Connection头
                if (ctx.keepAlive) 
                {
                    resp.headers_["Connection"]="keep-alive";
                } else 
                {
                    resp.headers_["Connection"]="close";
                }
                
                // 构建响应并发送
                std::string response = ctx.parser.buildResponse(resp);
                conn->send(std::move(response));
                
                // 消费缓冲区中的数据
                buffer.consume(ctx.parser.getRequest().getByte());
                
                // 如果不保持连接，则关闭
                if (!ctx.keepAlive) 
                {
                    conn->disconnect();
                } else 
                {
                    // 重置解析器，准备处理下一个请求
                    ctx.parser.clear();
                }
            }            
        }

    }
    
    /**
     * @brief TCP服务器
     */
    TcpServer server_;
    /**
     * @brief HTTP路由器
     */
    Http::HttpRouter router_;
    /**
     * @brief 连接回调函数
     */
    ServicesConnectedCallBack SconCb_;
};
}
}    
}

#endif 