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
  std::mutex m_mtx;
  std::string m_raftState;

  const std::string m_raftStateFileName;
  std::ofstream m_raftStateOutStream;
  long long m_raftStateSize;

 public:

  void Save(std::string raftstate);

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
};

#endif  // SKIP_LIST_ON_RAFT_PERSISTER_H
