# yy - 高性能 C++17 网络库

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Linux](https://img.shields.io/badge/Linux-3.9+-green.svg)](https://www.kernel.org/)

yy 是一个基于 epoll 的高性能 C++17 网络库，采用 **one loop per thread** 模型，支持 **千万级并发连接**。性能测试显示，HTTP 服务器在 8 核机器上可达 **37 万 QPS**。

---

## 🚀 性能数据

| 测试项 | 配置 | QPS | 吞吐量 |
|--------|------|-----|--------|
| GET /hello | 8核, 10000 连接 | **379,476** | 45.96 MB/s |
| GET / | 8核, 10000 连接 | **134,022** | 30.29 MB/s |


---

## ✨ 核心特性

### 高性能设计
- **零拷贝缓冲区**：`readv` + 栈上 64KB 临时缓冲，一次系统调用读取所有数据
- **CRTP+SFINAE 编译期多态**：替代虚函数，零开销抽象
- **无锁队列**：`MCMP无锁缓冲区` + 溢出队列，保证性能的同时保证任务不丢失
- **支持ET,LT模式**: 业务创建对象，选择对象创建方式，直接创建还是对象池

### 高并发能力
- **SO_REUSEPORT**：多 Acceptor 模式，内核级负载均衡
- **对象池**：连接对象复用，减少 new/delete 开销

### 现代 C++ 设计
- **C++17 标准**：`string_view`、`std::any`、`std::atomic`
- **RAII 资源管理**：智能指针 + 自动关闭
- **模板精度定时器**：编译期确定时间单位，零运行时开销

### 完整协议支持
- **HTTP/1.1**：完整的请求/响应解析，支持 Keep-Alive、100 Continue
- **Protobuf RPC**：基于 Protobuf 的 RPC 框架
- **UDP 协议**：类似 TCP 的编程接口，支持 `connect` 模式
- **自定义协议**：`LineCodec`、`LengthCodec` 编解码器

---

## 📦 环境要求

| 组件 | 版本 | 说明 |
|------|------|------|
| **操作系统** | Linux 3.9+ | 需要 `SO_REUSEPORT` 支持 |
| **编译器** | GCC 7.0+ / Clang 6.0+ | 需要完整 C++17 支持 |
| **Boost** | 1.83+ | 用于 `concurrent_flat_set`  |


## 📁目录结构
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

## 🎯快速开始

=======
- git clone https://github.com/yourname/yy.git
- cd yy
- mkdir build && cd build
- cmake -DCMAKE_BUILD_TYPE=Release ..
- make -j4

## 📚示例程序
所有示例位于 NetTest/ 目录：

- EchoServer/Client        基础 TCP 通信，展示 LT/ET 模式切换
- HttpServer/Client        HTTP 服务器和客户端
- ProtoBufTestServer/Client Protobuf RPC 通信
- UdpServer/Client         UDP 协议通信（接口风格类似 TCP）
- CodeTestServer/Client    自定义协议编解码器
- TimerTestServer/Client   时间轮定时器
- RetryConnectTest        自动重连机制
- CurcularQueueTest       无锁队列安全测试
## 📊性能测试
- 使用 wrk 进行压力测试：

- ./bin/HttpServer 4 4 0 1 &

### 测试 /hello 端点
wrk -c 10000 -t 8 -d 30s http://127.0.0.1:8080/hello

### 测试 / 端点
wrk -c 10000 -t 8 -d 30s http://127.0.0.1:8080/
### 🏗️ 设计亮点
#### 零拷贝缓冲区（Buffer）
使用 `readv` 实现**分散读 / 零拷贝**，一次系统调用读取全量数据，减少拷贝与系统调用开销。

```cpp
ssize_t Buffer::readFd(int fd) {
    char extrabuf[65536];
    struct iovec vec[2];
    vec[0].iov_base = beginWrite();
    vec[0].iov_len = writableBytes();
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    // 一次系统调用读取尽可能多的数据
    ssize_t n = readv(fd, vec, 2);
}
```

#### CRTP + SFINAE 接口检查
```cpp
template<class PollerTag>
class Poller {
    void poll(int timeout, HandlerList& handlers) {
        static_assert(has_poll_v<PollerTag>, "派生类必须实现 poll()");
        return static_cast<PollerTag*>(this).poll(timeout, handlers);
    }
};
```
#### 双队列任务系统
```cpp
class EventLoop {
    RingBuffer<Functor> pendingFunctors_;      // 无锁主队列
    SafeVector<Functor> overflowQueue_;        // 溢出队列
    
    void doPendingFunctors() {
        // 先处理主队列（最多 n 个）
        // 再处理溢出队列
    }
};
```
#### 对象池优化
```cpp
class ObjectPool<TcpConnectionPtr> {
    void expend(int num) {
        for (int i = 0; i < num; ++i) {
            auto conn = std::make_shared<TcpConnection>();
            conn->setMessageCallBack(config_.callback_);
            free_list_.push_back(conn);
        }
    }
    
    TcpConnectionPtr acquire(int fd, const Address& addr, EventLoop* loop) {
        auto conn = free_list_.front();
        conn->init(fd, addr, loop);
        return conn;
    }
};
```
## 📝许可证
MIT License

## 👥贡献
欢迎提交 Issue 和 Pull Request！

## 🙏 致谢与参考
本项目参考了以下优秀开源项目的设计思想：
- [muduo](https://github.com/chenshuo/muduo)：基于 Reactor 模型的经典 C++ 网络库，架构与事件驱动设计深受其影响
- [handy](https://github.com/yedf2/handy)：简洁高效的 C++ 网络库，接口设计与实践提供重要参考

yy 网络库在 muduo 的 one loop per thread 模型、handy 的简洁易用性基础上进行优化与扩展，在此深表感谢！