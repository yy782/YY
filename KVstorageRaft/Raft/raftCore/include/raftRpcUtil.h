/**
 * @file raftRpcUtil.h
 * @brief RaftRpcUtil类定义 - Raft节点之间的RPC通信
 * @details
 *
 * RaftRpcUtil负责维护当前节点对其他节点的RPC通信
 *
 * RPC基础知识：
 * - RPC（Remote Procedure Call）：远程过程调用
 * - 允许程序调用另一台计算机上的函数
 * - 就像调用本地函数一样简单
 * - 底层使用网络通信（TCP/UDP）
 *
 * Raft节点之间的通信：
 * - 每个节点都需要与其他节点通信
 * - 使用RPC发送RequestVote、AppendEntries、InstallSnapshot请求
 * - 使用RPC接收其他节点的响应
 *
 * 为什么需要RaftRpcUtil：
 * - 封装RPC通信细节
 * - 提供统一的接口
 * - 简化Raft算法的实现
 */

#ifndef RAFTRPC_H
#define RAFTRPC_H

#include "raftRPC.pb.h"

/**
 * @class RaftRpcUtil
 * @brief 维护当前节点对其他某一个节点的所有RPC发送通信功能
 * @details
 *
 * RaftRpcUtil是一个Raft节点与另一个Raft节点之间的RPC通信封装
 *
 * 主要功能：
 * 1. 发送RequestVote请求
 * 2. 发送AppendEntries请求
 * 3. 发送InstallSnapshot请求
 * 4. 接收RPC响应
 *
 * 设计模式：
 * - 每个Raft节点维护一个RaftRpcUtil列表
 * - 每个RaftRpcUtil对应一个其他节点
 * - 例如：3个节点的集群，每个节点维护2个RaftRpcUtil
 *
 * 为什么每个节点需要多个RaftRpcUtil：
 * - Raft算法需要与所有其他节点通信
 * - 每个RaftRpcUtil对应一个目标节点
 * - 避免重复创建连接
 *
 * 工作流程：
 * 1. Raft节点初始化时，为每个其他节点创建一个RaftRpcUtil
 * 2. 需要发送RPC时，调用对应的RaftRpcUtil方法
 * 3. RaftRpcUtil发送RPC请求，等待响应
 * 4. RaftRpcUtil返回响应给Raft节点
 */
class RaftRpcUtil {
 private:
  /**
   * @brief Protobuf生成的RPC存根（Stub）
   * @details
   * stub_是Protobuf RPC框架生成的客户端存根
   *
   * 存根的作用：
   * - 封装网络通信细节
   * - 提供RPC调用接口
   * - 序列化和反序列化参数
   *
   * 使用方式：
   * - stub_->RequestVote()：发送RequestVote请求
   * - stub_->AppendEntries()：发送AppendEntries请求
   * - stub_->InstallSnapshot()：发送InstallSnapshot请求
   *
   * 为什么需要存根：
   * - 简化RPC调用
   * - 自动处理网络通信
   * - 自动序列化和反序列化
   */
  raftRpcProctoc::raftRpc_Stub *stub_;

 public:
  /**
   * @brief 发送AppendEntries请求
   * @param args AppendEntries请求参数
   * @param response AppendEntries响应
   * @return true表示发送成功，false表示发送失败
   * @details
   *
   * AppendEntries请求的作用：
   * - 领导者发送心跳
   * - 领导者复制日志
   * - 领导者检查跟随者的状态
   *
   * AppendEntries请求参数（args）：
   * - Term：领导者的任期号
   * - LeaderId：领导者的ID
   * - PrevLogIndex：前一个日志的索引
   * - PrevLogTerm：前一个日志的任期号
   * - Entries：要追加的日志条目数组
   * - LeaderCommit：领导者的提交索引
   *
   * AppendEntries响应（response）：
   * - Term：跟随者的当前任期号
   * - Success：是否成功追加日志
   *
   * 工作流程：
   * 1. 构造AppendEntries请求参数
   * 2. 调用stub_->AppendEntries()发送请求
   * 3. 等待响应
   * 4. 返回响应
   */
  bool AppendEntries(raftRpcProctoc::AppendEntriesArgs *args, raftRpcProctoc::AppendEntriesReply *response);

  /**
   * @brief 发送InstallSnapshot请求
   * @param args InstallSnapshot请求参数
   * @param response InstallSnapshot响应
   * @return true表示发送成功，false表示发送失败
   * @details
   *
   * InstallSnapshot请求的作用：
   * - 领导者发送快照给跟随者
   * - 跟随者安装快照，追赶领导者
   * - 用于日志压缩
   *
   * InstallSnapshot请求参数（args）：
   * - Term：领导者的任期号
   * - LeaderId：领导者的ID
   * - LastIncludedIndex：快照包含的最后一个日志索引
   * - LastIncludedTerm：快照包含的最后一个日志任期号
   * - Data：快照数据
   *
   * InstallSnapshot响应（response）：
   * - Term：跟随者的当前任期号
   *
   * 工作流程：
   * 1. 构造InstallSnapshot请求参数
   * 2. 调用stub_->InstallSnapshot()发送请求
   * 3. 等待响应
   * 4. 返回响应
   */
  bool InstallSnapshot(raftRpcProctoc::InstallSnapshotRequest *args, raftRpcProctoc::InstallSnapshotResponse *response);

  /**
   * @brief 发送RequestVote请求
   * @param args RequestVote请求参数
   * @param response RequestVote响应
   * @return true表示发送成功，false表示发送失败
   * @details
   *
   * RequestVote请求的作用：
   * - 候选者发起选举
   * - 请求其他节点投票
   * - 用于领导者选举
   *
   * RequestVote请求参数（args）：
   * - Term：候选者的任期号
   * - CandidateId：候选者的ID
   * - LastLogIndex：候选者最后一个日志的索引
   * - LastLogTerm：候选者最后一个日志的任期号
   *
   * RequestVote响应（response）：
   * - Term：投票者的当前任期号
   * - VoteGranted：是否投票给候选者
   *
   * 工作流程：
   * 1. 构造RequestVote请求参数
   * 2. 调用stub_->RequestVote()发送请求
   * 3. 等待响应
   * 4. 返回响应
   */
  bool RequestVote(raftRpcProctoc::RequestVoteArgs *args, raftRpcProctoc::RequestVoteReply *response);

  /**
   * @brief 构造函数
   * @param ip 远端节点的IP地址
   * @param port 远端节点的端口
   * @details
   *
   * 初始化步骤：
   * 1. 创建RPC通道（MprpcChannel）
   * 2. 创建RPC存根（stub_）
   * 3. 设置RPC参数（超时时间等）
   *
   * 为什么需要ip和port：
   * - 标识目标节点的地址
   * - 用于建立TCP连接
   * - 用于发送RPC请求
   *
   * 示例：
   * - RaftRpcUtil("127.0.0.1", 5000)：连接到本地5000端口的节点
   * - RaftRpcUtil("192.168.1.100", 5001)：连接到远程5001端口的节点
   */
  RaftRpcUtil(std::string ip, short port);

  /**
   * @brief 析构函数
   * @details
   *
   * 清理步骤：
   * 1. 删除RPC存根（stub_）
   * 2. 关闭RPC通道
   * 3. 清理资源
   */
  ~RaftRpcUtil();
};

#endif  // RAFTRPC_H
