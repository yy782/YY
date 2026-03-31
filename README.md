# 一个正在开发的C++17网络库

## 环境要求

### 操作系统
- Linux 内核 **3.9** 或更高版本（需要 `SO_REUSEPORT` 支持）
- 推荐：Ubuntu 20.04+ / CentOS 8+ / Debian 11+

### 编译器
- GCC 7.0+ 或 Clang 6.0+（需要 C++17 支持）

### 依赖库
- **Boost** 1.83 或更高版本（用于 `boost::concurrent_flat_set` 等）
- **Python** 3.6 或更高版本（用于配置生成等工具）

## protobuf支持
- 使用protobuf的消息示例在
                Net/NetTest/ProtoBufTestServer.cpp
                Net/NetTest/ProtoBufTestClient.cpp    
## udp支持
- 支持udp，让udp类似tcp的使用通信

## 目录结构 
* `Net/` - 网络核心代码
  * `HTTP/` - HTTP 协议实现
  * `Poller/` - epoll 事件监听
  * `UDP/` - UDP 协议支持
  * `protobuf/` - Protobuf RPC
  * `StressTesting/` - 压力测试工具
  * `NetTest/` - 单元测试和示例
  * `include/` - 头文件目录
  * `src/` - 源文件目录
* `Common/` - 公共工具（日志、配置等）
* `ThreadPool/` - 线程池实现
* `MemoryPool/` - 内存池实现


## examples
在Net/NetTest/目录下
* CodeTestClient.cpp
  CodeTestServer.cpp 
  CodeTestLength.cpp 测试Code协议正确性和展示使用案例
* CurcularQueueTest.cpp 测试在高压环境下的无锁队列
* EchoServer.cpp
  EchoClient.cpp 展示基本(不使用Code协议解释)的通信和对config.conf的解析,以及如何选择用LT还是ET
* HttpClient.cpp    
  HttpServer.cpp 展示http通信 
* ProtoBufTestServer.cpp
  ProtoBufTestClient.cpp
  student.proto  展示protobuf通信
* RetryConnectTest.cpp 展示重连机制
* TimerTestClient.cpp 
  TimerTestServer.cpp 展示时间轮的使用 
* UdpClient.cpp
  UdpServer.cpp 展示udp通信
 