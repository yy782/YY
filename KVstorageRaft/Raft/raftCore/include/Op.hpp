#include <string>
#include <sstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/string.hpp>

namespace yy
{
namespace raft 
{
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
extern const int minRandomizedElectionTime;
extern const int maxRandomizedElectionTime;
const std::string OK = "OK";
const std::string ErrNoKey = "ErrNoKey";
const std::string ErrWrongLeader = "ErrWrongLeader";
std::chrono::milliseconds getRandomizedElectionTimeout(){
  std::random_device rd;
  std::mt19937 rng(rd());
  std::uniform_int_distribution<int> dist(minRandomizedElectionTime, maxRandomizedElectionTime);
  return std::chrono::milliseconds(dist(rng));
}
void sleepNMilliseconds(int N){ 
  std::this_thread::sleep_for(std::chrono::milliseconds(N)); 
};

const bool Debug = true;

const int debugMul = 1;  // 时间单位：time.Millisecond，不同网络环境rpc速度不同，因此需要乘以一个系数
const int HeartBeatTimeout = 25 * debugMul;  // 心跳时间一般要比选举超时小一个数量级
const int ApplyInterval = 10 * debugMul;     //

const int minRandomizedElectionTime = 300 * debugMul;  // ms
const int maxRandomizedElectionTime = 500 * debugMul;  // ms

const int CONSENSUS_TIMEOUT = 500 * debugMul;  // ms

// 协程相关设置

const int FIBER_THREAD_NUM = 1;              // 协程库中线程池大小
const bool FIBER_USE_CALLER_THREAD = false;  // 是否使用caller_thread执行调度任务

}
}