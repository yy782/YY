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
bool isDisConnected=false;
int main() {
    EventLoopThread thread;
    Address serverAddr("127.0.0.1", 8080);
    HTTPClient client(serverAddr,thread.run());
    std::map<std::string, std::string> headers;
    headers["Content-Type"] = "application/json";    
    client.setConCallback([&](int Cfd,const Address& Caddr,EventLoop* Cloop){
        auto conn=TcpConnection::accept(Cfd,Caddr,Cloop,Event(LogicEvent::Read));
        conn->setCloseCallBack([&](TcpConnectionPtr){
            std::cout << "Connection closed" << std::endl;
            isDisConnected=true;
        });
      
        return conn;
    });  
    client.connect();
    sleep(2);
    client.get("/user?name=Tom", [](const Http::HttpResponse& resp) {
            ++MsgCount;
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
    while(!isDisConnected)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }     
    
    thread.stop();
    
    return 0;
}