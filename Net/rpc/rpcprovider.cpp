#include "rpcprovider.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <string>
#include "rpcheader.pb.h"
#include "util.hpp"

#include "LogAppender.h"
#include "InetAddress.h"
namespace yy
{
namespace net 
{
namespace rpc
{  
void RpcProvider::NotifyService(google::protobuf::Service *service) {
  ServiceInfo service_info;

  // 获取了服务对象的描述信息
  const google::protobuf::ServiceDescriptor *pserviceDesc = service->GetDescriptor();
  // 获取服务的名字
  std::string service_name = pserviceDesc->name();
  // 获取服务对象service的方法的数量
  int methodCnt = pserviceDesc->method_count();

  std::cout << "service_name:" << service_name << std::endl;

  for (int i = 0; i < methodCnt; ++i) {
    // 获取了服务对象指定下标的服务方法的描述（抽象描述） UserService   Login
    const google::protobuf::MethodDescriptor *pmethodDesc = pserviceDesc->method(i);
    std::string method_name = pmethodDesc->name();
    service_info.m_methodMap.insert({method_name, pmethodDesc});
  }
  service_info.m_service = service;
  m_serviceMap.insert({service_name, service_info});
}

// 启动rpc服务节点，开始提供rpc远程网络调用服务
void RpcProvider::Run(int nodeIndex, short port) {
  Address addr(port,true);
  std::string node = "node" + std::to_string(nodeIndex);
  BaseLogAppender outfile("Address.conf");
  std::string AppendMsg= node + " IpAddr:"+addr.sockaddrToString();
  outfile.append(AppendMsg.c_str(),AppendMsg.size());

  server_ = std::make_unique<TcpServer>(addr, 1, 4);

  server_->setConnectCallBack([this](int fd,const Address& peerAddr,EventLoop* loop){
    auto conn = TcpConnection::accept(fd,peerAddr,loop);
    OnConnection(conn);
    conn->setMessageCallBack([this](const TcpConnectionPtr& con){
      OnMessage(con);
    });
    return conn;
  });
  server_->loop();
}
void RpcProvider::OnConnection(const TcpConnectionPtr &conn) {}

void RpcProvider::OnMessage(const TcpConnectionPtr &conn) {
  // 网络上接收的远程rpc调用请求的字符流    Login args
  Buffer& buf = conn->recvBuffer();
  ProtoBufInputStream stream(&buf);
  google::protobuf::io::CodedInputStream coded_input(&stream);
  uint32_t header_size{};

  coded_input.ReadVarint32(&header_size);  // 解析header_size

  // 根据header_size读取数据头的原始字符流，反序列化数据，得到rpc请求的详细信息
  std::string rpc_header_str;
  RPC::RpcHeader rpcHeader;
  std::string service_name;
  std::string method_name;

  // 设置读取限制，不必担心数据读多
  google::protobuf::io::CodedInputStream::Limit msg_limit = coded_input.PushLimit(header_size);
  coded_input.ReadString(&rpc_header_str, header_size);
  // 恢复之前的限制，以便安全地继续读取其他数据
  coded_input.PopLimit(msg_limit);
  uint32_t args_size{};
  if (rpcHeader.ParseFromString(rpc_header_str)) {
    // 数据头反序列化成功
    service_name = rpcHeader.service_name();
    method_name = rpcHeader.method_name();
    args_size = rpcHeader.args_size();
  } else {
    // 数据头反序列化失败
    std::cout << "rpc_header_str:" << rpc_header_str << " parse error!" << std::endl;
    return;
  }

  // 获取rpc方法参数的字符流数据
  std::string args_str;
  // 直接读取args_size长度的字符串数据
  bool read_args_success = coded_input.ReadString(&args_str, args_size);

  if (!read_args_success) {
    return;
  }
  auto it = m_serviceMap.find(service_name);

  auto mit = it->second.m_methodMap.find(method_name);

  google::protobuf::Service *service = it->second.m_service;       // 获取service对象  new UserService
  const google::protobuf::MethodDescriptor *method = mit->second;  // 获取method对象  Login

  // 生成rpc方法调用的请求request和响应response参数,由于是rpc的请求，因此请求需要通过request来序列化
  google::protobuf::Message *request = service->GetRequestPrototype(method).New();
  if (!request->ParseFromString(args_str)) {
    std::cout << "request parse error, content:" << args_str << std::endl;
    return;
  }
  google::protobuf::Message *response = service->GetResponsePrototype(method).New();

  // 给下面的method方法的调用，绑定一个Closure的回调函数
  // closure是执行完本地方法之后会发生的回调，因此需要完成序列化和反向发送请求的操作
  google::protobuf::Closure *done =
      google::protobuf::NewCallback<RpcProvider, const TcpConnectionPtr &, google::protobuf::Message *>(
          this, &RpcProvider::SendRpcResponse, conn, response);
  service->CallMethod(method, nullptr, request, response, done);
}

// Closure的回调操作，用于序列化rpc的响应和网络发送,发送响应回去
void RpcProvider::SendRpcResponse(const TcpConnectionPtr &conn, google::protobuf::Message *response) {
  std::string response_str;
  if (response->SerializeToString(&response_str))  // response进行序列化
  {
    // 序列化成功后，通过网络把rpc方法执行的结果发送会rpc的调用方
    conn->send(std::move(response_str));
  } else {
    std::cout << "serialize response_str error!" << std::endl;
  }

}

RpcProvider::~RpcProvider() {
}
}
}
}
