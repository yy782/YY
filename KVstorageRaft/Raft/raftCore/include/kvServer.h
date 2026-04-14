#ifndef SKIP_LIST_ON_RAFT_KVSERVER_H
#define SKIP_LIST_ON_RAFT_KVSERVER_H

#include <boost/any.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/foreach.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/vector.hpp>
#include <iostream>
#include <mutex>
#include <unordered_map>
#include "kvServerRPC.pb.h"
#include "raft.h"
#include "skipList.h"
namespace yy
{
namespace raft
{
class KvServer : raftKVRpcProctoc::kvServerRpc {
 private:
  std::mutex m_mtx;
  int id_;
  std::shared_ptr<Raft> raftNode_;

  std::shared_ptr<LockQueue<ApplyMsg> > applyChan_;
  int maxRaftState_;
  std::string serializedKVData_;
  SkipList<std::string, std::string> skipList_;

  /**
   * @brief KV数据库，使用哈希表存储
   * @details
   * 这是一个备用的存储结构，目前主要使用m_skipList
   * std::unordered_map提供O(1)的平均查找时间
   *
   * 为什么同时有m_skipList和m_kvDB：
   * - m_skipList：主要存储，支持范围查询
   * - m_kvDB：备用存储，可能用于其他目的
   */
  std::unordered_map<std::string, std::string> kvDB_;

  /**
   * @brief 等待特定索引操作完成的管道
   * @details
   * 这是一个映射表，键是Raft日志索引，值是操作管道
   *
   * 工作流程：
   * 1. 客户端发送请求，KVServer调用Raft::Start()
   * 2. Raft::Start()返回日志索引index
   * 3. KVServer创建一个管道waitApplyCh[index]
   * 4. KVServer等待管道中的操作完成
   * 5. Raft达成共识后，通过applyCh通知KVServer
   * 6. KVServer从applyCh读取操作，找到对应的waitApplyCh[index]
   * 7. KVServer将操作结果放入管道，唤醒等待的客户端请求
   *
   * 为什么需要waitApplyCh：
   * - 客户端请求需要等待操作完成才能返回结果
   * - Raft是异步的，提交操作后不会立即完成
   * - 需要一种机制来通知客户端请求操作已完成
   */
  std::unordered_map<int, LockQueue<Op> *> waitApplyCh;

  /**
   * @brief 记录每个客户端的最后请求ID，用于去重
   * @details
   * 键：客户端ID（ClientId）
   * 值：最后请求的ID（RequestId）
   *
   * 为什么需要去重：
   * - 网络不稳定可能导致客户端重试请求
   * - 如果不检查，同一个请求可能被执行多次
   * - 例如：客户端发送PUT(key, value)，网络超时，客户端重试
   *   如果不检查，key的值可能被设置两次
   *
   * 去重机制：
   * 1. 每个客户端有一个唯一的ClientId
   * 2. 每个请求有一个递增的RequestId
   * 3. 服务器记录每个客户端的最后RequestId
   * 4. 如果收到的RequestId <= 记录的RequestId，说明是重复请求
   * 5. 对于重复请求，直接返回之前的结果，不执行操作
   *
   * 示例：
   * - 客户端A发送请求1：PUT(key1, value1)
   * - 服务器记录：m_lastRequestId[A] = 1
   * - 客户端A重试请求1：PUT(key1, value1)
   * - 服务器检查：RequestId(1) <= m_lastRequestId[A](1)，重复请求
   * - 服务器返回之前的结果，不执行操作
   */
  std::unordered_map<std::string, int> lastRequestId_;

  /**
   * @brief 最后快照的Raft日志索引
   * @details
   * 记录最后一次制作快照时的Raft日志索引
   * 用于判断是否需要制作新的快照
   *
   * 快照制作时机：
   * - 当Raft日志大小超过m_maxRaftState时
   * - 检查当前日志索引与m_lastSnapShotRaftLogIndex的差值
   * - 如果差值超过阈值，则制作快照
   */
  int lastSnapShotRaftLogIndex_;

 public:
  KvServer() = delete;
  KvServer(int me, int maxraftstate, std::string nodeInforFileName, short port);
  void StartKVServer();
  void DprintfKVDB();

  void ExecuteAppendOpOnKVDB(Op op);
  bool tryGetOpOnKVDB(Op op, std::string *value);
  void ExecutePutOpOnKVDB(Op op);

  void Get(const raftKVRpcProctoc::GetArgs *args,
           raftKVRpcProctoc::GetReply
               *reply);

  void GetCommandFromRaft(ApplyMsg message);

  bool ifRequestDuplicate(std::string ClientId, int RequestId);

  void PutAppend(const raftKVRpcProctoc::PutAppendArgs *args, raftKVRpcProctoc::PutAppendReply *reply);

  /**
   * @brief 从Raft读取applyCh命令的后台线程
   * @details
   *
   * 这是KVServer的核心线程，不断从applyCh读取消息
   *
   * 工作流程：
   * 1. 无限循环，不断从applyCh读取消息
   * 2. 如果收到CommandValid=true的消息：
   *    - 调用GetCommandFromRaft处理
   * 3. 如果收到SnapshotValid=true的消息：
   *    - 调用GetSnapShotFromRaft处理快照
   *
   * 为什么需要独立线程：
   * - applyCh是异步的，需要不断读取
   * - 不能阻塞RPC处理线程
   * - 需要独立的后台线程来处理
   */
  void ReadRaftApplyCommandLoop();

  void ReadSnapShotToInstall(std::string snapshot);

  bool SendMessageToWaitChan(const Op &op, int raftIndex);

  void CheckAndSendSnapShotCommand(int raftIndex, int proportion);

  void GetSnapShotFromRaft(ApplyMsg message);

  std::string MakeSnapShot();

 public:  // for rpc

  void PutAppend(google::protobuf::RpcController *controller, const ::raftKVRpcProctoc::PutAppendArgs *request,
                 ::raftKVRpcProctoc::PutAppendReply *response, ::google::protobuf::Closure *done) override;

  void Get(google::protobuf::RpcController *controller, const ::raftKVRpcProctoc::GetArgs *request,
           ::raftKVRpcProctoc::GetReply *response, ::google::protobuf::Closure *done) override;

 private:
  friend class boost::serialization::access;

  template <class Archive>
  void serialize(Archive &ar, const unsigned int version)
  {
    ar &serializedKVData_;
    ar &m_lastRequestId;
  }

  /**
   * @brief 获取快照数据
   * @return 快照数据（字符串形式）
   * @details
   *
   * 快照制作流程：
   * 1. 调用m_skipList.dump_file()将跳表内容序列化为字符串
   * 2. 将字符串保存到m_serializedKVData
   * 3. 使用Boost.Serialization序列化m_serializedKVData和m_lastRequestId
   * 4. 返回序列化后的字符串
   *
   * 为什么需要两步序列化：
   * - m_skipList有自己的序列化格式
   * - 其他数据使用Boost.Serialization序列化
   * - 需要先将m_skipList序列化为字符串，再与其他数据一起序列化
   */
  std::string getSnapshotData() {
    serializedKVData_ = skipList_.dump_file();
    std::stringstream ss;
    boost::archive::text_oarchive oa(ss);
    oa << *this;
    serializedKVData_.clear();
    return ss.str();
  }

  /**
   * @brief 从字符串解析快照数据
   * @param str 快照数据（字符串形式）
   * @details
   *
   * 快照解析流程：
   * 1. 使用Boost.Serialization反序列化m_serializedKVData和m_lastRequestId
   * 2. 调用m_skipList.load_file()从m_serializedKVData恢复跳表内容
   * 3. 清空m_serializedKVData
   *
   * 这是getSnapshotData()的逆操作
   */
  void parseFromString(const std::string &str) {
    std::stringstream ss(str);
    boost::archive::text_iarchive ia(ss);
    ia >> *this;
    skipList_.load_file(serializedKVData_);
    serializedKVData_.clear();
  }

  /////////////////serialiazation end ///////////////////////////////
};
}
}  // namespace yy
#endif  // SKIP_LIST_ON_RAFT_KVSERVER_H
