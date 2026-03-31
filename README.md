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


## 目录结构 
* `Net/` - 网络核心代码
  * `HTTP/` - HTTP 协议实现
  * `Poller/` - epoll 事件监听
  * `UDP/` - UDP 协议支持
  * `protobuf/` - Protobuf RPC
  * `StressTesting/` - 压力测试工具
  * `NetTest/` - 单元测试和示例
* `Common/` - 公共工具（日志、配置等）
* `ThreadPool/` - 线程池实现
* `MemoryPool/` - 内存池实现