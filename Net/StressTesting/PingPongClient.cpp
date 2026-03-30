#include <iostream>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <iomanip>
#include <cstring>
#include <random>

#include "../EventLoop.h"
#include "../EventLoopThreadPool.h"
#include "../TcpClient.h"
#include "../TcpConnection.h"
#include "../InetAddress.h"
#include "../ConfigCenter.h"
#include "../TimerQueue.h"
#include "../SignalHandler.h"
using namespace yy;
using namespace yy::net;
//./PingPongClient 4 4096 100 60 0
//./PingPongClient 4 4096 10000 10 0
template<bool isET>
class Client;
//valgrind --leak-check=full --track-origins=yes ./PingPongClient 4 4096 1000 60 1 2>&1 | tee valgrind.out
template<bool isET>
class Session : noncopyable
{
 public:
  Session(EventLoop* loop,
          const Address& serverAddr,
          Client<isET>* owner)
    : client_(serverAddr,loop),
      owner_(owner),
      bytesRead_(0),
      bytesWritten_(0),
      messagesRead_(0)
  {
    client_.setConnectionCallback([this](int Cfd,const Address& Caddr,EventLoop* Cloop)
    {
        if constexpr (isET)
        {
            auto conn=TcpConnection::accept(Cfd,Caddr,Cloop,Event(LogicEvent::Read|LogicEvent::Edge));
            onConnection(conn);
            conn->setMessageCallBack([this](TcpConnectionPtr con){
                onMessage(con);
            });
            conn->setCloseCallBack([this](TcpConnectionPtr){
                handleClose();
            });
            conn->setErrorCallBack([](TcpConnectionPtr con){
                LOG_SYSTEM_ERROR(con->addr().sockaddrToString()<<" errno:"<<errno);
            });
            return conn;          
        }
        else 
        {
            auto conn=TcpConnection::accept(Cfd,Caddr,Cloop,Event(LogicEvent::Read));
            onConnection(conn);
            conn->setMessageCallBack([this](TcpConnectionPtr con){
                onMessage(con);
            });
            conn->setCloseCallBack([this](TcpConnectionPtr){
                handleClose();
            });
            conn->setErrorCallBack([](TcpConnectionPtr con){
                LOG_SYSTEM_ERROR(con->addr().sockaddrToString()<<" errno:"<<errno);
            });
            return conn;          
        }

    });
  }

  void start()
  {
    client_.connect();
  }

  void stop()
  {
    client_.disconnect();
  }
  void handleClose(); 
  int64_t bytesRead() const
  {
     return bytesRead_;
  }

  int64_t messagesRead() const
  {
     return messagesRead_;
  }

 private:

void onConnection(const TcpConnectionPtr& conn);

  void onMessage(const TcpConnectionPtr& conn)
  {
    auto& buf=conn->recvBuffer();
    auto size=buf.readable_size();
    assert(size==4096);
    ++messagesRead_;
    bytesRead_+=size;
    bytesWritten_+=size;
    conn->send(buf.retrieve(size),size);      
    assert(buf.readable_size()==0);
  }


  TcpClient client_;
  Client<isET>* owner_;
  int64_t bytesRead_;
  int64_t bytesWritten_;
  int64_t messagesRead_;
};
template<bool isET>
class Client : noncopyable
{
 public:
  Client(EventLoop* loop,
         const Address& serverAddr,
         int blockSize,
         int sessionCount,
         int timeout,
         int threadCount)
    : loop_(loop),
      threadPool_(threadCount),
      sessionCount_(sessionCount),
      timeout_(timeout)
  {
    loop->runTimer<LowPrecision>([this](){
        handleTimeout();
    },timeout,1);
    threadPool_.run();

    for (int i = 0; i < blockSize; ++i)
    {
      message_.push_back(static_cast<char>(i % 128));
    }

    for (int i = 0; i < sessionCount; ++i)
    {
      char buf[32];
      snprintf(buf, sizeof buf, "C%05d", i);
      Session<isET>* session = new Session<isET>(threadPool_.getEventLoop(), serverAddr,this);
      session->start();
      sessions_.emplace_back(session);
    }
  }
  ~Client()
  {
    
  }
  const std::string& message() const
  {
    return message_;
  }
  void onConnect()
  {
    
    if (++numConnected_ == sessionCount_)
    {
      std::cout<< "all connected"<<"\n";

    }
    LOG_TCP_DEBUG("连接数:"<<numConnected_);
    // if(numConnected_%250==0)
    // {
    //     std::cout<<"已连接 "<<numConnected_<<" 个客户端\n";
    // }
        
    
    //std::cout<<"已连接 "<<numConnected_<<" 个客户端\n";
    
  }

  void onDisconnect()
  {

    if(--numConnected_== 0)
    {
      std::cout<< "all disconnected"<<"\n";

      int64_t totalBytesRead = 0;
      int64_t totalMessagesRead = 0;

      for (const auto& session : sessions_)
      {
        totalBytesRead += session->bytesRead();
        totalMessagesRead += session->messagesRead();
      }

      std::cout<< totalBytesRead << " total bytes read"<<"\n";
      std::cout << totalMessagesRead << " total messages read"<<"\n";
      std::cout<< static_cast<double>(totalBytesRead) / static_cast<double>(totalMessagesRead)
               << " average message size"<<"\n";
      std::cout << static_cast<double>(totalBytesRead) / (timeout_ * 1024 * 1024)
               << " MiB/s throughput"<<"\n";
      loop_->quit();
    }
  }

private:

void handleTimeout()
{
    for (auto& session : sessions_)
    {
        session->stop();
    }
}

  EventLoop* loop_;
  std::vector<std::unique_ptr<Session<isET> >> sessions_;
  EventLoopThreadPool threadPool_;
  int sessionCount_;
  int timeout_;
  std::string message_;
  std::atomic<int> numConnected_={0};
};
template<bool isET>
void Session<isET>::onConnection(const TcpConnectionPtr& conn)
{
  if constexpr (isET)
  {
      conn->setEvent(Event(LogicEvent::Read|LogicEvent::Edge));
  }
  else 
  {
      conn->setReading();
  }
    conn->setTcpNoDelay(true);
    const std::string& msg=owner_->message();
   
    conn->send(msg.c_str(),msg.size());
    owner_->onConnect();
}
template<bool isET>
void Session<isET>::handleClose()
{
    owner_->onDisconnect();
}
int main(int argc, char* argv[])
{
    Signal::signal(SIGPIPE,[](){});
    SyncLog::getInstance("../CliLog.log").getFilter() 
      .set_global_level(LOG_LEVEL_DEBUG)
      .set_module_enabled("LOOP")
      ; 
      
      
        // SyncLog::getInstance("../CliLog.log").getFilter() 
        // .set_global_level(LOG_LEVEL_DEBUG) 
        // .set_module_enabled("TCP")
        // .set_module_enabled("SYSTEM")
        // .set_module_enabled("HTTP")
        // .set_module_enabled("CLIENT")
        // .set_module_enabled("EVENT")
        // ;
    int threadCount;
    int blockSize;
    int sessionCount ;
    int timeout ;
    bool isET=true;
    if(argc<5)
    {
      threadCount = 4;
      blockSize = 4096;
      sessionCount =100;
      timeout = 1000000;        
    }
    else
    {
      threadCount = atoi(argv[1]);
      blockSize = atoi(argv[2]);
      sessionCount = atoi(argv[3]);
      timeout = atoi(argv[4]);
      isET=std::atoi(argv[5])==1?true:false;
    } 


    EventLoop loop;
    Address serverAddr("0.0.0.0",8080);
    if(isET)
    {
      Client<true> client(&loop, serverAddr, blockSize, sessionCount, timeout, threadCount);
      loop.loop();
    }
    else 
    {
      Client<false> client(&loop, serverAddr, blockSize, sessionCount, timeout, threadCount);
      loop.loop();      
    }
    
}
