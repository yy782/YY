/**
 * @file clerk.h
 * @brief Clerk类定义 - KV存储客户端
 * @details
 *
 * Clerk是分布式KV存储系统的客户端库
 *
 * 客户端基础知识：
 * - 客户端：使用服务的程序
 * - 服务器：提供服务的程序
 * - RPC：客户端和服务器之间的通信方式
 *
 * Clerk的主要功能：
 * 1. 提供PUT、GET、APPEND操作接口
 * 2. 自动处理请求重试
 * 3. 自动发现领导者
 * 4. 处理请求去重
 *
 * 为什么需要Clerk：
 * - 封装RPC通信细节
 * - 提供简单的接口
 * - 自动处理故障和重试
 * - 对用户屏蔽分布式系统的复杂性
 *
 * 架构设计：
 * - 客户端 -> Clerk -> KVServer -> Raft -> 持久化存储
 * - Clerk：客户端库，负责请求路由和重试
 * - KVServer：服务器端，处理请求并调用Raft
 * - Raft：一致性算法，保证数据一致性
 * - 持久化存储：保存Raft状态和快照
 */

#ifndef SKIP_LIST_ON_RAFT_CLERK_H
#define SKIP_LIST_ON_RAFT_CLERK_H
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cerrno>
#include <string>
#include <vector>

#include "raftServerRpcUtil.h"
#include "kvServerRPC.pb.h"
#include "mprpcconfig.h"

namespace yy 
{
/**
 * @class Clerk
 * @brief KV存储客户端
 * @details
 *
 * Clerk是分布式KV存储系统的客户端库，提供简单的PUT、GET、APPEND接口
 *
 * 主要功能：
 * 1. 提供PUT、GET、APPEND操作接口
 * 2. 自动处理请求重试
 * 3. 自动发现领导者
 * 4. 处理请求去重
 *
 * 工作流程：
 * 1. 客户端调用Clerk的PUT/GET/APPEND方法
 * 2. Clerk构造请求（包含ClientId和RequestId）
 * 3. Clerk向KVServer发送RPC请求
 * 4. 如果请求失败，Clerk重试
 * 5. 如果收到"NotLeader"错误，Clerk切换到其他节点
 * 6. Clerk返回结果给客户端
 * 
 * 关键设计：
 * - m_servers：保存所有KVServer的RPC客户端
 * - m_clientId：客户端的唯一标识
 * - m_requestId：请求ID，每次请求递增
 * - m_recentLeaderId：最近已知的领导者ID
 *
 * 为什么需要自动重试：
 * - 网络不稳定可能导致请求失败
 * - 领导者可能变更
 * - 节点可能故障
 * - 自动重试提高可靠性
 *
 * 为什么需要自动发现领导者：
 * - Raft的领导者可能变更
 * - 客户端不知道当前领导者是谁
 * - Clerk需要自动发现领导者
 */
class Clerk {
  private:
  /**
   * @brief 保存所有KVServer的RPC客户端
   * @details
   * 这是一个向量，每个元素是一个raftServerRpcUtil指针
   * 每个raftServerRpcUtil对应一个KVServer节点
   *
   * 为什么需要保存所有节点：
   * - 客户端不知道哪个节点是领导者
   * - 需要向所有节点发送请求
   * - 如果当前节点不是领导者，切换到其他节点
   *
   * 工作流程：
   * 1. 客户端向m_recentLeaderId对应的节点发送请求
   * 2. 如果收到"NotLeader"错误，切换到其他节点
   * 3. 重复步骤1-2，直到找到领导者
   */
  std::vector<std::shared_ptr<raftServerRpcUtil>> m_servers;

  /**
   * @brief 客户端的唯一标识
   * @details
   * 每个客户端有一个唯一的ClientId
   * 用于去重和识别客户端
   *
   * 为什么需要ClientId：
   * - 服务器需要识别不同的客户端
   * - 用于去重检查
   * - 用于记录每个客户端的请求历史
   *
   * ClientId的生成：
   * - 使用Uuid()方法生成
   * - 由多个随机数拼接而成
   * - 保证唯一性
   */
  std::string m_clientId;

  /**
   * @brief 请求ID，每次请求递增
   * @details
   * 每个请求有一个唯一的RequestId
   * 用于去重和识别请求
   *
   * 为什么需要RequestId：
   * - 服务器需要识别不同的请求
   * - 用于去重检查
   * - 保证请求的顺序性
   *
   * RequestId的递增：
   * - 每次发送请求前递增
   * - 保证RequestId单调递增
   * - 用于判断请求的新旧
   */
  int m_requestId;

  /**
   * @brief 最近已知的领导者ID
   * @details
   * 记录最近一次成功的请求对应的节点ID
   * 下次请求优先使用这个节点
   *
   * 为什么需要记录领导者：
   * - 避免每次都遍历所有节点
   * - 提高性能
   * - 减少RPC调用
   *
   * 为什么只是"可能"是领导者：
   * - 领导者可能变更
   * - 需要定期检查
   * - 如果收到"NotLeader"错误，切换到其他节点
   */
  int m_recentLeaderId;

  /**
   * @brief 生成随机的客户端ID
   * @return 随机的客户端ID（字符串形式）
   * @details
   *
   * 生成方法：
   * 1. 调用rand()生成随机数
   * 2. 将多个随机数转换为字符串
   * 3. 拼接成一个长字符串
   *
   * 为什么使用多个随机数：
   * - 增加随机性
   * - 减少碰撞概率
   * - 提高唯一性
   *
   * 注意：
   * - rand()不是真正的随机数生成器
   * - 在生产环境中应该使用更安全的随机数生成器
   * - 这里只是为了演示
   */
  std::string Uuid() {
    return std::to_string(rand()) + std::to_string(rand()) + std::to_string(rand()) + std::to_string(rand());
  }

  /**
   * @brief 内部方法，执行PUT或APPEND操作
   * @param key 键
   * @param value 值
   * @param op 操作类型（"Put"或"Append"）
   * @details
   *
   * PUT/APPEND操作流程：
   * 1. 递增m_requestId
   * 2. 构造请求参数（包含key、value、op、m_clientId、m_requestId）
   * 3. 向m_recentLeaderId对应的节点发送请求
   * 4. 如果收到"NotLeader"错误，切换到其他节点
   * 5. 重复步骤3-4，直到成功
   *
   * 为什么需要内部方法：
   * - PUT和APPEND的逻辑类似
   * - 避免代码重复
   * - 提高代码复用性
   */
  void PutAppend(std::string key, std::string value, std::string op);

 public:
  /**
   * @brief 初始化Clerk
   * @param configFileName 配置文件名
   * @details
   *
   * 初始化步骤：
   * 1. 读取配置文件
   * 2. 解析所有KVServer的地址
   * 3. 为每个KVServer创建RPC客户端
   * 4. 生成m_clientId
   * 5. 初始化m_requestId为0
   *
   * 配置文件格式：
   * 每行一个节点，格式为：ip:port
   * 例如：
   * 127.0.0.1:5000
   * 127.0.0.1:5001
   * 127.0.0.1:5002
   */
  void Init(std::string configFileName);

  /**
   * @brief 执行GET操作
   * @param key 键
   * @return 值
   * @details
   *
   * GET操作流程：
   * 1. 递增m_requestId
   * 2. 构造请求参数（包含key、m_clientId、m_requestId）
   * 3. 向m_recentLeaderId对应的节点发送请求
   * 4. 如果收到"NotLeader"错误，切换到其他节点
   * 5. 重复步骤3-4，直到成功
   * 6. 返回查询结果
   *
   * 注意：
   * - GET操作不会修改数据
   * - 但仍需要通过Raft达成共识
   * - 这是为了保证GET操作看到的是已提交的数据
   */
  std::string Get(std::string key);

  /**
   * @brief 执行PUT操作
   * @param key 键
   * @param value 值
   * @details
   *
   * PUT操作流程：
   * 1. 调用PutAppend(key, value, "Put")
   * 2. PutAppend内部处理重试和领导者发现
   *
   * PUT操作的作用：
   * - 将key设置为value
   * - 如果key已存在，覆盖旧值
   * - 如果key不存在，创建新键值对
   */
  void Put(std::string key, std::string value);

  /**
   * @brief 执行APPEND操作
   * @param key 键
   * @param value 值
   * @details
   *
   * APPEND操作流程：
   * 1. 调用PutAppend(key, value, "Append")
   * 2. PutAppend内部处理重试和领导者发现
   *
   * APPEND操作的作用：
   * - 将value追加到key对应的值后面
   * - 如果key不存在，相当于PUT操作
   */
  void Append(std::string key, std::string value);

 public:
  /**
   * @brief 构造函数
   * @details
   *
   * 初始化成员变量：
   * - m_requestId = 0
   * - m_recentLeaderId = 0
   *
   * 注意：
   * - 需要调用Init()方法初始化RPC客户端
   * - 构造函数只是初始化基本成员变量
   */
  Clerk();
};
}
#endif  // SKIP_LIST_ON_RAFT_CLERK_H
