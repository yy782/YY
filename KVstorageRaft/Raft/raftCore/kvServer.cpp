#include "kvServer.h"

#include <rpcprovider.h>
#include "include/Op.hpp"
#include "mprpcconfig.h"
#include "../../../include/util.hpp"
#include "../../../Common/ConfigCenter.h"
namespace yy
{
namespace  raft
{



//快照创建
// ┌─────────────────────────────────────────────────────────────────────────┐
// │                    快照创建的分工                                        │
// │                                                                         │
// │   KVServer                              Raft                            │
// │                                                                         │
// │   1. 检测日志太大                                                        │
// │      IfNeedToSendSnapShotCommand()                                      │
// │              │                                                          │
// │              ▼                                                          │
// │   2. 制作业务快照                                                        │
// │      MakeSnapShot()                                                     │
// │      - 序列化 m_skipList                                                │
// │      - 序列化 m_lastRequestId                                           │
// │      - 返回 snapshot 字符串                                              │
// │              │                                                          │
// │              │ 调用 Snapshot(index, snapshot)                           │
// │              ▼                                                          │
// │                                     3. 更新 Raft 状态                    │
// │                                        m_lastSnapshotIncludeIndex       │
// │                                        m_lastSnapshotIncludeTerm        │
// │                                              │                          │
// │                                              ▼                          │
// │                                     4. 删除旧日志                        │
// │                                        m_logs = 截断后的日志             │
// │                                              │                          │
// │                                              ▼                          │
// │                                     5. 保存到磁盘                        │
// │                                        m_persister->Save(               │
// │                                            raftstate,  // Raft 状态     │
// │                                            snapshot     // 业务快照     │
// │                                        )                                 │
// │                                                                         │
// └─────────────────────────────────────────────────────────────────────────┘


//节点重启
// ┌─────────────────────────────────────────────────────────────────────────┐
// │                    节点重启的恢复分工                                     │
// │                                                                         │
// │   KvServer                              Raft                            │
// │                                                                         │
// │   1. 读取快照文件                                                        │
// │      snapshot = persister->ReadSnapshot()                               │
// │              │                                                          │
// │              ▼                                                          │
// │   2. 恢复业务数据                                                        │
// │      ReadSnapShotToInstall(snapshot)                                    │
// │      - 恢复 m_skipList                                                  │
// │      - 恢复 m_lastRequestId                                             │
// │                                                                         │
// │                                     3. 读取 Raft 状态文件                 │
// │                                        data = persister->ReadRaftState()│
// │                                              │                          │
// │                                              ▼                          │
// │                                     4. 恢复 Raft 状态                    │
// │                                        readPersist(data)                │
// │                                        - m_currentTerm                  │
// │                                        - m_votedFor                     │
// │                                        - m_logs                         │
// │                                        - m_lastSnapshotIncludeIndex     │
// │                                        - m_lastSnapshotIncludeTerm      │
// │                                                                         │
// └─────────────────────────────────────────────────────────────────────────┘

// 快照传输
// ┌─────────────────────────────────────────────────────────────────────────┐
// │                    快照传输的分工                                        │
// │                                                                         │
// │   Leader                                   Follower                     │
// │                                                                         │
// │   Raft 层：                                                             │
// │   1. 读取快照文件                                                        │
// │      snapshot = m_persister->ReadSnapshot()                             │
// │              │                                                          │
// │              ▼                                                          │
// │   2. 发送 InstallSnapshot RPC                                           │
// │      args.set_data(snapshot)                                            │
// │              │                                                          │
// │              │ 网络传输                                                  │
// │              ▼                                                          │
// │                                      Raft 层：                           │
// │                                      3. 接收快照                         │
// │                                         InstallSnapshot()               │
// │                                              │                          │
// │                                              ▼                          │
// │                                      4. 保存到磁盘                       │
// │                                         m_persister->Save(...)          │
// │                                              │                          │
// │                                              ▼                          │
// │                                      5. 通知 KVServer                    │
// │                                         applyChan->Push(ApplyMsg{       │
// │                                             SnapshotValid = true,       │
// │                                             Snapshot = data             │
// │                                         })                              │
// │                                              │                          │
// │                                              ▼                          │
// │                                      KVServer 层：                       │
// │                                      6. 安装快照                         │
// │                                         ReadSnapShotToInstall()         │
// │                                         - 恢复业务数据                   │
// │                                                                         │
// └─────────────────────────────────────────────────────────────────────────|


void KvServer::DprintfKVDB() {
  if (!Debug) {
    return;
  }
  std::lock_guard<std::mutex> lg(m_mtx);
  DEFER {
    m_skipList.display_list();
  };
}

void KvServer::ExecuteAppendOpOnKVDB(Op op) {

  m_mtx.lock();
  m_skipList.insert_set_element(op.Key(), op.Value());
  m_lastRequestId[op.ClientId()] = op.RequestId();
  m_mtx.unlock();
  DprintfKVDB();
}

bool KvServer::tryGetOpOnKVDB(Op op, std::string *value) {
  m_mtx.lock();
  *value = "";
  bool exist = false;
  if (m_skipList.search_element(op.Key(), *value)) {
    exist = true;
  }
  m_lastRequestId[op.ClientId()] = op.RequestId();
  m_mtx.unlock();
  DprintfKVDB();
  return exist;
}

void KvServer::ExecutePutOpOnKVDB(Op op) {
  m_mtx.lock();
  m_skipList.insert_set_element(op.Key(), op.Value());
  m_lastRequestId[op.ClientId()] = op.RequestId();
  m_mtx.unlock();
  DprintfKVDB();
}

// 处理来自clerk的Get RPC
void KvServer::Get(const raftKVRpcProctoc::GetArgs *args, raftKVRpcProctoc::GetReply *reply) {
  Op op;
  op.Operation() = "Get";
  op.Key() = args->key();
  op.Value() = "";
  op.ClientId() = args->clientid();
  op.RequestId() = args->requestid();

  int raftIndex = -1;
  int _ = -1;
  bool isLeader = false;
  m_raftNode->Propose(op, &raftIndex, &_,
                    &isLeader);  // raftIndex：raft预计的logIndex
                                 // ，虽然是预计，但是正确情况下是准确的，op的具体内容对raft来说 是隔离的

  if (!isLeader) {
    reply->set_err(ErrWrongLeader);
    return;
  }


  LockQueue<Op> *chForRaftIndex = new LockQueue<Op>();
  {
    std::lock_guard<std::mutex> lg(m_mtx);
    myAssert(waitApplyCh.find(raftIndex) == waitApplyCh.end());
    waitApplyCh.insert(std::make_pair(raftIndex, chForRaftIndex));
  }

  // timeout
  Op raftCommitOp;

  if (!chForRaftIndex->timeOutPop(CONSENSUS_TIMEOUT, &raftCommitOp)) {
    int _ = -1;
    bool isLeader = false;
    m_raftNode->GetState(&_, &isLeader);

    if (
      (ifRequestDuplicate(op.ClientId(), op.RequestId()) && isLeader) ||
      (isLeader && raftCommitOp.ClientId() == op.ClientId() && raftCommitOp.RequestId() == op.RequestId())
      ) {
      //如果超时，代表raft集群不保证已经commitIndex该日志，但是如果是已经提交过的get请求，是可以再执行的。
      // 不会违反线性一致性
      std::string value;
      if (tryGetOpOnKVDB(op, &value)) {
        reply->set_err(OK);
        reply->set_value(value);
      } else {
        reply->set_err(ErrNoKey);
        reply->set_value("");
      }
    } else {
      reply->set_err(ErrWrongLeader);  //返回这个，其实就是让clerk换一个节点重试
    }
  }
  m_mtx.lock();  // todo 這個可以先弄一個defer，因爲刪除優先級並不高，先把rpc發回去更加重要
  auto tmp = waitApplyCh[raftIndex];
  waitApplyCh.erase(raftIndex);
  delete tmp;
  m_mtx.unlock();
}

void KvServer::GetCommandFromRaft(ApplyMsg message) {
  Op op;
  op.parseFromString(message.Command);

  DPrintf(
      "[KvServer::GetCommandFromRaft-kvserver{%d}] , Got Command --> Index:{%d} , ClientId {%s}, RequestId {%d}, "
      "Opreation {%s}, Key :{%s}, Value :{%s}",
      m_me, message.CommandIndex, &op.ClientId(), op.RequestId(), &op.Operation(), &op.Key(), &op.Value());
  if (message.CommandIndex <= m_lastSnapShotRaftLogIndex) {
    return;
  }
  if (!ifRequestDuplicate(op.ClientId(), op.RequestId())) {
    // execute command
    if (op.Operation() == "Put") {
      ExecutePutOpOnKVDB(op);
    }
    if (op.Operation() == "Append") {
      ExecuteAppendOpOnKVDB(op);
    }
  }
  CheckAndSendSnapShotCommand(message.CommandIndex, 9);
  // Send message to the chan of op.ClientId
  SendMessageToWaitChan(op, message.CommandIndex); // 唤醒RPC
}

bool KvServer::ifRequestDuplicate(std::string ClientId, int RequestId) {
  std::lock_guard<std::mutex> lg(m_mtx);
  if (m_lastRequestId.find(ClientId) == m_lastRequestId.end()) {
    return false;
    // todo :不存在这个client就创建
  }
  return RequestId <= m_lastRequestId[ClientId];
}

// get和put//append執行的具體細節是不一樣的
// PutAppend在收到raft消息之後執行，具體函數裏面只判斷冪等性（是否重複）
// get函數收到raft消息之後在，因爲get無論是否重複都可以再執行
void KvServer::PutAppend(const raftKVRpcProctoc::PutAppendArgs *args, raftKVRpcProctoc::PutAppendReply *reply) {
  Op op;
  op.Operation()= args->op();
  op.Key() = args->key();
  op.Value() = args->value();
  op.ClientId() = args->clientid();
  op.RequestId() = args->requestid();

  int raftIndex = -1;
  int _ = -1;
  bool isleader = false;
  m_raftNode->Propose(op, &raftIndex, &_, &isleader);

  if (!isleader) {
    DPrintf(
        "[func -KvServer::PutAppend -kvserver{%d}]From Client %s (Request %d) To Server %d, key %s, raftIndex %d , but "
        "not leader",
        m_me, &args->clientid(), args->requestid(), m_me, &op.Key(), raftIndex);

    reply->set_err(ErrWrongLeader);
    return;
  }

  DPrintf(
      "[func -KvServer::PutAppend -kvserver{%d}]From Client %s (Request %d) To Server %d, key %s, raftIndex %d , is "
      "leader ",
      m_me, &args->clientid(), args->requestid(), m_me, &op.Key(), raftIndex);
  LockQueue<Op> *chForRaftIndex =  new LockQueue<Op>();  

  {
    std::lock_guard<std::mutex> lg(m_mtx);
    myAssert(waitApplyCh.find(raftIndex) == waitApplyCh.end()); ///////////////////// chForRaftIndex内存泄漏
    waitApplyCh.insert(std::make_pair(raftIndex, chForRaftIndex)); // 这里应该只有一条消息，不用队列
  }

  // timeout
  Op raftCommitOp;

  if (!chForRaftIndex->timeOutPop(CONSENSUS_TIMEOUT, &raftCommitOp)) {
    DPrintf(
        "[func -KvServer::PutAppend -kvserver{%d}]TIMEOUT PUTAPPEND !!!! Server %d , get Command <-- Index:%d , "
        "ClientId %s, RequestId %s, Opreation %s Key :%s, Value :%s",
        m_me, m_me, raftIndex, &op.ClientId(), op.RequestId(), &op.Operation(), &op.Key(), &op.Value());

    if (ifRequestDuplicate(op.ClientId(), op.RequestId())) {
      reply->set_err(OK);  // 超时了,但因为是重复的请求，返回ok，实际上就算没有超时，在真正执行的时候也要判断是否重复
    } else {
      reply->set_err(ErrWrongLeader);  ///这里返回这个的目的让clerk重新尝试
    }
  } else {
    DPrintf(
        "[func -KvServer::PutAppend -kvserver{%d}]WaitChanGetRaftApplyMessage<--Server %d , get Command <-- Index:%d , "
        "ClientId %s, RequestId %d, Opreation %s, Key :%s, Value :%s",
        m_me, m_me, raftIndex, &op.ClientId(), op.RequestId(), &op.Operation(), &op.Key(), &op.Value());
    if (raftCommitOp.ClientId() == op.ClientId() && raftCommitOp.RequestId() == op.RequestId()) {
      //可能发生leader的变更导致日志被覆盖，因此必须检查
      reply->set_err(OK);
    } else {
      reply->set_err(ErrWrongLeader);
    }
  }
  std::lock_guard<std::mutex> lg(m_mtx);
  auto tmp = waitApplyCh[raftIndex];
  waitApplyCh.erase(raftIndex);
  delete tmp;
}

void KvServer::ReadRaftApplyCommandLoop() {
  while (true) {
    auto message = applyChan->Pop();
    DPrintf(
        "---------------tmp-------------[func-KvServer::ReadRaftApplyCommandLoop()-kvserver{%d}] 收到了下raft的消息",
        m_me);
    // listen to every command applied by its raft ,delivery to relative RPC Handler

    if (message.CommandValid) {
      GetCommandFromRaft(message);
    }
    if (message.SnapshotValid) {
      GetSnapShotFromRaft(message);
    }
  }
}

// raft会与persist层交互，kvserver层也会，因为kvserver层开始的时候需要恢复kvdb的状态
//  关于快照raft层与persist的交互：保存kvserver传来的snapshot；生成leaderInstallSnapshot RPC的时候也需要读取snapshot；
//  因此snapshot的具体格式是由kvserver层来定的，raft只负责传递这个东西
//  snapShot里面包含kvserver需要维护的persist_lastRequestId 以及kvDB真正保存的数据persist_kvdb
void KvServer::ReadSnapShotToInstall(std::string snapshot) {
  if (snapshot.empty()) {
    return;
  }
  parseFromString(snapshot);
}

bool KvServer::SendMessageToWaitChan(const Op &op, int raftIndex) {
  std::lock_guard<std::mutex> lg(m_mtx);
  DPrintf(
      "[RaftApplyMessageSendToWaitChan--> raftserver{%d}] , Send Command --> Index:{%d} , ClientId {%d}, RequestId "
      "{%d}, Opreation {%v}, Key :{%v}, Value :{%v}",
      m_me, raftIndex, &op.ClientId(), op.RequestId(), &op.Operation(), &op.Key(), &op.Value());

  if (waitApplyCh.find(raftIndex) == waitApplyCh.end()) {
    return false;
  }
  waitApplyCh[raftIndex]->Push(op);
  DPrintf(
      "[RaftApplyMessageSendToWaitChan--> raftserver{%d}] , Send Command --> Index:{%d} , ClientId {%d}, RequestId "
      "{%d}, Opreation {%v}, Key :{%v}, Value :{%v}",
      m_me, raftIndex, &op.ClientId(), op.RequestId(), &op.Operation(), &op.Key(), &op.Value());
  return true;
}

void KvServer::CheckAndSendSnapShotCommand(int raftIndex, int proportion) {
  if (m_raftNode->GetRaftStateSize() > m_maxRaftState / 10.0) {
    // Send SnapShot Command
    auto snapshot = MakeSnapShot();
    m_raftNode->Snapshot(raftIndex, snapshot);
  }
}

void KvServer::GetSnapShotFromRaft(ApplyMsg message) {
  std::lock_guard<std::mutex> lg(m_mtx);

  if (m_raftNode->CondInstallSnapshot(message.SnapshotTerm, message.SnapshotIndex, message.Snapshot)) {
    ReadSnapShotToInstall(message.Snapshot);
    m_lastSnapShotRaftLogIndex = message.SnapshotIndex;
  }
}

std::string KvServer::MakeSnapShot() {
  std::lock_guard<std::mutex> lg(m_mtx);
  std::string snapshotData = getSnapshotData();
  return snapshotData;
}

void KvServer::PutAppend(google::protobuf::RpcController *controller, const ::raftKVRpcProctoc::PutAppendArgs *request,
                         ::raftKVRpcProctoc::PutAppendReply *response, ::google::protobuf::Closure *done) {
  KvServer::PutAppend(request, response);
  done->Run();
}

void KvServer::Get(google::protobuf::RpcController *controller, const ::raftKVRpcProctoc::GetArgs *request,
                   ::raftKVRpcProctoc::GetReply *response, ::google::protobuf::Closure *done) {
  KvServer::Get(request, response);
  done->Run();
}

KvServer::KvServer(int me, int maxraftstate, std::string nodeInforFileName, short port) : 
m_skipList(6),
m_me(me),  
m_maxRaftState(maxraftstate),
applyChan(std::make_shared<LockQueue<ApplyMsg> >()),
m_raftNode(std::make_shared<Raft>()) 
{
  std::shared_ptr<Persister> persister = std::make_shared<Persister>(me);
  ////////////////clerk层面 kvserver开启rpc接受功能
  //    同时raft与raft节点之间也要开启rpc功能，因此有两个注册
  std::thread t([this, port]() -> void {
    // provider是一个rpc网络服务对象。把UserService对象发布到rpc节点上
    RpcProvider provider;
    provider.NotifyService(this);
    provider.NotifyService(
        this->m_raftNode.get());  // todo：这里获取了原始指针，后面检查一下有没有泄露的问题 或者 shareptr释放的问题
    // 启动一个rpc服务发布节点   Run以后，进程进入阻塞状态，等待远程的rpc调用请求
    provider.Run(m_me, port);
  });
  t.detach();

  ////开启rpc远程调用能力，需要注意必须要保证所有节点都开启rpc接受功能之后才能开启rpc远程调用能力
  ////这里使用睡眠来保证
  std::cout << "raftServer node:" << m_me << " start to sleep to wait all ohter raftnode start!!!!" << std::endl;
  sleep(6);
  std::cout << "raftServer node:" << m_me << " wake up!!!! start to connect other raftnode" << std::endl;
  //获取所有raft节点ip、port ，并进行连接  ,要排除自己
  Conf config;
  config.parse(nodeInforFileName.c_str());
  std::vector<std::pair<std::string, short> > ipPortVt;
  for (int i = 0; i < INT_MAX - 1; ++i) {
    std::string node = "node" + std::to_string(i);

    std::string nodeIp = config.get(node,"ip");
    std::string nodePortStr = config.get(node,"port");
    if (nodeIp.empty()) {
      break;
    }
    ipPortVt.emplace_back(nodeIp, atoi(nodePortStr.c_str()));  //沒有atos方法，可以考慮自己实现
  }
  std::vector<std::shared_ptr<RaftRpcUtil> > servers;
  //进行连接
  for (int i = 0; i < ipPortVt.size(); ++i) {
    if (i == m_me) {
      servers.push_back(nullptr);
      continue;
    }
    std::string otherNodeIp = ipPortVt[i].first;
    short otherNodePort = ipPortVt[i].second;
    auto *rpc = new RaftRpcUtil(otherNodeIp, otherNodePort);
    servers.push_back(std::shared_ptr<RaftRpcUtil>(rpc));

    std::cout << "node" << m_me << " 连接node" << i << "success!" << std::endl;
  }
  sleep(ipPortVt.size() - me);  //等待所有节点相互连接成功，再启动raft
  m_raftNode->init(servers, m_me, persister, applyChan);
  // kv的server直接与raft通信，但kv不直接与raft通信，所以需要把ApplyMsg的chan传递下去用于通信，两者的persist也是共用的

  auto snapshot = persister->ReadSnapshot();
  if (!snapshot.empty()) {
    ReadSnapShotToInstall(snapshot);
  }
  std::thread t2(&KvServer::ReadRaftApplyCommandLoop, this);  //马上向其他节点宣告自己就是leader
  t2.join();  //由於ReadRaftApplyCommandLoop一直不會結束，达到一直卡在这的目的
}
}
}