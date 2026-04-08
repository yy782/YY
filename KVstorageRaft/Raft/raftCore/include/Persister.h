/**
 * @file Persister.h
 * @brief Persister类定义 - Raft状态持久化
 * @details
 *
 * Persister负责将Raft的关键状态持久化到磁盘
 *
 * 持久化基础知识：
 * - 持久化：将数据保存到磁盘，防止数据丢失
 * - Raft要求某些关键数据必须持久化
 * - 节点重启后可以从磁盘恢复状态
 *
 * 为什么需要持久化：
 * - 防止节点崩溃导致数据丢失
 * - 确保Raft算法的正确性
 * - 支持节点重启和恢复
 *
 * 需要持久化的数据：
 * 1. Raft状态：currentTerm、votedFor、logs
 * 2. 快照：状态机的当前状态
 *
 * 持久化时机：
 * - 每次修改Raft状态后
 * - 每次制作快照后
 * - 定期保存，防止数据丢失
 */

#ifndef SKIP_LIST_ON_RAFT_PERSISTER_H
#define SKIP_LIST_ON_RAFT_PERSISTER_H
#include <fstream>
#include <mutex>

/**
 * @class Persister
 * @brief Raft状态持久化类
 * @details
 *
 * Persister负责将Raft的关键状态持久化到磁盘
 *
 * 主要功能：
 * 1. 保存Raft状态（currentTerm、votedFor、logs）
 * 2. 保存快照（状态机的当前状态）
 * 3. 读取Raft状态
 * 4. 读取快照
 * 5. 获取Raft状态大小
 *
 * 持久化文件：
 * - raftState文件：保存Raft状态
 * - snapshot文件：保存快照
 *
 * 工作流程：
 * 1. Raft修改状态后，调用Save()保存
 * 2. Save()将Raft状态和快照写入磁盘
 * 3. 节点重启后，调用ReadRaftState()和ReadSnapshot()恢复状态
 */
class Persister {
 private:
  /**
   * @brief 互斥锁，保护持久化操作的并发访问
   * @details
   * 保护以下共享数据：
   * - m_raftState：Raft状态
   * - m_snapshot：快照数据
   * - m_raftStateSize：Raft状态大小
   * - 文件操作：读写文件
   */
  std::mutex m_mtx;

  /**
   * @brief Raft状态（内存中的副本）
   * @details
   * 保存Raft的关键状态：
   * - currentTerm：当前任期号
   * - votedFor：本轮投票给的节点ID
   * - logs：日志条目数组
   *
   * 为什么需要内存副本：
   * - 避免每次都读取文件
   * - 提高读取性能
   * - 减少磁盘I/O
   */
  std::string m_raftState;

  /**
   * @brief 快照数据（内存中的副本）
   * @details
   * 保存状态机的当前状态：
   * - KV数据库的内容
   * - 最后应用的日志索引
   * - 客户端请求ID映射
   *
   * 为什么需要内存副本：
   * - 避免每次都读取文件
   * - 提高读取性能
   * - 减少磁盘I/O
   */
  std::string m_snapshot;

  /**
   * @brief Raft状态文件名
   * @details
   * 文件名格式：raftstate-{me}
   * 其中me是节点ID
   * 例如：raftstate-0、raftstate-1、raftstate-2
   *
   * 为什么每个节点有独立的文件：
   * - 每个节点有独立的Raft状态
   * - 避免文件冲突
   * - 支持多节点部署
   */
  const std::string m_raftStateFileName;

  /**
   * @brief 快照文件名
   * @details
   * 文件名格式：snapshot-{me}
   * 其中me是节点ID
   * 例如：snapshot-0、snapshot-1、snapshot-2
   *
   * 为什么每个节点有独立的文件：
   * - 每个节点有独立的快照
   * - 避免文件冲突
   * - 支持多节点部署
   */
  const std::string m_snapshotFileName;

  /**
   * @brief 保存Raft状态的输出流
   * @details
   * 用于将Raft状态写入文件
   *
   * 为什么使用输出流：
   * - 提高写入性能
   * - 支持批量写入
   * - 减少系统调用
   */
  std::ofstream m_raftStateOutStream;

  /**
   * @brief 保存快照的输出流
   * @details
   * 用于将快照写入文件
   *
   * 为什么使用输出流：
   * - 提高写入性能
   * - 支持批量写入
   * - 减少系统调用
   */
  std::ofstream m_snapshotOutStream;

  /**
   * @brief Raft状态大小（字节）
   * @details
   * 记录Raft状态的大小，避免每次都读取文件来获取大小
   *
   * 为什么需要缓存大小：
   * - 避免每次都读取文件
   * - 提高性能
   * - 用于判断是否需要制作快照
   *
   * 更新时机：
   * - 每次保存Raft状态后更新
   */
  long long m_raftStateSize;

 public:
  /**
   * @brief 保存Raft状态和快照
   * @param raftstate Raft状态数据
   * @param snapshot 快照数据
   * @details
   *
   * 保存流程：
   * 1. 更新内存中的m_raftState和m_snapshot
   * 2. 将m_raftState写入raftState文件
   * 3. 将m_snapshot写入snapshot文件
   * 4. 更新m_raftStateSize
   *
   * 为什么需要同时保存Raft状态和快照：
   * - Raft状态和快照是一致的
   * - 需要原子性地保存两者
   * - 避免状态不一致
   */
  void Save(std::string raftstate, std::string snapshot);

  /**
   * @brief 读取快照数据
   * @return 快照数据
   * @details
   *
   * 读取流程：
   * 1. 如果内存中有快照，直接返回
   * 2. 如果内存中没有快照，从文件读取
   * 3. 更新内存中的m_snapshot
   * 4. 返回快照数据
   *
   * 为什么需要先检查内存：
   * - 避免重复读取文件
   * - 提高性能
   * - 减少磁盘I/O
   */
  std::string ReadSnapshot();

  /**
   * @brief 保存Raft状态
   * @param data Raft状态数据
   * @details
   *
   * 保存流程：
   * 1. 更新内存中的m_raftState
   * 2. 将m_raftState写入raftState文件
   * 3. 更新m_raftStateSize
   *
   * 为什么单独提供SaveRaftState：
   * - 有时只需要保存Raft状态，不需要保存快照
   * - 提高灵活性
   * - 减少不必要的磁盘I/O
   */
  void SaveRaftState(const std::string& data);

  /**
   * @brief 获取Raft状态大小
   * @return Raft状态大小（字节）
   * @details
   *
   * 返回m_raftStateSize
   *
   * 为什么需要这个方法：
   * - 用于判断是否需要制作快照
   * - 避免每次都读取文件
   * - 提高性能
   */
  long long RaftStateSize();

  /**
   * @brief 读取Raft状态
   * @return Raft状态数据
   * @details
   *
   * 读取流程：
   * 1. 如果内存中有Raft状态，直接返回
   * 2. 如果内存中没有Raft状态，从文件读取
   * 3. 更新内存中的m_raftState
   * 4. 返回Raft状态数据
   *
   * 为什么需要先检查内存：
   * - 避免重复读取文件
   * - 提高性能
   * - 减少磁盘I/O
   */
  std::string ReadRaftState();

  /**
   * @brief 构造函数
   * @param me 当前节点的ID
   * @details
   *
   * 初始化步骤：
   * 1. 生成文件名（raftstate-{me}、snapshot-{me}）
   * 2. 初始化成员变量
   * 3. 如果文件存在，从文件读取状态
   * 4. 如果文件不存在，创建空文件
   */
  explicit Persister(int me);

  /**
   * @brief 析构函数
   * @details
   *
   * 清理步骤：
   * 1. 关闭所有打开的文件
   * 2. 清理资源
   */
  ~Persister();

 private:
  /**
   * @brief 清空Raft状态
   * @details
   *
   * 清空流程：
   * 1. 清空内存中的m_raftState
   * 2. 清空raftState文件
   * 3. 重置m_raftStateSize为0
   *
   * 什么时候会调用：
   * - 节点重启时
   * - 需要重置Raft状态时
   */
  void clearRaftState();

  /**
   * @brief 清空快照
   * @details
   *
   * 清空流程：
   * 1. 清空内存中的m_snapshot
   * 2. 清空snapshot文件
   *
   * 什么时候会调用：
   * - 节点重启时
   * - 需要重置快照时
   */
  void clearSnapshot();

  /**
   * @brief 清空Raft状态和快照
   * @details
   *
   * 清空流程：
   * 1. 调用clearRaftState()清空Raft状态
   * 2. 调用clearSnapshot()清空快照
   *
   * 什么时候会调用：
   * - 节点重启时
   * - 需要完全重置时
   */
  void clearRaftStateAndSnapshot();
};

#endif  // SKIP_LIST_ON_RAFT_PERSISTER_H
