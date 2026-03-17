#include <stdio.h>  // snprintf
#include "TcpClient.h"
#include "sockets.h"
namespace yy 
{
namespace net 
{

TcpClient::TcpClient(const Address& serverAddr,EventLoop* loop):
    fd_(sockets::createTcpSocketOrDie(serverAddr.get_family())),
    addr_(serverAddr),
    loop_(loop),
    connection_(std::make_shared<TcpConnection>()),
    ConnectHandler_(fd_,loop_),
    retry_(false)
{
  ConnectHandler_.setWriteCallBack(
      std::bind(&TcpClient::newConnection, this));
}

TcpClient::~TcpClient()
{


}

void TcpClient::connect()
{
  
  int ret=sockets::connect(fd_,addr_);
  if(ret<0)
  {
    switch (errno)
    {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN:
            break;
        default:
            if(connectFailCallback_)connectFailCallback_(this);
            break;
    }   
  }
  ConnectHandler_.setWriting(); 
}

void TcpClient::disconnect()
{
  connection_->disconnect();
}



void TcpClient::newConnection()
{

  
    int err = sockets::sockfd_has_error(fd_);
    if (err!=0) // 有错误
    {
        if (connectFailCallback_)
        {
            connectFailCallback_(this);
        }
    }
    else 
    {
        connection_->init(dup(fd_),addr_,loop_);
        connection_->ConnectSuccess();
        connection_->setReading();
        if (connectedCallback_)
        {
            connectedCallback_(connection_);
        }
    }
    ConnectHandler_.cancelWriting();
}




}
}
