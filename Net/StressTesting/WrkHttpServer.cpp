#include "../HTTP/HttpServer.h"
#include "../../Common/TimeStamp.h"
#include "../include/TcpConPool.h"
#include <vector>
#include "../../Common/SyncLog.h"
using namespace yy;
using namespace yy::net;
using namespace yy::net::Http;
//cd programs/yy/Net/StressTesting
//./WrkHttpServer
extern char favicon[555];
// int main()
// {
//     return 0;
// }
int main(int argc, char* argv[])
{
    Signal::signal(SIGPIPE,[](){});
    // SyncLog::getInstance("../Log.log").getFilter() 
    // .set_global_level(LOG_LEVEL_ERROR)
    // .set_module_enabled("LOOP") 
    // ;
    bool isET=true;
    bool useConPool=true;
    Address serveraddr(8080,true);
    int numThreads=4;
    int AcceptorNum=2;
    if (argc>1)
    {
        numThreads =std::atoi(argv[1]);
        AcceptorNum=std::atoi(argv[2]);
        isET=(atoi(argv[3])==1?true:false);
        useConPool=(atoi(argv[4])==1?true:false);
    }
    Event event=(isET)?Event(LogicEvent::Read|LogicEvent::Edge):Event(LogicEvent::Read);
    HTTPServer ser(serveraddr,AcceptorNum,numThreads);
    std::vector<std::unique_ptr<TcpConPool>> conPools;
    if(useConPool)
    {
        TcpConPool::Config config;
        config.event_=event;
        config.isTcpAlive_=true;
        config.ScloseCallBack_=([&conPools](TcpConnectionPtr con){
            auto addr = con->addr();   
            LOG_SYSTEM_INFO("HTTP connection closed! " << addr.sockaddrToString());  
            conPools[con->loop()->id()]->release(con);//////////////////////////////////////////////////////////////////
        });        
        for(int i=0;i<numThreads;++i)
        {
            conPools.emplace_back(std::make_unique<TcpConPool>(1000,config,i));
        }        
        ser.setConCallback([&conPools,&event](int Cfd,const Address& Caddr,EventLoop* Cloop){
            if(event.has(LogicEvent::Edge) && !sockets::setNonBlocking(Cfd))
            {
                LOG_ERRNO(errno);
                sockets::close(Cfd);
            }
            auto conn=conPools[Cloop->id()]->acquire(Cfd,Caddr,Cloop);
            auto addr = conn->addr();
            LOG_SYSTEM_INFO("HTTP connection! " << addr.sockaddrToString());            
            return conn;
        });
    }
    else 
    {
        ser.setConCallback([&event](int Cfd,const Address& Caddr,EventLoop* Cloop){
            if(event.has(LogicEvent::Edge) && !sockets::setNonBlocking(Cfd))
            {
                LOG_ERRNO(errno);
                sockets::close(Cfd);
            }
            auto conn=TcpConnection::accept(Cfd,Caddr,Cloop,event);///////////////////可能在还没来的即设计回调就handleRead了
            conn->setTcpNoDelay(true);
            conn->setCloseCallBack([](TcpConnectionPtr con){
                auto addr = con->addr();   
                LOG_SYSTEM_INFO("HTTP connection closed! " << addr.sockaddrToString());        
            });
            auto addr = conn->addr();
            LOG_SYSTEM_INFO("HTTP connection! " << addr.sockaddrToString());
            return conn;
        });
    }

    ser.get("/",[](Http::HttpRequest&, Http::HttpResponse& resp) {
        resp.setStatus(Http::HttpResponse::Status::OK);
        resp.headers_["Content-Type"] = "text/html";
        resp.headers_["Server"]="HttpSer";
        std::string now = TimeStamp<LowPrecision>::nowToString();
        resp.body_=std::string("<html><head><title>This is title</title></head>"
            "<body><h1>Hello</h1>Now is " + now +
            "</body></html>"); 
    });
    ser.get("/favicon.ico",[](Http::HttpRequest&, Http::HttpResponse& resp) {
        resp.setStatus(Http::HttpResponse::Status::OK);
        resp.headers_["Content-Type"] = "image/png";
        resp.body_=std::string(favicon, sizeof favicon);
    });    
    ser.get("/hello",[](Http::HttpRequest&, Http::HttpResponse& resp) {
        resp.setStatus(Http::HttpResponse::Status::OK);
        resp.headers_["Content-Type"] = "text/plain";
        resp.body_=std::string("hello, world!\n");
    });
    ser.start();
    ser.wait();
}
    
    
//wrk -c 100000 -t 8 -d 30s http://127.0.0.1:8080/

char favicon[555] = {
  '\x89', 'P', 'N', 'G', '\xD', '\xA', '\x1A', '\xA',
  '\x0', '\x0', '\x0', '\xD', 'I', 'H', 'D', 'R',
  '\x0', '\x0', '\x0', '\x10', '\x0', '\x0', '\x0', '\x10',
  '\x8', '\x6', '\x0', '\x0', '\x0', '\x1F', '\xF3', '\xFF',
  'a', '\x0', '\x0', '\x0', '\x19', 't', 'E', 'X',
  't', 'S', 'o', 'f', 't', 'w', 'a', 'r',
  'e', '\x0', 'A', 'd', 'o', 'b', 'e', '\x20',
  'I', 'm', 'a', 'g', 'e', 'R', 'e', 'a',
  'd', 'y', 'q', '\xC9', 'e', '\x3C', '\x0', '\x0',
  '\x1', '\xCD', 'I', 'D', 'A', 'T', 'x', '\xDA',
  '\x94', '\x93', '9', 'H', '\x3', 'A', '\x14', '\x86',
  '\xFF', '\x5D', 'b', '\xA7', '\x4', 'R', '\xC4', 'm',
  '\x22', '\x1E', '\xA0', 'F', '\x24', '\x8', '\x16', '\x16',
  'v', '\xA', '6', '\xBA', 'J', '\x9A', '\x80', '\x8',
  'A', '\xB4', 'q', '\x85', 'X', '\x89', 'G', '\xB0',
  'I', '\xA9', 'Q', '\x24', '\xCD', '\xA6', '\x8', '\xA4',
  'H', 'c', '\x91', 'B', '\xB', '\xAF', 'V', '\xC1',
  'F', '\xB4', '\x15', '\xCF', '\x22', 'X', '\x98', '\xB',
  'T', 'H', '\x8A', 'd', '\x93', '\x8D', '\xFB', 'F',
  'g', '\xC9', '\x1A', '\x14', '\x7D', '\xF0', 'f', 'v',
  'f', '\xDF', '\x7C', '\xEF', '\xE7', 'g', 'F', '\xA8',
  '\xD5', 'j', 'H', '\x24', '\x12', '\x2A', '\x0', '\x5',
  '\xBF', 'G', '\xD4', '\xEF', '\xF7', '\x2F', '6', '\xEC',
  '\x12', '\x20', '\x1E', '\x8F', '\xD7', '\xAA', '\xD5', '\xEA',
  '\xAF', 'I', '5', 'F', '\xAA', 'T', '\x5F', '\x9F',
  '\x22', 'A', '\x2A', '\x95', '\xA', '\x83', '\xE5', 'r',
  '9', 'd', '\xB3', 'Y', '\x96', '\x99', 'L', '\x6',
  '\xE9', 't', '\x9A', '\x25', '\x85', '\x2C', '\xCB', 'T',
  '\xA7', '\xC4', 'b', '1', '\xB5', '\x5E', '\x0', '\x3',
  'h', '\x9A', '\xC6', '\x16', '\x82', '\x20', 'X', 'R',
  '\x14', 'E', '6', 'S', '\x94', '\xCB', 'e', 'x',
  '\xBD', '\x5E', '\xAA', 'U', 'T', '\x23', 'L', '\xC0',
  '\xE0', '\xE2', '\xC1', '\x8F', '\x0', '\x9E', '\xBC', '\x9',
  'A', '\x7C', '\x3E', '\x1F', '\x83', 'D', '\x22', '\x11',
  '\xD5', 'T', '\x40', '\x3F', '8', '\x80', 'w', '\xE5',
  '3', '\x7', '\xB8', '\x5C', '\x2E', 'H', '\x92', '\x4',
  '\x87', '\xC3', '\x81', '\x40', '\x20', '\x40', 'g', '\x98',
  '\xE9', '6', '\x1A', '\xA6', 'g', '\x15', '\x4', '\xE3',
  '\xD7', '\xC8', '\xBD', '\x15', '\xE1', 'i', '\xB7', 'C',
  '\xAB', '\xEA', 'x', '\x2F', 'j', 'X', '\x92', '\xBB',
  '\x18', '\x20', '\x9F', '\xCF', '3', '\xC3', '\xB8', '\xE9',
  'N', '\xA7', '\xD3', 'l', 'J', '\x0', 'i', '6',
  '\x7C', '\x8E', '\xE1', '\xFE', 'V', '\x84', '\xE7', '\x3C',
  '\x9F', 'r', '\x2B', '\x3A', 'B', '\x7B', '7', 'f',
  'w', '\xAE', '\x8E', '\xE', '\xF3', '\xBD', 'R', '\xA9',
  'd', '\x2', 'B', '\xAF', '\x85', '2', 'f', 'F',
  '\xBA', '\xC', '\xD9', '\x9F', '\x1D', '\x9A', 'l', '\x22',
  '\xE6', '\xC7', '\x3A', '\x2C', '\x80', '\xEF', '\xC1', '\x15',
  '\x90', '\x7', '\x93', '\xA2', '\x28', '\xA0', 'S', 'j',
  '\xB1', '\xB8', '\xDF', '\x29', '5', 'C', '\xE', '\x3F',
  'X', '\xFC', '\x98', '\xDA', 'y', 'j', 'P', '\x40',
  '\x0', '\x87', '\xAE', '\x1B', '\x17', 'B', '\xB4', '\x3A',
  '\x3F', '\xBE', 'y', '\xC7', '\xA', '\x26', '\xB6', '\xEE',
  '\xD9', '\x9A', '\x60', '\x14', '\x93', '\xDB', '\x8F', '\xD',
  '\xA', '\x2E', '\xE9', '\x23', '\x95', '\x29', 'X', '\x0',
  '\x27', '\xEB', 'n', 'V', 'p', '\xBC', '\xD6', '\xCB',
  '\xD6', 'G', '\xAB', '\x3D', 'l', '\x7D', '\xB8', '\xD2',
  '\xDD', '\xA0', '\x60', '\x83', '\xBA', '\xEF', '\x5F', '\xA4',
  '\xEA', '\xCC', '\x2', 'N', '\xAE', '\x5E', 'p', '\x1A',
  '\xEC', '\xB3', '\x40', '9', '\xAC', '\xFE', '\xF2', '\x91',
  '\x89', 'g', '\x91', '\x85', '\x21', '\xA8', '\x87', '\xB7',
  'X', '\x7E', '\x7E', '\x85', '\xBB', '\xCD', 'N', 'N',
  'b', 't', '\x40', '\xFA', '\x93', '\x89', '\xEC', '\x1E',
  '\xEC', '\x86', '\x2', 'H', '\x26', '\x93', '\xD0', 'u',
  '\x1D', '\x7F', '\x9', '2', '\x95', '\xBF', '\x1F', '\xDB',
  '\xD7', 'c', '\x8A', '\x1A', '\xF7', '\x5C', '\xC1', '\xFF',
  '\x22', 'J', '\xC3', '\x87', '\x0', '\x3', '\x0', 'K',
  '\xBB', '\xF8', '\xD6', '\x2A', 'v', '\x98', 'I', '\x0',
  '\x0', '\x0', '\x0', 'I', 'E', 'N', 'D', '\xAE',
  'B', '\x60', '\x82',
};