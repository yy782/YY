/**
 * @file raft.h
 * @brief Raft一致性算法的核心实现
 * @details
 *
 * 分布式KV存储基础知识：
 * - 分布式KV存储：将数据分散存储在多个节点上，通过网络协调保持一致性
 * - 一致性：所有节点看到的数据视图相同
 * - 可用性：即使部分节点故障，系统仍能提供服务
 * - 分区容错：网络分区恢复后，系统能自动合并数据
 *
 * Raft算法基础知识：
 * - Raft是一种分布式一致性算法，用于管理日志复制
 * - 它将一致性问题分解为三个子问题：领导者选举、日志复制、安全性
 * - Raft算法易于理解，是实现分布式系统的常用算法
 *
 * Raft算法的三个核心问题：
 * 1. 领导者选举：从候选节点中选出一个领导者
 * 2. 日志复制：领导者将日志复制到跟随者节点
 * 3. 安全性：确保只有拥有完整日志的节点才能成为领导者
 *
 * Raft算法的状态：
 * - Follower（跟随者）：被动接收领导者的日志和心跳
 * - Candidate（候选者）：参与领导者选举
 * - Leader（领导者）：处理所有客户端请求，复制日志到跟随者
 *
 * Raft算法的RPC：
 * - RequestVote：请求投票，用于领导者选举
 * - AppendEntries：追加日志，用于日志复制和心跳
 * - InstallSnapshot：安装快照，用于日志压缩
 *
 * 项目架构：
 * - Raft层：实现Raft算法，处理节点间的协调
 * - KVServer层：实现KV存储，作为状态机
 * - RPC层：提供节点间的通信能力
 * - 持久层：持久化关键数据，确保节点重启后可以恢复
 */
#ifndef RAFT_H
#define RAFT_H

#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/serialization.hpp>
#include <cmath>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "ApplyMsg.h"
#include "Persister.h"
#include "boost/any.hpp"

#include "raftRpcUtil.hpp"
#include "Op.hpp"
#include "LockQueue.hpp"
#include "../../../Common/TimeStamp.h"

#include "../../../Common/locker.h"
namespace yy 
{
namespace raft 
{
/// @brief 网络状态表示
/// @details
///
/// 网络状态用于模拟网络分区等异常情况
/// - Disconnected：网络断开，节点间无法通信
/// - AppNormal：网络正常，节点间可以正常通信
///
/// todo：可以在rpc中删除该字段，实际生产中是用不到的.
constexpr int Disconnected =0;  
// 方便网络分区的时候debug，网络异常的时候为disconnected，只要网络正常就为AppNormal，防止matchIndex[]数组异常减小
constexpr int AppNormal = 1;

///////////////投票状态

/// @brief 投票状态枚举
/// @details
///
/// 投票状态用于跟踪节点在选举过程中的状态
/// - Killed：节点已停止，不参与选举
/// - Voted：本轮已经投过票了
/// - Expire：投票（消息、竞选者）过期
/// - Normal：正常状态，可以参与选举
constexpr int Killed = 0;
constexpr int Voted = 1;   //本轮已经投过票了
constexpr int Expire = 2;  //投票（消息、竞选者）过期
constexpr int Normal = 3;
class Raft : public raftRpcProctoc::raftRpc {
 private:
  std::mutex m_mtx;  // 互斥锁，保护共享数据的并发访问
  // m_peers：其他节点的RPC客户端列表
  // 每个节点都保存了其他节点的RPC客户端，用于发送RPC请求
  std::vector<std::shared_ptr<RaftRpcUtil>> m_peers;
  // m_persister：持久化对象，负责将关键数据落盘
  // Raft算法要求某些关键数据必须持久化，以确保节点重启后可以恢复
  std::shared_ptr<Persister> m_persister;
  int id_;
  int m_currentTerm;

  // m_votedFor：本轮投票给的节点ID
  // 在每个任期中，每个节点只能投票一次
  // 如果m_votedFor == -1，表示还没有投票
  // 如果m_votedFor == m_me，表示投票给自己
  // 如果m_votedFor == other，表示投票给其他节点
  int m_votedFor;
  std::vector<raftRpcProctoc::LogEntry> m_logs;  //// 日志条目数组，包含了状态机要执行的指令集，以及收到领导时的任期号
  int m_commitIndex;
  int m_lastApplied;  
  std::vector<int> m_nextIndex;  
  std::vector<int> m_matchIndex;
  TimeStamp<LowPrecision> m_lastResetElectionTime;
  TimeStamp<LowPrecision> m_lastResetHearBeatTime;

  std::shared_ptr<LockQueue<ApplyMsg>> applyChan;  // client从这里取日志（2B），client与raft通信的接口
 private: // Leader

  void CheckAndHearBeat();
  void asyncDoHeartBeat();
private:  

  /**
   * @brief 处理追加日志请求（RPC）
   * @param args 追加日志的参数
   * @param reply 追加日志的回复
   * @details
   *
   * AppendEntries RPC的作用：
   * 1. 日志复制：领导者将日志复制到跟随者
   * 2. 心跳：领导者定期发送空AppendEntries作为心跳
   * 3. 一致性检查：跟随者检查日志是否一致
   *
   * AppendEntries的参数：
   * - Term：领导者的任期号
   * - LeaderId：领导者的ID
   * - PrevLogIndex：前一个日志的索引
   * - PrevLogTerm：前一个日志的任期号
   * - Entries：要追加的日志条目数组
   * - LeaderCommit：领导者的提交索引
   *
   * AppendEntries的回复：
   * - Term：跟随者的当前任期号
   * - Success：是否成功追加日志
   *
   * 处理逻辑：
   * 1. 检查任期号，如果args->Term < m_currentTerm，拒绝
   * 2. 检查日志一致性，如果不一致，拒绝
   * 3. 追加日志，更新m_logs
   * 4. 更新m_commitIndex，如果args->LeaderCommit > m_commitIndex
   */
  void AppendEntries1(const raftRpcProctoc::AppendEntriesArgs *args, raftRpcProctoc::AppendEntriesReply *reply);

  /**
   * @brief 应用日志的定时器
   * @details
   *
   * applierTicker的作用：
   * 定期检查是否有新的日志需要应用到状态机
   * 如果m_commitIndex > m_lastApplied，说明有新的日志需要应用
   * 将这些日志封装成ApplyMsg，放入applyChan队列
   * 上层应用从队列中取出ApplyMsg，应用到状态机
   */
  void applierTicker();

  /**
   * @brief 选举超时定时器
   * @details
   *
   * 选举超时定时器的作用：
   * 1. 检查是否超时：每隔一段时间检查是否超时
   * 2. 触发选举：如果超时，调用doElection()发起选举
   * 3. 随机超时：选举超时是随机的，避免多个节点同时发起选举
   *
   * 超时的判断：
   * 1. 如果在选举超时时间内没有收到领导者的心跳，说明超时
   * 2. 如果在选举超时时间内收到了领导者的心跳，重置超时
   *
   * 睡眠时间的计算：
   * 1. 如果没有超时，睡眠到m_lastResetElectionTime + 选举超时时间
   * 2. 如果已经超时，立即发起选举
   */
  void electionTimeOutTicker();

  /**
   * @brief 获取需要应用的日志
   * @return 需要应用的日志列表
   * @details
   *
   * 这个函数用于上层应用获取需要应用的日志
   * 上层应用调用这个函数，从applyChan队列中取出ApplyMsg
   * 然后将ApplyMsg应用到状态机
   */
  std::vector<ApplyMsg> getApplyLogs();

  /**
   * @brief 获取新命令的索引
   * @return 新命令的索引
   * @details
   *
   * 当领导者收到客户端请求时，需要为新命令分配一个索引
   * 这个函数返回下一个可用的日志索引
   * 索引从1开始递增
   */
  int getNewCommandIndex();

  /**
   * @brief 获取前一个日志的信息
   * @param server 目标节点
   * @param preIndex 前一个日志的索引（输出参数）
   * @param preTerm 前一个日志的任期号（输出参数）
   * @details
   *
   * 这个函数用于日志复制时获取前一个日志的信息
   * 领导者在发送AppendEntries时，需要知道前一个日志的索引和任期号
   * 跟随者通过这些信息检查日志是否一致
   */
  void getPrevLogInfo(int server, int *preIndex, int *preTerm);

  /**
   * @brief 获取Raft状态
   * @param term 当前任期号（输出参数）
   * @param isLeader 是否是领导者（输出参数）
   * @details
   *
   * 这个函数用于上层应用查询当前节点的状态
   * 上层应用需要知道当前节点是否是领导者
   * 如果是领导者，才能处理客户端请求
   */



  /**
   * @brief 领导者心跳定时器
   * @details
   *
   * 领导者心跳定时器的作用：
   * 1. 定期发送心跳：领导者定期向所有跟随者发送心跳
   * 2. 维持领导地位：防止跟随者发起选举
   * 3. 日志复制：心跳可以携带日志，实现日志复制
   *
   * 心跳的发送：
   * 1. 领导者定期向所有跟随者发送AppendEntries RPC
   * 2. AppendEntries可以携带日志，也可以是空的心跳
   * 3. 心跳的间隔通常比选举超时短得多
   */




  /**
   * @brief 领导者更新提交索引
   * @details
   *
   * 更新提交索引的规则：
   * 1. 找到大多数节点都匹配的日志索引N
   * 2. 如果日志N的任期号是当前任期，可以提交
   * 3. 更新m_commitIndex为N
   *
   * 为什么需要这个规则：
   * - 只有大多数节点都匹配的日志才能提交
   * - 确保提交的日志不会因为领导者变更而丢失
   * - 确保所有节点以相同的顺序执行相同的日志
   */
  void leaderUpdateCommitIndex();

  /**
   * @brief 检查日志是否匹配
   * @param logIndex 日志索引
   * @param logTerm 日志任期号
   * @return 是否匹配
   * @details
   *
   * 日志匹配的条件：
   * 1. 日志索引和任期号都匹配
   * 2. 如果日志索引超出范围，不匹配
   * 3. 如果日志索引在范围内，但任期号不匹配，不匹配
   *
   * 日志匹配的作用：
   * 1. 检查日志一致性：跟随者通过这个函数检查日志是否一致
   * 2. 日志复制：领导者通过这个函数找到匹配的日志
   * 3. 快照安装：通过这个函数检查快照是否有效
   */
  bool matchLog(int logIndex, int logTerm);

  /**
   * @brief 持久化Raft状态
   * @details
   *
   * 持久化的数据：
   * 1. m_currentTerm：当前任期号
   * 2. m_votedFor：本轮投票给的节点ID
   * 3. m_logs：日志条目数组
   * 4. m_lastSnapshotIncludeIndex：快照中包含的最后一个日志索引
   * 5. m_lastSnapshotIncludeTerm：快照中包含的最后一个日志的任期号
   *
   * 持久化的作用：
   * 1. 容错：节点重启后可以恢复状态
   * 2. 一致性：确保所有节点的持久化状态一致
   * 3. 安全性：防止数据丢失
   */
  void persist();

  /**
   * @brief 处理投票请求（RPC）
   * @param args 投票请求的参数
   * @param reply 投票请求的回复
   * @details
   *
   * RequestVote RPC的作用：
   * 1. 领导者选举：候选者通过RequestVote请求投票
   * 2. 任期更新：如果发现更高任期，更新当前任期
   * 3. 状态转换：如果发现更高任期的领导者，转为Follower
   *
   * RequestVote的参数：
   * - Term：候选者的任期号
   * - CandidateId：候选者的ID
   * - LastLogIndex：候选者最后一个日志的索引
   * - LastLogTerm：候选者最后一个日志的任期号
   *
   * RequestVote的回复：
   * - Term：投票者的当前任期号
   * - VoteGranted：是否投票给候选者
   *
   * 投票的规则：
   * 1. 如果args->Term < m_currentTerm，拒绝投票
   * 2. 如果m_votedFor == -1或m_votedFor == args->CandidateId，可以投票
   * 3. 如果候选者的日志至少一样新，投票给候选者
   * 4. 否则，拒绝投票
   */
  void RequestVote(const raftRpcProctoc::RequestVoteArgs *args, raftRpcProctoc::RequestVoteReply *reply);

  /**
   * @brief 检查日志是否更新
   * @param index 日志索引
   * @param term 日志任期号
   * @return 是否更新
   * @details
   *
   * 日志更新的定义：
   * 1. 任期号更大：term > m_currentTerm
   * 2. 任期号相同，索引更大：term == m_currentTerm && index > getLastLogIndex()
   *
   * 日志更新的作用：
   * 1. 选举投票：候选者的日志必须至少一样新才能获得投票
   * 2. 日志复制：领导者通过这个函数找到匹配的日志
   * 3. 安全性：确保只有拥有完整日志的节点才能成为领导者
   */
  bool checkDataToCandidate(int index, int term);

  /**
   * @brief 获取最后一个日志的索引
   * @return 最后一个日志的索引
   * @details
   *
   * 如果没有日志，返回0
   * 如果有日志，返回最后一个日志的索引
   */
  int getLastLogIndex();

  /**
   * @brief 获取最后一个日志的任期号
   * @return 最后一个日志的任期号
   * @details
   *
   * 如果没有日志，返回0
   * 如果有日志，返回最后一个日志的任期号
   */
  int getLastLogTerm();

  /**
   * @brief 获取最后一个日志的索引和任期号
   * @param lastLogIndex 最后一个日志的索引（输出参数）
   * @param lastLogTerm 最后一个日志的任期号（输出参数）
   * @details
   *
   * 这个函数同时获取最后一个日志的索引和任期号
   * 用于选举投票时比较日志的新旧程度
   */
  void getLastLogIndexAndTerm(int *lastLogIndex, int *lastLogTerm);

  /**
   * @brief 从日志索引获取日志任期号
   * @param logIndex 日志索引
   * @return 日志任期号
   * @details
   *
   * 如果日志索引超出范围，返回0
   * 如果日志索引在范围内，返回对应日志的任期号
   */
  int getLogTermFromLogIndex(int logIndex);



  /**
   * @brief 从日志索引获取切片索引
   * @param logIndex 日志索引
   * @return 切片索引
   * @details
   *
   * 这个函数用于日志压缩时计算切片索引
   * 切片索引表示日志在日志数组中的位置
   */
  int getSlicesIndexFromLogIndex(int logIndex);

  /**
   * @brief 发送投票请求
   * @param server 目标节点
   * @param args 投票请求的参数
   * @param reply 投票请求的回复
   * @param votedNum 获得的投票数（输出参数）
   * @return 是否成功发送
   * @details
   *
   * 这个函数用于候选者向其他节点发送投票请求
   * 发送RequestVote RPC，等待回复
   * 统计获得的投票数
   */
  bool sendRequestVote(int server, std::shared_ptr<raftRpcProctoc::RequestVoteArgs> args,
                       std::shared_ptr<raftRpcProctoc::RequestVoteReply> reply, std::shared_ptr<int> votedNum);

  /**
   * @brief 发送追加日志请求
   * @param server 目标节点
   * @param args 追加日志的参数
   * @param reply 追加日志的回复
   * @param appendNums 追加的日志数（输出参数）
   * @return 是否成功发送
   * @details
   *
   * 这个函数用于领导者向跟随者发送追加日志请求
   * 发送AppendEntries RPC，等待回复
   * 统计追加的日志数
   */
bool SendAppendEntries(int server, std::shared_ptr<raftRpcProctoc::AppendEntriesArgs> args,
                             std::shared_ptr<raftRpcProctoc::AppendEntriesReply> reply,
                             std::shared_ptr<int> appendNums);

  /**
   * @brief 将消息推送到KVServer
   * @param msg 应用消息
   * @details
   *
   * 这个函数用于Raft节点与上层应用（KVServer）通信
   * 当日志被提交时，Raft节点会将ApplyMsg放入applyChan队列
   * 上层应用从队列中取出ApplyMsg，应用到状态机
   */
  void pushMsgToKvServer(ApplyMsg msg);

  /**
   * @brief 读取持久化数据
   * @param data 持久化数据
   * @details
   *
   * 这个函数用于从持久化数据中恢复Raft状态
   * 节点重启后，从磁盘中读取持久化数据
   * 恢复m_currentTerm、m_votedFor、m_logs等状态
   */
  void readPersist(std::string data);

  /**
   * @brief 获取持久化数据
   * @return 持久化数据
   * @details
   *
   * 这个函数用于将Raft状态序列化为字符串
   * 序列化的数据可以写入磁盘，实现持久化
   * 包括m_currentTerm、m_votedFor、m_logs等状态
   */
  std::string persistData();

  /**
   * @brief 启动Raft节点
   * @param command 命令
   * @param newLogIndex 新日志索引（输出参数）
   * @param newLogTerm 新日志任期号（输出参数）
   * @param isLeader 是否是领导者（输出参数）
   * @details
   *
   * 这个函数用于上层应用向Raft节点提交命令
   * 只有领导者才能处理客户端请求
   * 领导者将命令追加到日志，复制到跟随者
   * 当日志被提交时，上层应用可以应用到状态机
   */


  /**
   * @brief 创建快照
   * @param index 快照应用的索引
   * @param snapshot 快照数据
   * @details
   *
   * 快照的作用：
   * 1. 压缩日志：当日志过大时，创建快照，删除快照之前的日志
   * 2. 快速恢复：新节点可以通过安装快照快速加入集群
   * 3. 节省空间：快照比日志占用更少的空间
   *
   * 创建快照的步骤：
   * 1. 上层应用创建快照，包含所有应用到index的数据
   * 2. 上层应用调用Snapshot函数，将快照传递给Raft节点
   * 3. Raft节点删除快照之前的日志
   * 4. Raft节点更新m_lastSnapshotIncludeIndex和m_lastSnapshotIncludeTerm
   * 5. Raft节点持久化快照数据
   *
   * index代表是快照apply应用的index,而snapshot代表的是上层service传来的快照字节流，包括了Index之前的数据
   * 这个函数的目的是把安装到快照里的日志抛弃，并安装快照数据，同时更新快照下标，属于peers自身主动更新，与leader发送快照不冲突
   * 即服务层主动发起请求raft保存snapshot里面的数据，index是用来表示snapshot快照执行到了哪条命令
   */
  

 public:
  // 重写基类方法,因为rpc远程调用真正调用的是这个方法
  //序列化，反序列化等操作rpc框架都已经做完了，因此这里只需要获取值然后真正调用本地方法即可。
  /**
   * @brief 处理追加日志请求（RPC）
   * @param controller RPC控制器
   * @param request 追加日志的参数
   * @param response 追加日志的回复
   * @param done RPC完成回调
   * @details
   *
   * 这个函数是RPC框架调用的入口函数
   * RPC框架负责序列化、反序列化、网络传输等
   * 这个函数只需要调用本地方法AppendEntries1处理请求
   */
  void AppendEntries(google::protobuf::RpcController *controller, const ::raftRpcProctoc::AppendEntriesArgs *request,
                     ::raftRpcProctoc::AppendEntriesReply *response, ::google::protobuf::Closure *done) override;


  /**
   * @brief 处理投票请求（RPC）
   * @param controller RPC控制器
   * @param request 投票请求的参数
   * @param response 投票请求的回复
   * @param done RPC完成回调
   * @details
   *
   * 这个函数是RPC框架调用的入口函数
   * RPC框架负责序列化、反序列化、网络传输等
   * 这个函数只需要调用本地方法RequestVote处理请求
   */
  void RequestVote(google::protobuf::RpcController *controller, const ::raftRpcProctoc::RequestVoteArgs *request,
                   ::raftRpcProctoc::RequestVoteReply *response, ::google::protobuf::Closure *done) override;

 public:
  /**
   * @brief 初始化Raft节点
   * @param peers 其他节点的RPC客户端列表
   * @param me 当前节点的ID
   * @param persister 持久化对象
   * @param applyCh 应用消息队列
   * @details
   *
   * 初始化的步骤：
   * 1. 保存其他节点的RPC客户端列表
   * 2. 保存当前节点的ID
   * 3. 保存持久化对象
   * 4. 保存应用消息队列
   * 5. 初始化Raft状态
   * 6. 从持久化数据中恢复状态
   * 7. 启动定时器
   */
  void init(std::vector<std::shared_ptr<RaftRpcUtil>> peers, int id, std::shared_ptr<Persister> persister,
            std::shared_ptr<LockQueue<ApplyMsg>> applyCh);
  void Propose(Op command, int *newLogIndex, int *newLogTerm, bool *isLeader);
  void GetState(int *term, bool *isLeader);
  int GetRaftStateSize();
 private:
  void doElection();
  void doHeartBeat();
  void checkAndVerbCandidate();
  bool AsyncSendRequestVote(int server, std::shared_ptr<raftRpcProctoc::RequestVoteArgs> requestVoteArgs,
                         std::shared_ptr<raftRpcProctoc::RequestVoteReply> requestVoteReply);
  /**
   * @class BoostPersistRaftNode
   * @brief 用于持久化的Raft节点数据结构
   * @details
   *
   * 这个类用于将Raft状态序列化为字符串
   * 使用Boost.Serialization库实现序列化
   * 序列化的数据可以写入磁盘，实现持久化
   */
  class BoostPersistRaftNode {
   public:
    friend class boost::serialization::access;
    // When the class Archive corresponds to an output archive, the
    // & operator is defined similar to <<. Likewise, when the class Archive
    // is a type of input archive, & operator is defined similar to >>.
    template <class Archive>
    void serialize(Archive &ar, const unsigned int version) {
      ar &m_currentTerm;
      ar &m_votedFor;
      ar &m_lastSnapshotIncludeIndex;
      ar &m_lastSnapshotIncludeTerm;
      ar &m_logs;
    }
    int m_currentTerm;
    int m_votedFor;
    int m_lastSnapshotIncludeIndex;
    int m_lastSnapshotIncludeTerm;
    std::vector<std::string> m_logs;
    std::unordered_map<std::string, int> umap;

   
  };
  // Status：节点状态枚举
  // - Follower：跟随者，被动接收领导者的日志和心跳
  // - Candidate：候选者，参与领导者选举
  // - Leader：领导者，处理所有客户端请求，复制日志到跟随者
  enum Status { Follower, Candidate, Leader };

  // m_status：当前节点的状态
  // 节点状态会在Follower、Candidate、Leader之间转换
  // - Follower -> Candidate：选举超时
  // - Candidate -> Leader：获得大多数投票
  // - Candidate -> Follower：发现更高任期的领导者或选举超时
  // - Leader -> Follower：发现更高任期的领导者
  Status status_;
  void run();
  Status checkStatus();
  void doLeaderWork();
  void doCandidateWork();
  void doFollowerWork();
  Thread thread_;
  bool isRunning_;
};
}  // namespace yy
}
#endif  // RAFT_H