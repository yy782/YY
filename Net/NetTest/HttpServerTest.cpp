#include "../TcpServer.h"
#include "../HTTP/http.h"
#include <functional>
#include <map>
#include "../../Common/SyncLog.h"
#include "../HTTP/HttpServer.h"
#include "../TcpConPool.h"
#include <vector>
#include <memory>
using namespace yy;
using namespace yy::net;
using namespace yy::net::Http;
// ./HttpServerTest

int main() {
    EventLoop loop;
    Address serveraddr(8080,true);
    HTTPServer server(serveraddr,1,2);
    server.setConCallback([](int Cfd,const Address& Caddr,EventLoop* Cloop){
        auto conn=TcpConnection::accept(Cfd,Caddr,Cloop);
        LOG_SYSTEM_INFO("HTTP connection! " << Caddr.sockaddrToString());
        conn->setCloseCallBack([](TcpConnectionPtr con){  
            LOG_SYSTEM_INFO("HTTP connection closed! " << con->addr().sockaddrToString());        
        });
        return conn;
    });
    SyncLog::getInstance("../Log.log").getFilter() 
        .set_global_level(LOG_LEVEL_DEBUG) 
        .set_module_enabled("TCP")
        .set_module_enabled("SYSTEM")
        .set_module_enabled("HTTP");
    // 注册路由
    server.get("/", [](Http::HttpRequest&, Http::HttpResponse& resp) {
        resp.setStatus(Http::HttpResponse::Status::OK);
        resp.body_ = "<h1>Welcome to HTTP Server</h1>";
        resp.headers_["Content-Type"] = "text/html";
    });
    
    server.get("/user", [](Http::HttpRequest& req, Http::HttpResponse& resp) {
        std::string name = req.getArg("name");
        resp.setStatus(Http::HttpResponse::Status::OK);
        resp.body_ = "Hello, " + (name.empty() ? "Guest" : name);
        resp.headers_["Content-Type"] = "text/plain";
    });
    
    server.post("/api/user", [](Http::HttpRequest& req, Http::HttpResponse& resp) {
        resp.setStatus(Http::HttpResponse::Status::OK);
        resp.body_ = "User created: " + req.body_;
        resp.headers_["Content-Type"] = "text/plain";
    });
    
    // 自定义404
    server.setDefaultHandler([](Http::HttpRequest& req, Http::HttpResponse& resp) {
        resp.setStatus(Http::HttpResponse::Status::NOT_FOUND);
        resp.body_ = "<html><body><h1>404 - " + req.url_ + " not found</h1></body></html>";
        resp.headers_["Content-Type"] = "text/html";
    });
    LOG_SYSTEM_INFO("HTTP Server starting on port 8080...");
    server.start();
    server.wait();
    return 0;
}