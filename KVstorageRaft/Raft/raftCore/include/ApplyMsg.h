/**
 * @file ApplyMsg.h
 * @brief ApplyMsg类定义 - Raft和KVServer之间的通信消息
 * @details
 *
 * ApplyMsg是Raft和KVServer之间的通信消息类型
 *
 * 通信基础知识：
 * - Raft和KVServer是两个独立的模块
 * - 它们需要通过某种机制进行通信
 * - 使用管道（LockQueue）传递消息
 *
 * 消息类型：
 * 1. 命令消息（CommandValid=true）：普通命令（PUT/GET/APPEND）
 * 2. 快照消息（SnapshotValid=true）：快照数据
 *
 * 消息流向：
 * Raft -> applyCh -> KVServer
 *
 * 为什么需要ApplyMsg：
 * - Raft达成共识后，需要通知KVServer
 * - KVServer需要将命令应用到状态机
 * - 需要一种统一的消息格式
 */

#ifndef APPLYMSG_H
#define APPLYMSG_H
#include <string>

/**
 * @class ApplyMsg
 * @brief Raft和KVServer之间的通信消息
 * @details
 *
 * ApplyMsg有两种类型：
 * 1. 命令消息：包含要执行的命令
 * 2. 快照消息：包含快照数据
 *
 * 命令消息（CommandValid=true）：
 * - Command：命令内容（序列化的Op操作）
 * - CommandIndex：命令的Raft日志索引
 *
 * 快照消息（SnapshotValid=true）：
 * - Snapshot：快照数据
 * - SnapshotTerm：快照的任期号
 * - SnapshotIndex：快照的Raft日志索引
 *
 * 为什么需要两个valid标志：
 * - 区分消息类型
 * - 确保消息的完整性
 * - 避免混淆
 */
class ApplyMsg {
 public:
  /**
   * @brief 命令有效标志
   * @details
   * true表示这是一个命令消息
   * false表示这不是一个命令消息
   *
   * 命令消息包含：
   * - Command：命令内容
   * - CommandIndex：命令索引
   */
  bool CommandValid;

  /**
   * @brief 命令内容
   * @details
   * 序列化的Op操作
   *
   * Op操作包含：
   * - Operation：操作类型（"Put"、"Append"、"Get"）
   * - Key：键
   * - Value：值
   * - ClientId：客户端ID
   * - RequestId：请求ID
   *
   * 为什么需要序列化：
   * - 统一消息格式
   * - 方便传输
   * - 支持多种操作类型
   */
  std::string Command;

  /**
   * @brief 命令的Raft日志索引
   * @details
   * 表示这个命令在Raft日志中的位置
   *
   * 索引的作用：
   * - 标识命令的唯一性
   * - 用于等待和通知
   * - 用于去重
   *
   * 索引的用途：
   * - KVServer通过索引找到对应的waitApplyCh
   * - KVServer通过索引判断命令是否重复
   * - KVServer通过索引更新m_lastApplied
   */
  int CommandIndex;

  /**
   * @brief 快照有效标志
   * @details
   * true表示这是一个快照消息
   * false表示这不是一个快照消息
   *
   * 快照消息包含：
   * - Snapshot：快照数据
   * - SnapshotTerm：快照的任期号
   * - SnapshotIndex：快照的Raft日志索引
   */
  bool SnapshotValid;

  /**
   * @brief 快照数据
   * @details
   * 序列化的快照内容
   *
   * 快照内容包含：
   * - KV数据库的内容（m_skipList）
   * - 最后应用的Raft日志索引
   * - 客户端请求ID映射（m_lastRequestId）
   *
   * 为什么需要快照：
   * - 减少Raft日志大小
   * - 加速节点恢复
   * - 防止日志无限增长
   */
  std::string Snapshot;

  /**
   * @brief 快照的任期号
   * @details
   * 表示快照是在哪个任期制作的
   *
   * 任期号的作用：
   * - 标识快照的新旧程度
   * - 用于判断快照是否过时
   * - 用于Raft算法的正确性
   *
   * 为什么需要任期号：
   * - Raft算法需要知道快照的任期
   * - 用于安全性检查
   * - 避免使用过时的快照
   */
  int SnapshotTerm;

  /**
   * @brief 快照的Raft日志索引
   * @details
   * 表示快照对应的最后一个Raft日志索引
   *
   * 索引的作用：
   * - 标识快照的位置
   * - 用于判断快照是否过时
   * - 用于日志压缩
   *
   * 为什么需要索引：
   * - Raft算法需要知道快照的索引
   * - 用于日志压缩
   * - 用于判断是否需要安装快照
   */
  int SnapshotIndex;

 public:
  /**
   * @brief 构造函数
   * @details
   *
   * 初始化所有成员变量：
   * - CommandValid = false
   * - Command = ""
   * - CommandIndex = -1
   * - SnapshotValid = false
   * - Snapshot = ""
   * - SnapshotTerm = -1
   * - SnapshotIndex = -1
   *
   * 为什么需要初始化：
   * - 确保消息的完整性
   * - 避免未定义行为
   * - 方便判断消息类型
   *
   * 为什么两个valid都要初始化为false：
   * - 表示这是一个空消息
   * - 避免误判消息类型
   * - 确保消息的正确性
   */
  ApplyMsg()
      : CommandValid(false),
        Command(),
        CommandIndex(-1),
        SnapshotValid(false),
        Snapshot(),
        SnapshotTerm(-1),
        SnapshotIndex(-1){

        };
};

#endif  // APPLYMSG_H