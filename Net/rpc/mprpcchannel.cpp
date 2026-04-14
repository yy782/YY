#include "mprpcchannel.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <string>
#include "mprpccontroller.h"
#include "rpcheader.pb.h"
#include "util.hpp"
namespace yy
{
namespace net
{
namespace rpc
{
void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                              google::protobuf::RpcController* controller, const google::protobuf::Message* request,
                              google::protobuf::Message* response, google::protobuf::Closure* done) {
  if (!client_->isConnected()) {
    controller->SetFailed("not connected!");
    return; 
  }
  const google::protobuf::ServiceDescriptor* sd = method->service();
  std::string service_name = sd->name();     // service_name
  std::string method_name = method->name();  // method_name

  // 获取参数的序列化字符串长度 args_size
  uint32_t args_size{};
  std::string args_str;
  if (request->SerializeToString(&args_str)) {
    args_size = args_str.size();
  } else {
    controller->SetFailed("serialize request error!");
    return;
  }
  RPC::RpcHeader rpcHeader;
  rpcHeader.set_service_name(service_name);
  rpcHeader.set_method_name(method_name);
  rpcHeader.set_args_size(args_size);

  std::string rpc_header_str;
  if (!rpcHeader.SerializeToString(&rpc_header_str)) {
    controller->SetFailed("serialize rpc header error!");
    return;
  }

  // 使用protobuf的CodedOutputStream来构建发送的数据流
  std::string send_rpc_str;  // 用来存储最终发送的数据
  {
    // 创建一个StringOutputStream用于写入send_rpc_str
    google::protobuf::io::StringOutputStream string_output(&send_rpc_str);
    google::protobuf::io::CodedOutputStream coded_output(&string_output);

    // 先写入header的长度（变长编码）
    coded_output.WriteVarint32(static_cast<uint32_t>(rpc_header_str.size()));

    // 不需要手动写入header_size，因为上面的WriteVarint32已经包含了header的长度信息
    // 然后写入rpc_header本身
    coded_output.WriteString(rpc_header_str);
  }

  // 最后，将请求参数附加到send_rpc_str后面
  send_rpc_str += args_str;

  client_->getConnection()->send(std::move(send_rpc_str));
  

  return AsyncRecv(controller, response);
}
yy::cppcoro::task<void> MprpcChannel::AsyncRecv(google::protobuf::RpcController *controller,
                  google::protobuf::Message *response)
{
  // 挂起当前协程，等待数据到达
  auto& buf = client_->getConnection()->recvBuffer();
  if (!response->ParseFromArray(buf.data(), buf.size())) {
    char errtxt[1050] = {0};
    sprintf(errtxt, "parse error! response_str:%s", buf.data());
    controller->SetFailed(errtxt);
    return;
  }  
}

MprpcChannel::MprpcChannel(const std::string& ip, short port, yy::net::EventLoop* loop) : 
addr_(ip.c_str(),port),
client_(std::make_shared<TcpClient>(addr_, loop))
{
  client_->setConnectionCallback([this](int Cfd,const Address& Caddr,EventLoop* Cloop){
    auto conn = std::make_shared<TcpConnection>(Cfd,Caddr,Cloop);
    conn->setMessageCallback([this](TcpConnectionPtr conn){
      // 恢复协程，继续处理数据
    });
    return conn;
  });
  client_->enableRetry();
  client_->connect();
}
}
}
}