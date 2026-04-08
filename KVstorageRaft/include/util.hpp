
#ifndef UTIL_H
#define UTIL_H

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/access.hpp>
#include <condition_variable>  // pthread_condition_t
#include <functional>
#include <iostream>
#include <mutex>  // pthread_mutex_t
#include <queue>
#include <random>
#include <sstream>
#include <thread>
// #include "config.h"

extern const int minRandomizedElectionTime;
extern const int maxRandomizedElectionTime;



template <class F>
class DeferClass 
{
 public:
  DeferClass(F&& f) : m_func(std::forward<F>(f)) {}
  DeferClass(const F& f) : m_func(f) {}
  ~DeferClass() { m_func(); }

  DeferClass(const DeferClass& e) = delete;
  DeferClass& operator=(const DeferClass& e) = delete;

 private:
  F m_func;
};

#define _CONCAT(a, b) a##b
#define _MAKE_DEFER_(line) DeferClass _CONCAT(defer_placeholder, line) = [&]()

#undef DEFER
#define DEFER _MAKE_DEFER_(__LINE__)

void DPrintf(const char *format, ...) {
#ifndef NDEBUG
    time_t now = time(nullptr);
    tm *nowtm = localtime(&now);
    va_list args;
    va_start(args, format);
    std::printf("[%d-%d-%d-%d-%d-%d] ", nowtm->tm_year + 1900, nowtm->tm_mon + 1, nowtm->tm_mday, nowtm->tm_hour,
                nowtm->tm_min, nowtm->tm_sec);
    std::vprintf(format, args);
    std::printf("\n");
    va_end(args);
#endif
}

void myAssert(bool condition, std::string message = "Assertion failed!"){
  if (!condition) {
    std::cerr << "Error: " << message << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

template <typename... Args>
std::string format(const char* format_str, Args... args) 
{
    int size_s = std::snprintf(nullptr, 0, format_str, args...) + 1; // "\0"
    assert(size_s > 0); // Ensure that the format string is valid///////////////////////////////////////////////
    auto size = static_cast<size_t>(size_s);
    std::vector<char> buf(size);
    std::snprintf(buf.data(), size, format_str, args...);
    return std::string(buf.data(), buf.data() + size - 1);  // remove '\0'
}

std::chrono::_V2::system_clock::time_point now()
{
   return std::chrono::high_resolution_clock::now(); 
}

std::chrono::milliseconds getRandomizedElectionTimeout(){
  std::random_device rd;
  std::mt19937 rng(rd());
  std::uniform_int_distribution<int> dist(minRandomizedElectionTime, maxRandomizedElectionTime);
  return std::chrono::milliseconds(dist(rng));
}
void sleepNMilliseconds(int N){ 
  std::this_thread::sleep_for(std::chrono::milliseconds(N)); 
};


// ////////////////////////异步写日志的日志队列
// read is blocking!!! LIKE  go chan
template <typename T>
class LockQueue {
 public:
  // 多个worker线程都会写日志queue
  void Push(const T& data) {
    std::lock_guard<std::mutex> lock(m_mutex);  //使用lock_gurad，即RAII的思想保证锁正确释放
    m_queue.push(data);
    m_condvariable.notify_one();
  }

  // 一个线程读日志queue，写日志文件
  T Pop() {
    std::unique_lock<std::mutex> lock(m_mutex);
    while (m_queue.empty()) {
      // 日志队列为空，线程进入wait状态
      m_condvariable.wait(lock);  //这里用unique_lock是因为lock_guard不支持解锁，而unique_lock支持
    }
    T data = m_queue.front();
    m_queue.pop();
    return data;
  }

  bool timeOutPop(int timeout, T* ResData)  // 添加一个超时时间参数，默认为 50 毫秒
  {
    std::unique_lock<std::mutex> lock(m_mutex);

    // 获取当前时间点，并计算出超时时刻
    auto now = std::chrono::system_clock::now();
    auto timeout_time = now + std::chrono::milliseconds(timeout);

    // 在超时之前，不断检查队列是否为空
    while (m_queue.empty()) {
      // 如果已经超时了，就返回一个空对象
      if (m_condvariable.wait_until(lock, timeout_time) == std::cv_status::timeout) {
        return false;
      } else {
        continue;
      }
    }
    T data = m_queue.front();
    m_queue.pop();
    *ResData = data;
    return true;
  }

 private:
  std::queue<T> m_queue;
  std::mutex m_mutex;
  std::condition_variable m_condvariable;
};

// 这个Op是kv传递给raft的command
class Op {       
 public:
  // todo
  //为了协调raftRPC中的command只设置成了string,这个的限制就是正常字符中不能包含|
  //当然后期可以换成更高级的序列化方法，比如protobuf
  std::string asString() const {
    std::stringstream ss;
    boost::archive::text_oarchive oa(ss);

    // write class instance to archive
    oa << *this;
    // close archive

    return ss.str();
  }

  bool parseFromString(std::string str) {
    std::stringstream iss(str);
    boost::archive::text_iarchive ia(iss);
    // read class state from archive
    ia >> *this;
    return true;  // todo : 解析失敗如何處理，要看一下boost庫了
  }
  friend std::ostream& operator<<(std::ostream& os, const Op& obj){
    os << "[MyClass:Operation{" + obj.Operation_ + "},Key_{" + obj.Key_ + "},Value_{" + obj.Value_ + "},ClientId_{" +
              obj.ClientId_ + "},RequestId_{" + std::to_string(obj.RequestId_) + "}";  // 在这里实现自定义的输出格式
    return os;
  }
  std::string& Operation() { return Operation_; }
  const std::string& Operation() const { return Operation_; }
  std::string& Key() { return Key_; }
  const std::string& Key() const { return Key_; }
  std::string& Value() { return Value_; }
  const std::string& Value() const { return Value_; }
  std::string& ClientId() { return ClientId_; }
  const std::string& ClientId() const { return ClientId_; }
  int& RequestId() { return RequestId_; }
  const int RequestId() const { return RequestId_; }
 private:
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& Operation;
    ar& Key_;
    ar& Value_;
    ar& ClientId_;
    ar& RequestId_;
  }
  std::string Operation_;  // "Get" "Put" "Append"
  std::string Key_;
  std::string Value_;
  std::string ClientId_;  
  int RequestId_; 
};

const std::string OK = "OK";
const std::string ErrNoKey = "ErrNoKey";
const std::string ErrWrongLeader = "ErrWrongLeader";

bool isReleasePort(unsigned short usPort) {
  int s = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(usPort);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  int ret = ::bind(s, (sockaddr *)&addr, sizeof(addr));
  if (ret != 0) {
    close(s);
    return false;
  }
  close(s);
  return true;
}

bool getReleasePort(short &port) {
  short num = 0;
  while (!isReleasePort(port) && num < 30) {
    ++port;
    ++num;
  }
  if (num >= 30) {
    port = -1;
    return false;
  }
  return true;
}

#endif  //  UTIL_H