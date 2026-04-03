#include <yy/Net/include/InetAddress.h>
#include <yy/Net/HTTP/HttpServer.h>
class Server 
{
public:
    /**
     * @brief 构造函数
     * @param port 服务器监听端口
     */
    Server(const char* ip,int port):
    m_addr_(ip,port),
    http_server_(m_addr_,1,4)
    {
        http_server_.get("/", [](yy::net::Http::HttpRequest&, yy::net::Http::HttpResponse& resp) {
            resp.setStatus(yy::net::Http::HttpResponse::Status::OK);
            resp.body_ = "<h1>Welcome to HTTP Server</h1>";
            resp.headers_["Content-Type"] = "text/html";
        });        
    };
    /**
     * @brief 启动服务器
     */
    void start(){http_server_.start();}
    void wait(){http_server_.wait();}
    void stop(){http_server_.stop();}
private:
    yy::net::Address m_addr_;  ///< 服务器地址
    yy::net::Http::HttpServer http_server_; ///< HTTP服务器实例
};