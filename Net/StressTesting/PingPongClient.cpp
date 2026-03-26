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
using namespace yy;
using namespace yy::net;
//./PingPongClient 4 4096 100 60 
class Client;

class Session : noncopyable
{
 public:
  Session(EventLoop* loop,
          const Address& serverAddr,
          Client* owner)
    : client_(serverAddr,loop),
      owner_(owner),
      bytesRead_(0),
      bytesWritten_(0),
      messagesRead_(0)
  {
    client_.setConnectionCallback(
        std::bind(&Session::onConnection,this,_1));
    client_.setMessageCallBack(
        std::bind(&Session::onMessage,this,_1));
    client_.setCloseCallBack(bind(&Session::handleClose,this,_1));
    client_.setErrorCallback([](TcpConnectionPtr con){
        LOG_SYSTEM_ERROR(con->getAddr().sockaddrToString()<<" errno:"<<errno);
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
  void handleClose(TcpConnectionPtr);
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
    auto& buf=conn->getRecvBuffer();
    auto size=buf.get_readable_size();
    assert(size==4096);
    ++messagesRead_;
    bytesRead_+=size;
    bytesWritten_+=size;
    conn->send(buf.retrieve(size),size);      
    assert(buf.get_readable_size()==0);
  }


  TcpClient client_;
  Client* owner_;
  int64_t bytesRead_;
  int64_t bytesWritten_;
  int64_t messagesRead_;
};

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
      threadPool_(threadCount,loop),
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
      Session* session = new Session(threadPool_.getEventLoop(), serverAddr,this);
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
  EventLoopThreadPool threadPool_;
  int sessionCount_;
  int timeout_;
  std::vector<std::unique_ptr<Session>> sessions_;
  std::string message_;
  std::atomic<int> numConnected_;
};

void Session::onConnection(const TcpConnectionPtr& conn)
{
    conn->setEvent(LogicEvent::Read|LogicEvent::Edge);
    //conn->setReading();
    conn->setTcpNoDelay(true);
    const std::string& msg=owner_->message();
   
    conn->send(msg.c_str(),msg.size());
    owner_->onConnect();
}
void Session::handleClose(TcpConnectionPtr)
{
    owner_->onDisconnect();
}
int main(int argc, char* argv[])
{
    // SyncLog::getInstance("../CliLog.log").getFilter() 
    //   .set_global_level(LOG_LEVEL_DEBUG) 
    //   .set_module_enabled("TCP")
    //   .set_module_enabled("SYSTEM")

    //   ;
    int threadCount;
    int blockSize;
    int sessionCount ;
    int timeout ;
    if(argc<5)
    {
      threadCount = 1;
      blockSize = 4096;
      sessionCount =1;
      timeout = 10;        
    }
    else
    {
      threadCount = atoi(argv[1]);
      blockSize = atoi(argv[2]);
      sessionCount = atoi(argv[3]);
      timeout = atoi(argv[4]);
    } 


    EventLoop loop;
    Address serverAddr("0.0.0.0",8080);

    Client client(&loop, serverAddr, blockSize, sessionCount, timeout, threadCount);
    loop.loop();
    
}
