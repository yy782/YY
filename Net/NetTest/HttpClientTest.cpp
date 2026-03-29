#include "../TcpClient.h"
#include "../HTTP/http.h"
#include <functional>
#include <map>
#include <iostream>
#include "../EventLoopThread.h"
#include "../HTTP/HttpClient.h"
#include "../TimerQueue.h"
using namespace yy;
using namespace yy::net;
using namespace yy::net::Http;
// ./HttpClientTest
int MsgCount=0;

int main() {
    EventLoop loop;
    Address serverAddr("127.0.0.1", 8080);
    
    HTTPClient client(serverAddr, &loop);
    std::map<std::string, std::string> headers;
    headers["Content-Type"] = "application/json";    
    client.setConCallback([&](int Cfd,const Address& Caddr,EventLoop* Cloop){
        auto conn=TcpConnection::accept(Cfd,Caddr,Cloop);
        conn->setCloseCallBack([&](TcpConnectionPtr){
            std::cout << "Connection closed" << std::endl;
            loop.quit();
        });
        return conn;
    });    
    client.get("/user?name=Tom", [](const Http::HttpResponse& resp) {
            std::cout << "Response: " << resp.body_ << std::endl;
    });
    client.post("/api/user", "{\"name\":\"Tom\",\"age\":20}", 
            headers, [&client](const Http::HttpResponse& resp) {
            ++MsgCount;
            std::cout << "POST Response: " << resp.body_ << std::endl;
            if(MsgCount==2)
            {
                client.disconnect();
            }
    });    
    client.connect();
    loop.loop();
    
    return 0;
}