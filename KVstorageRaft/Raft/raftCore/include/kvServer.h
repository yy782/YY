/**
 * @file kvServer.h
 * @brief KVServer类定义 - 基于Raft的分布式KV存储服务器
 * @details
 *
 * KVServer是分布式KV存储系统的上层应用，它使用Raft一致性算法来保证数据的一致性
 *
 * 分布式KV存储基础知识：
 * - KV存储：键值对存储系统，支持PUT、GET、APPEND等操作
 * - 分布式系统：多个节点协同工作，提供高可用性和容错能力
 * - 一致性：所有节点看到相同的数据视图
 * - 容错性：部分节点故障时系统仍能正常工作
 *
 * KVServer的主要功能：
 * 1. 提供KV存储接口：PUT、GET、APPEND操作
 * 2. 使用Raft算法保证数据一致性
 * 3. 处理客户端请求，通过Raft达成共识
 * 4. 实现快照机制，减少日志大小
 * 5. 处理请求去重，避免重复执行
 *
 * KVServer与Raft的关系：
 * - KVServer是上层应用，Raft是底层一致性算法
 * - KVServer通过applyCh管道从Raft获取提交的命令
 * - KVServer将客户端请求转换为Raft日志条目
 * - Raft保证所有节点以相同顺序执行相同的命令
 *
 * 架构设计：
 * - 客户端 -> Clerk -> KVServer -> Raft -> 持久化存储
 * - Clerk：客户端库，负责请求路由和重试
 * - KVServer：服务器端，处理请求并调用Raft
 * - Raft：一致性算法，保证数据一致性
 * - 持久化存储：保存Raft状态和快照
 */

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
/**
 * @class KvServer
 * @brief 基于Raft的分布式KV存储服务器
 * @details
 *
 * KvServer是分布式KV存储系统的核心组件，它：
 * 1. 继承自raftKVRpcProctoc::kvServerRpc，提供RPC接口
 * 2. 使用Raft一致性算法保证数据一致性
 * 3. 使用跳表（SkipList）作为底层存储结构
 * 4. 实现快照机制，减少日志大小
 * 5. 处理请求去重，避免重复执行
 *
 * 工作流程：
 * 1. 客户端通过RPC发送PUT/GET/APPEND请求
 * 2. KVServer将请求转换为Op操作
 * 3. KVServer调用Raft::Start()将操作提交到Raft日志
 * 4. Raft达成共识后，通过applyCh通知KVServer
 * 5. KVServer从applyCh读取提交的操作，应用到状态机
 * 6. KVServer返回结果给客户端
 *
 * 关键设计：
 * - applyCh：KVServer和Raft之间的通信管道
 * - waitApplyCh：等待特定索引的操作完成
 * - m_lastRequestId：记录每个客户端的最后请求ID，用于去重
 * - m_skipList：跳表存储，提供高效的键值对操作
 */
class KvServer : raftKVRpcProctoc::kvServerRpc {
 private:
  /**
   * @brief 互斥锁，保护共享数据的并发访问
   * @details
   * 保护以下共享数据：
   * - m_kvDB：KV数据库
   * - m_lastRequestId：客户端请求ID映射
   * - m_skipList：跳表存储
   * - m_lastSnapShotRaftLogIndex：最后快照的Raft日志索引
   */
  std::mutex m_mtx;

  /**
   * @brief 当前节点的ID
   * @details
   * 每个节点都有一个唯一的ID，用于标识自己
   * 在集群初始化时分配，从0开始递增
   */
  int m_me;

  /**
   * @brief Raft节点指针
   * @details
   * 指向当前节点对应的Raft实例
   * KVServer通过这个指针调用Raft的方法：
   * - Start()：提交操作到Raft日志
   * - GetState()：获取Raft状态
   * - Snapshot()：制作快照
   */
  std::shared_ptr<Raft> m_raftNode;

  /**
   * @brief KVServer和Raft节点的通信管道
   * @details
   * 这是一个无锁队列（LockQueue），用于在KVServer和Raft之间传递消息
   *
   * 消息流向：
   * Raft -> applyCh -> KVServer
   *
   * 消息类型（ApplyMsg）：
   * - CommandValid=true：普通命令（PUT/GET/APPEND）
   * - SnapshotValid=true：快照数据
   *
   * 工作流程：
   * 1. Raft达成共识后，将提交的命令放入applyCh
   * 2. KVServer从applyCh读取命令
   * 3. KVServer将命令应用到状态机（m_skipList）
   * 4. KVServer通知等待的客户端请求
   */
  std::shared_ptr<LockQueue<ApplyMsg> > applyChan;

  /**
   * @brief 快照阈值，当日志大小超过此值时制作快照
   * @details
   * Raft日志会不断增长，为了减少日志大小，需要定期制作快照
   *
   * 快照的作用：
   * - 减少日志大小，提高性能
   * - 加速节点恢复，减少日志重放时间
   * - 防止日志无限增长
   *
   * 制作快照的时机：
   * - 当Raft状态大小超过m_maxRaftState时
   * - 由KVServer主动触发
   *
   * 快照内容：
   * - KV数据库的当前状态（m_skipList的内容）
   - 最后应用的Raft日志索引
   * 客户端请求ID映射（m_lastRequestId）
   */
  int m_maxRaftState;

  /**
   * @brief 序列化后的KV数据
   * @details
   * 用于快照的序列化和反序列化
   * 理论上可以不用，但为了简化实现，暂时保留
   *
   * 序列化流程：
   * 1. m_skipList.dump_file()：将跳表内容序列化为字符串
   * 2. boost::archive：将其他数据序列化
   * 3. 合并为完整的快照数据
   *
   * 反序列化流程：
   * 1. boost::archive：反序列化其他数据
   * 2. m_skipList.load_file()：从字符串恢复跳表内容
   */
  std::string m_serializedKVData;

  /**
   * @brief 跳表存储，用于存储键值对
   * @details
   * 跳表是一种概率数据结构，提供高效的查找、插入、删除操作
   * 时间复杂度：O(log n)，与平衡树相当，但实现更简单
   *
   * 跳表的特点：
   * - 支持范围查询
   * - 并发友好（通过锁实现）
   * - 内存效率高
   * - 实现简单
   *
   * 为什么使用跳表而不是红黑树：
   * - 跳表更容易实现并发控制
   * - 跳表的查找效率与红黑树相当
   * - 跳表的内存占用更少
   */
  SkipList<std::string, std::string> m_skipList;

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
  std::unordered_map<std::string, std::string> m_kvDB;

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
  std::unordered_map<std::string, int> m_lastRequestId;

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
  int m_lastSnapShotRaftLogIndex;

 public:
  KvServer() = delete;

  /**
   * @brief 构造函数
   * @param me 当前节点的ID
   * @param maxraftstate 快照阈值，当日志大小超过此值时制作快照
   * @param nodeInforFileName 节点信息文件名，包含集群中所有节点的地址信息
   * @param port 当前节点的RPC服务端口
   * @details
   *
   * 初始化步骤：
   * 1. 初始化成员变量
   * 2. 创建Raft节点
   * 3. 创建applyCh管道
   * 4. 启动后台线程，从applyCh读取命令
   *
   * 节点信息文件格式：
   * 每行一个节点，格式为：ip:port
   * 例如：
   * 127.0.0.1:5000
   * 127.0.0.1:5001
   * 127.0.0.1:5002
   */
  KvServer(int me, int maxraftstate, std::string nodeInforFileName, short port);

  /**
   * @brief 启动KVServer
   * @details
   *
   * 启动步骤：
   * 1. 启动RPC服务，监听客户端请求
   * 2. 启动后台线程，从applyCh读取命令
   * 3. 开始处理客户端请求
   */
  void StartKVServer();

  /**
   * @brief 打印KV数据库的内容（调试用）
   * @details
   * 遍历m_skipList，打印所有键值对
   * 用于调试和验证数据正确性
   */
  void DprintfKVDB();

  /**
   * @brief 在KV数据库上执行APPEND操作
   * @param op 要执行的操作
   * @details
   *
   * APPEND操作：
   * - 将value追加到key对应的值后面
   * - 如果key不存在，相当于PUT操作
   *
   * 示例：
   * - 初始状态：key1 = "hello"
   * - APPEND(key1, " world") -> key1 = "hello world"
   * - APPEND(key2, "test") -> key2 = "test"（key2不存在，相当于PUT）
   */
  void ExecuteAppendOpOnKVDB(Op op);

  /**
   * @brief 在KV数据库上执行GET操作
   * @param op 要执行的操作
   * @param value 输出参数，存储查询到的值
   * @param exist 输出参数，表示key是否存在
   * @details
   *
   * GET操作：
   * - 从m_skipList中查询key对应的值
   * - 如果key存在，设置value和exist=true
   * - 如果key不存在，设置exist=false
   */
  void ExecuteGetOpOnKVDB(Op op, std::string *value, bool *exist);

  /**
   * @brief 在KV数据库上执行PUT操作
   * @param op 要执行的操作
   * @details
   *
   * PUT操作：
   * - 将key设置为value
   * - 如果key已存在，覆盖旧值
   * - 如果key不存在，创建新键值对
   */
  void ExecutePutOpOnKVDB(Op op);

  /**
   * @brief 处理GET请求（RPC接口）
   * @param args GET请求参数，包含key、ClientId、RequestId
   * @param reply GET响应，包含查询结果、错误信息
   * @details
   *
   * GET请求处理流程：
   * 1. 检查请求是否重复（通过ClientId和RequestId）
   * 2. 如果是重复请求，直接返回之前的结果
   * 3. 如果不是重复请求，创建Op操作
   * 4. 调用Raft::Start()将操作提交到Raft日志
   * 5. 等待操作完成（通过waitApplyCh）
   * 6. 返回查询结果给客户端
   *
   * 注意：
   * - GET操作不会修改数据，但仍需要通过Raft达成共识
   * - 这是为了保证GET操作看到的是已提交的数据
   * - 如果不通过Raft，GET可能看到未提交的数据
   */
  void Get(const raftKVRpcProctoc::GetArgs *args,
           raftKVRpcProctoc::GetReply
               *reply);

  /**
   * @brief 从Raft节点获取消息（不是执行GET命令）
   * @param message 从applyCh读取的消息
   * @details
   *
   * 这个方法是后台线程的主循环，不断从applyCh读取消息
   *
   * 消息类型：
   * 1. CommandValid=true：普通命令（PUT/GET/APPEND）
   *    - 解析Op操作
   *    - 检查请求是否重复
   *    - 执行操作（应用到m_skipList）
   *    - 通知等待的客户端请求
   *
   * 2. SnapshotValid=true：快照数据
   *    - 调用GetSnapShotFromRaft处理快照
   *
   * 为什么需要这个方法：
   * - Raft达成共识后，需要通知KVServer
   * - KVServer需要将命令应用到状态机
   * - 需要一个独立的线程来处理这些消息
   */
  void GetCommandFromRaft(ApplyMsg message);

  /**
   * @brief 检查请求是否重复
   * @param ClientId 客户端ID
   * @param RequestId 请求ID
   * @return true表示重复请求，false表示新请求
   * @details
   *
   * 去重逻辑：
   * 1. 从m_lastRequestId中查找ClientId对应的RequestId
   * 2. 如果找不到，说明是新请求，返回false
   * 3. 如果找到，比较RequestId
   *    - 如果RequestId <= 记录的RequestId，说明是重复请求，返回true
   *    - 如果RequestId > 记录的RequestId，说明是新请求，返回false
   *
   * 为什么使用<=而不是==：
   * - 网络可能导致请求乱序
   * - 可能收到旧的请求（RequestId较小）
   * - 使用<=可以过滤掉所有旧请求
   */
  bool ifRequestDuplicate(std::string ClientId, int RequestId);

  /**
   * @brief 处理PUT/APPEND请求（RPC接口）
   * @param args PUT/APPEND请求参数，包含key、value、op、ClientId、RequestId
   * @param reply PUT/APPEND响应，包含错误信息
   * @details
   *
   * PUT/APPEND请求处理流程：
   * 1. 检查请求是否重复（通过ClientId和RequestId）
   * 2. 如果是重复请求，直接返回之前的结果
   * 3. 如果不是重复请求，创建Op操作
   * 4. 调用Raft::Start()将操作提交到Raft日志
   * 5. 等待操作完成（通过waitApplyCh）
   * 6. 返回结果给客户端
   *
   * op参数：
   * - "Put"：执行PUT操作
   * - "Append"：执行APPEND操作
   */
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

  /**
   * @brief 读取并安装快照
   * @param snapshot 快照数据
   * @details
   *
   * 快照安装流程：
   * 1. 清空当前状态（m_skipList、m_lastRequestId等）
   * 2. 反序列化快照数据
   * 3. 恢复m_skipList的内容
   * 4. 恢复m_lastRequestId
   * 5. 更新m_lastSnapShotRaftLogIndex
   *
   * 什么时候会调用这个方法：
   * - 节点重启后，从持久化存储读取快照
   * - 领导者发送快照给跟随者
   * - 跟随者安装快照，追赶领导者
   */
  void ReadSnapShotToInstall(std::string snapshot);

  /**
   * @brief 发送消息到等待管道
   * @param op 要发送的操作
   * @param raftIndex Raft日志索引
   * @return true表示发送成功，false表示发送失败
   * @details
   *
   * 这个方法用于通知等待的客户端请求操作已完成
   *
   * 工作流程：
   * 1. 从waitApplyCh中查找raftIndex对应的管道
   * 2. 如果找到，将op放入管道
   * 3. 如果没找到，说明没有等待的请求，忽略
   *
   * 为什么需要这个方法：
   * - 客户端请求可能超时，不再等待
   * - 此时waitApplyCh中可能没有对应的管道
   * - 需要检查管道是否存在，避免错误
   */
  bool SendMessageToWaitChan(const Op &op, int raftIndex);

  /**
   * @brief 检查是否需要制作快照，需要的话就向Raft发送快照命令
   * @param raftIndex 当前Raft日志索引
   * @param proportion 比例阈值，当日志增长超过此比例时制作快照
   * @details
   *
   * 快照制作流程：
   * 1. 计算当前日志索引与最后快照索引的差值
   * 2. 如果差值超过阈值，调用Raft::Snapshot()
   * 3. Raft会保存快照，并删除旧日志
   *
   * 为什么需要快照：
   * - Raft日志会不断增长
   * - 日志太大会影响性能
   * - 快照可以减少日志大小
   *
   * 快照内容：
   * - m_skipList的内容
   * - m_lastRequestId
   * - m_lastSnapShotRaftLogIndex
   */
  void IfNeedToSendSnapShotCommand(int raftIndex, int proportion);

  /**
   * @brief 处理从Raft收到的快照
   * @param message 从applyCh读取的快照消息
   * @details
   *
   * 快照处理流程：
   * 1. 从message中提取快照数据
   * 2. 调用ReadSnapShotToInstall安装快照
   * 3. 更新m_lastSnapShotRaftLogIndex
   *
   * 什么时候会收到快照：
   * - 领导者发现跟随者日志太旧
   * - 领导者发送快照给跟随者
   * - 跟随者通过applyCh收到快照
   */
  void GetSnapShotFromRaft(ApplyMsg message);

  /**
   * @brief 制作快照
   * @return 快照数据（字符串形式）
   * @details
   *
   * 快照制作流程：
   * 1. 序列化m_skipList的内容
   * 2. 序列化m_lastRequestId
   * 3. 合并为完整的快照数据
   * 4. 返回快照数据
   *
   * 序列化方法：
   * - m_skipList.dump_file()：将跳表内容序列化为字符串
   * - boost::archive：序列化其他数据
   */
  std::string MakeSnapShot();

 public:  // for rpc
  /**
   * @brief 处理PUT/APPEND请求（RPC接口实现）
   * @param controller RPC控制器
   * @param request PUT/APPEND请求参数
   * @param response PUT/APPEND响应
   * @param done RPC完成回调
   * @details
   *
   * 这是Protobuf生成的RPC接口实现
   * 内部调用PutAppend方法处理请求
   */
  void PutAppend(google::protobuf::RpcController *controller, const ::raftKVRpcProctoc::PutAppendArgs *request,
                 ::raftKVRpcProctoc::PutAppendReply *response, ::google::protobuf::Closure *done) override;

  /**
   * @brief 处理GET请求（RPC接口实现）
   * @param controller RPC控制器
   * @param request GET请求参数
   * @param response GET响应
   * @param done RPC完成回调
   * @details
   *
   * 这是Protobuf生成的RPC接口实现
   * 内部调用Get方法处理请求
   */
  void Get(google::protobuf::RpcController *controller, const ::raftKVRpcProctoc::GetArgs *request,
           ::raftKVRpcProctoc::GetReply *response, ::google::protobuf::Closure *done) override;

  /////////////////serialiazation start ///////////////////////////////
  /**
   * @brief 序列化相关方法
   * @details
   *
   * 使用Boost.Serialization库实现序列化和反序列化
   * 用于快照的保存和恢复
   *
   * 序列化的数据：
   * - m_serializedKVData：序列化后的KV数据（跳表内容）
   * - m_lastRequestId：客户端请求ID映射
   *
   * 序列化流程：
   * 1. 调用m_skipList.dump_file()将跳表内容序列化为字符串
   * 2. 调用getSnapshotData()序列化其他数据
   * 3. 合并为完整的快照数据
   *
   * 反序列化流程：
   * 1. 调用parseFromString()反序列化其他数据
   * 2. 调用m_skipList.load_file()从字符串恢复跳表内容
   */
 private:
  /**
   * @brief Boost.Serialization的友元类
   * @details
   * Boost.Serialization需要访问类的私有成员
   * 通过friend声明允许访问
   */
  friend class boost::serialization::access;

  /**
   * @brief 序列化函数
   * @param ar 归档对象（输入或输出）
   * @param version 版本号
   * @details
   *
   * 这个函数定义了哪些成员变量需要序列化
   *
   * 序列化的成员：
   * - m_serializedKVData：序列化后的KV数据
   * - m_lastRequestId：客户端请求ID映射
   *
   * 为什么不序列化m_skipList：
   * - m_skipList有自己的序列化方法（dump_file/load_file）
   * - 先将m_skipList序列化到m_serializedKVData
   * - 然后序列化m_serializedKVData和m_lastRequestId
   */
  template <class Archive>
  void serialize(Archive &ar, const unsigned int version)
  {
    ar &m_serializedKVData;
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
    m_serializedKVData = m_skipList.dump_file();
    std::stringstream ss;
    boost::archive::text_oarchive oa(ss);
    oa << *this;
    m_serializedKVData.clear();
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
    m_skipList.load_file(m_serializedKVData);
    m_serializedKVData.clear();
  }

  /////////////////serialiazation end ///////////////////////////////
};
}
}  // namespace yy
#endif  // SKIP_LIST_ON_RAFT_KVSERVER_H
