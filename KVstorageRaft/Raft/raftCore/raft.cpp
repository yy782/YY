#include "raft.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <memory>

#include "../../../include/util.hpp"

namespace yy
{
namespace raft 
{
void Raft::AppendEntries1(const raftRpcProctoc::AppendEntriesArgs* args, raftRpcProctoc::AppendEntriesReply* reply) {
  std::lock_guard<std::mutex> locker(m_mtx);
  reply->set_appstate(AppNormal); 
  if (args->term() < m_currentTerm) {
    reply->set_success(false);
    reply->set_term(m_currentTerm);
    reply->set_updatenextindex(-100); 
    DPrintf("[func-AppendEntries-rf{%d}] 拒绝了 因为Leader{%d}的term{%v}< rf{%d}.term{%d}\n", id_, args->leaderid(),
            args->term(), id_, m_currentTerm);
    return;  
  }
  else if (args->term() > m_currentTerm) {
    status_ = Follower;
    m_currentTerm = args->term();
    m_votedFor = -1;  
  }

  m_lastResetElectionTime = TimeStamp<LowPrecision>::now();
  if (args->prevlogindex() > getLastLogIndex()) {
    reply->set_success(false);
    reply->set_term(m_currentTerm);
    reply->set_updatenextindex(getLastLogIndex() + 1);
    return;
  }
  else if (matchLog(args->prevlogindex(), args->prevlogterm())) {
    for (int i = 0; i < args->entries_size(); i++) {
      auto log = args->entries(i);
      if (log.logindex() > getLastLogIndex()) {
        m_logs.push_back(log);
      } 
      else 
      { 
        if (m_logs[getSlicesIndexFromLogIndex(log.logindex())].logterm() != log.logterm()) {
          m_logs[getSlicesIndexFromLogIndex(log.logindex())] = log;
        }
      }
    }

    if (args->leadercommit() > m_commitIndex) {
      m_commitIndex = std::min(args->leadercommit(), getLastLogIndex());
      // 这个地方不能无脑跟上getLastLogIndex()，因为可能存在args->leadercommit()落后于 getLastLogIndex()的情况
    }
    reply->set_success(true);
    reply->set_term(m_currentTerm);

    return;
  } else {
    reply->set_updatenextindex(args->prevlogindex());
    reply->set_success(false);
    reply->set_term(m_currentTerm);
    return;
  }
}





std::vector<ApplyMsg> Raft::getApplyLogs() {
  std::vector<ApplyMsg> applyMsgs;
  myAssert(m_commitIndex <= getLastLogIndex(), format("[func-getApplyLogs-rf{%d}] commitIndex{%d} >getLastLogIndex{%d}",
                                                      id_, m_commitIndex, getLastLogIndex()));

  while (m_lastApplied < m_commitIndex) {
    m_lastApplied++;
    myAssert(m_logs[getSlicesIndexFromLogIndex(m_lastApplied)].logindex() == m_lastApplied,
             format("rf.logs[rf.getSlicesIndexFromLogIndex(rf.lastApplied)].LogIndex{%d} != rf.lastApplied{%d} ",
                    m_logs[getSlicesIndexFromLogIndex(m_lastApplied)].logindex(), m_lastApplied));
    ApplyMsg applyMsg;
    applyMsg.CommandValid = true;
    applyMsg.SnapshotValid = false;
    applyMsg.Command = m_logs[getSlicesIndexFromLogIndex(m_lastApplied)].command();
    applyMsg.CommandIndex = m_lastApplied;
    applyMsgs.emplace_back(applyMsg);
    //        DPrintf("[	applyLog func-rf{%v}	] apply Log,logIndex:%v  ，logTerm：{%v},command：{%v}\n",
    //        rf.me, rf.lastApplied, rf.logs[rf.getSlicesIndexFromLogIndex(rf.lastApplied)].LogTerm,
    //        rf.logs[rf.getSlicesIndexFromLogIndex(rf.lastApplied)].Command)
  }
  return applyMsgs;
}

// 获取新命令应该分配的Index
int Raft::getNewCommandIndex() {
  //	如果len(logs)==0,就为快照的index+1，否则为log最后一个日志+1
  auto lastLogIndex = getLastLogIndex();
  return lastLogIndex + 1;
}

// getPrevLogInfo
// leader调用，传入：服务器index，传出：发送的AE的preLogIndex和PrevLogTerm
void Raft::getPrevLogInfo(int server, int* preIndex, int* preTerm) {
  auto nextIndex = m_nextIndex[server];
  *preIndex = nextIndex - 1;
  *preTerm = m_logs[getSlicesIndexFromLogIndex(*preIndex)].logterm();
}

// GetState return currentTerm and whether this server
// believes it is the Leader.
void Raft::GetState(int* term, bool* isLeader) {
  m_mtx.lock();
  DEFER {
    // todo 暂时不清楚会不会导致死锁
    m_mtx.unlock();
  };

  // Your code here (2A).
  *term = m_currentTerm;
  *isLeader = (status_ == Leader);
}

void Raft::pushMsgToKvServer(ApplyMsg msg) { applyChan->Push(msg); }


void Raft::leaderUpdateCommitIndex() {
}

//进来前要保证logIndex是存在的，即≥rf.lastSnapshotIncludeIndex	，而且小于等于rf.getLastLogIndex()
bool Raft::matchLog(int logIndex, int logTerm) {
  return logTerm == getLogTermFromLogIndex(logIndex);
}

void Raft::persist() {
  // Your code here (2C).
  auto data = persistData();
  m_persister->SaveRaftState(data);
  // fmt.Printf("RaftNode[%d] persist starts, currentTerm[%d] voteFor[%d] log[%v]\n", rf.me, rf.currentTerm,
  // rf.votedFor, rf.logs) fmt.Printf("%v\n", string(data))
}


int Raft::GetRaftStateSize() { return m_persister->RaftStateSize(); }


bool Raft::checkDataToCandidate(int index, int term) {
  int lastIndex = -1;
  int lastTerm = -1;
  getLastLogIndexAndTerm(&lastIndex, &lastTerm);
  return term > lastTerm || (term == lastTerm && index >= lastIndex);
}



void Raft::getLastLogIndexAndTerm(int* lastLogIndex, int* lastLogTerm) {
    *lastLogIndex = m_logs[m_logs.size() - 1].logindex();
    *lastLogTerm = m_logs[m_logs.size() - 1].logterm();
}

int Raft::getLastLogIndex() {
  int lastLogIndex = -1;
  int _ = -1;
  getLastLogIndexAndTerm(&lastLogIndex, &_);
  return lastLogIndex;
}

int Raft::getLastLogTerm() {
  int _ = -1;
  int lastLogTerm = -1;
  getLastLogIndexAndTerm(&_, &lastLogTerm);
  return lastLogTerm;
}

int Raft::getLogTermFromLogIndex(int logIndex) {


  int lastLogIndex = getLastLogIndex();

  myAssert(logIndex <= lastLogIndex, format("[func-getSlicesIndexFromLogIndex-rf{%d}]  logIndex{%d} > lastLogIndex{%d}",
                                            id_, logIndex, lastLogIndex));


    return m_logs[getSlicesIndexFromLogIndex(logIndex)].logterm();
  
}

int Raft::getSlicesIndexFromLogIndex(int logIndex) {
  int lastLogIndex = getLastLogIndex();
  myAssert(logIndex <= lastLogIndex, format("[func-getSlicesIndexFromLogIndex-rf{%d}]  logIndex{%d} > lastLogIndex{%d}",
                                            id_, logIndex, lastLogIndex));
  int SliceIndex = logIndex  - 1;
  return SliceIndex;
}


void Raft::RequestVote(const raftRpcProctoc::RequestVoteArgs* args, raftRpcProctoc::RequestVoteReply* reply) {
  std::lock_guard<std::mutex> lg(m_mtx);

  // Your code here (2A, 2B).
  DEFER {
    //应该先持久化，再撤销lock
    persist();
  };
  //对args的term的三种情况分别进行处理，大于小于等于自己的term都是不同的处理
  // reason: 出现网络分区，该竞选者已经OutOfDate(过时）
  if (args->term() < m_currentTerm) {
    reply->set_term(m_currentTerm);
    reply->set_votestate(Expire);
    reply->set_votegranted(false);
    return;
  }
  // fig2:右下角，如果任何时候rpc请求或者响应的term大于自己的term，更新term，并变成follower
  if (args->term() > m_currentTerm) {
    //        DPrintf("[	    func-RequestVote-rf(%v)		] : 变成follower且更新term
    //        因为candidate{%v}的term{%v}> rf{%v}.term{%v}\n ", rf.me, args.CandidateId, args.Term, rf.me,
    //        rf.currentTerm)
    status_ = Follower;
    m_currentTerm = args->term();
    m_votedFor = -1;

    //	重置定时器：收到leader的ae，开始选举，透出票
    //这时候更新了term之后，votedFor也要置为-1
  }
  myAssert(args->term() == m_currentTerm,
           format("[func--rf{%d}] 前面校验过args.Term==rf.currentTerm，这里却不等", id_));
  //	现在节点任期都是相同的(任期小的也已经更新到新的args的term了)，还需要检查log的term和index是不是匹配的了

  int lastLogTerm = getLastLogTerm();
  //只有没投票，且candidate的日志的新的程度 ≥ 接受者的日志新的程度 才会授票
  if (!checkDataToCandidate(args->lastlogindex(), args->lastlogterm())) {
    // args.LastLogTerm < lastLogTerm || (args.LastLogTerm == lastLogTerm && args.LastLogIndex < lastLogIndex) {
    //日志太旧了
    if (args->lastlogterm() < lastLogTerm) {
      //                    DPrintf("[	    func-RequestVote-rf(%v)		] : refuse voted rf[%v] ,because
      //                    candidate_lastlog_term{%v} < lastlog_term{%v}\n", rf.me, args.CandidateId, args.LastLogTerm,
      //                    lastLogTerm)
    } else {
      //            DPrintf("[	    func-RequestVote-rf(%v)		] : refuse voted rf[%v] ,because
      //            candidate_log_index{%v} < log_index{%v}\n", rf.me, args.CandidateId, args.LastLogIndex,
      //            rf.getLastLogIndex())
    }
    reply->set_term(m_currentTerm);
    reply->set_votestate(Voted);
    reply->set_votegranted(false);

    return;
  }
  // todo ： 啥时候会出现rf.votedFor == args.CandidateId ，就算candidate选举超时再选举，其term也是不一样的呀
  //     当因为网络质量不好导致的请求丢失重发就有可能！！！！
  if (m_votedFor != -1 && m_votedFor != args->candidateid()) {
    //        DPrintf("[	    func-RequestVote-rf(%v)		] : refuse voted rf[%v] ,because has voted\n",
    //        rf.me, args.CandidateId)
    reply->set_term(m_currentTerm);
    reply->set_votestate(Voted);
    reply->set_votegranted(false);

    return;
  } else {
    m_votedFor = args->candidateid();
    m_lastResetElectionTime = now();  //认为必须要在投出票的时候才重置定时器，
    //        DPrintf("[	    func-RequestVote-rf(%v)		] : voted rf[%v]\n", rf.me, rf.votedFor)
    reply->set_term(m_currentTerm);
    reply->set_votestate(Normal);
    reply->set_votegranted(true);

    return;
  }
}

bool Raft::AsyncSendRequestVote(int server, std::shared_ptr<raftRpcProctoc::RequestVoteArgs> args,
                           std::shared_ptr<raftRpcProctoc::RequestVoteReply> reply) {
  bool ok = m_peers[server]->RequestVote(args.get(), reply.get());
  return true;
}

bool Raft::SendAppendEntries(int server, std::shared_ptr<raftRpcProctoc::AppendEntriesArgs> args,
                             std::shared_ptr<raftRpcProctoc::AppendEntriesReply> reply,
                             std::shared_ptr<int> appendNums) {
  //这个ok是网络是否正常通信的ok，而不是requestVote rpc是否投票的rpc
  // 如果网络不通的话肯定是没有返回的，不用一直重试
  // todo： paper中5.3节第一段末尾提到，如果append失败应该不断的retries ,直到这个log成功的被store
  DPrintf("[func-Raft::sendAppendEntries-raft{%d}] leader 向节点{%d}发送AE rpc開始 ， args->entries_size():{%d}", id_,
          server, args->entries_size());
  bool ok = m_peers[server]->AppendEntries(args.get(), reply.get());

  if (!ok) {
    DPrintf("[func-Raft::sendAppendEntries-raft{%d}] leader 向节点{%d}发送AE rpc失敗", id_, server);
    return ok;
  }
  DPrintf("[func-Raft::sendAppendEntries-raft{%d}] leader 向节点{%d}发送AE rpc成功", id_, server);
  if (reply->appstate() == Disconnected) {
    return ok;
  }
  std::lock_guard<std::mutex> lg1(m_mtx);

  //对reply进行处理
  // 对于rpc通信，无论什么时候都要检查term
  if (reply->term() > m_currentTerm) {
    status_ = Follower;
    m_currentTerm = reply->term();
    m_votedFor = -1;
    return ok;
  } else if (reply->term() < m_currentTerm) {
    DPrintf("[func -sendAppendEntries  rf{%d}]  节点：{%d}的term{%d}<rf{%d}的term{%d}\n", id_, server, reply->term(),
            id_, m_currentTerm);
    return ok;
  }

  if (status_ != Leader) {
    //如果不是leader，那么就不要对返回的情况进行处理了
    return ok;
  }
  // term相等

  myAssert(reply->term() == m_currentTerm,
           format("reply.Term{%d} != rf.currentTerm{%d}   ", reply->term(), m_currentTerm));
  if (!reply->success()) {
    //日志不匹配，正常来说就是index要往前-1，既然能到这里，第一个日志（idnex =
    // 1）发送后肯定是匹配的，因此不用考虑变成负数 因为真正的环境不会知道是服务器宕机还是发生网络分区了
    if (reply->updatenextindex() != -100) {
      // todo:待总结，就算term匹配，失败的时候nextIndex也不是照单全收的，因为如果发生rpc延迟，leader的term可能从不符合term要求
      //变得符合term要求
      //但是不能直接赋值reply.UpdateNextIndex
      DPrintf("[func -sendAppendEntries  rf{%d}]  返回的日志term相等，但是不匹配，回缩nextIndex[%d]：{%d}\n", id_,
              server, reply->updatenextindex());
      m_nextIndex[server] = reply->updatenextindex();  //失败是不更新mathIndex的
    }
    //	怎么越写越感觉rf.nextIndex数组是冗余的呢，看下论文fig2，其实不是冗余的
  } else {
    *appendNums = *appendNums + 1;
    DPrintf("---------------------------tmp------------------------- 節點{%d}返回true,當前*appendNums{%d}", server,
            *appendNums);
    // rf.matchIndex[server] = len(args.Entries) //只要返回一个响应就对其matchIndex应该对其做出反应，
    //但是这么修改是有问题的，如果对某个消息发送了多遍（心跳时就会再发送），那么一条消息会导致n次上涨
    m_matchIndex[server] = std::max(m_matchIndex[server], args->prevlogindex() + args->entries_size());
    m_nextIndex[server] = m_matchIndex[server] + 1;
    int lastLogIndex = getLastLogIndex();

    myAssert(m_nextIndex[server] <= lastLogIndex + 1,
             format("error msg:rf.nextIndex[%d] > lastLogIndex+1, len(rf.logs) = %d   lastLogIndex{%d} = %d", server,
                    m_logs.size(), server, lastLogIndex));
    if (*appendNums >= 1 + m_peers.size() / 2) {
      //可以commit了
      //两种方法保证幂等性，1.赋值为0 	2.上面≥改为==

      *appendNums = 0;
      // todo https://578223592-laughing-halibut-wxvpggvw69qh99q4.github.dev/ 不断遍历来统计rf.commitIndex
      //改了好久！！！！！
      // leader只有在当前term有日志提交的时候才更新commitIndex，因为raft无法保证之前term的Index是否提交
      //只有当前term有日志提交，之前term的log才可以被提交，只有这样才能保证“领导人完备性{当选领导人的节点拥有之前被提交的所有log，当然也可能有一些没有被提交的}”
      // rf.leaderUpdateCommitIndex()
      if (args->entries_size() > 0) {
        DPrintf("args->entries(args->entries_size()-1).logterm(){%d}   m_currentTerm{%d}",
                args->entries(args->entries_size() - 1).logterm(), m_currentTerm);
      }
      if (args->entries_size() > 0 && args->entries(args->entries_size() - 1).logterm() == m_currentTerm) {
        DPrintf(
            "---------------------------tmp------------------------- 當前term有log成功提交，更新leader的m_commitIndex "
            "from{%d} to{%d}",
            m_commitIndex, args->prevlogindex() + args->entries_size());

        m_commitIndex = std::max(m_commitIndex, args->prevlogindex() + args->entries_size());
      }
      myAssert(m_commitIndex <= lastLogIndex,
               format("[func-sendAppendEntries,rf{%d}] lastLogIndex:%d  rf.commitIndex:%d\n", id_, lastLogIndex,
                      m_commitIndex));
      // fmt.Printf("[func-sendAppendEntries,rf{%v}] len(rf.logs):%v  rf.commitIndex:%v\n", rf.me, len(rf.logs),
      // rf.commitIndex)
    }
  }
  return ok;
}


void Raft::AppendEntries(google::protobuf::RpcController* controller,
                         const ::raftRpcProctoc::AppendEntriesArgs* request,
                         ::raftRpcProctoc::AppendEntriesReply* response, ::google::protobuf::Closure* done) {
  AppendEntries1(request, response);
  done->Run();
}


void Raft::RequestVote(google::protobuf::RpcController* controller, const ::raftRpcProctoc::RequestVoteArgs* request,
                       ::raftRpcProctoc::RequestVoteReply* response, ::google::protobuf::Closure* done) {
  RequestVote(request, response);
  done->Run();
}




void Raft::Propose(Op command, int* newLogIndex, int* newLogTerm, bool* isLeader) {
  std::lock_guard<std::mutex> lg1(m_mtx);
  if (status_ != Leader) {
    DPrintf("[func-Start-rf{%d}]  is not leader");
    *newLogIndex = -1;
    *newLogTerm = -1;
    *isLeader = false;
    return;
  }

  raftRpcProctoc::LogEntry newLogEntry;
  newLogEntry.set_command(command.asString());
  newLogEntry.set_logterm(m_currentTerm);
  newLogEntry.set_logindex(getNewCommandIndex());
  m_logs.emplace_back(newLogEntry);

  persist();
  *newLogIndex = newLogEntry.logindex();
  *newLogTerm = newLogEntry.logterm();
  *isLeader = true;
}

void Raft::init(std::vector<std::shared_ptr<RaftRpcUtil>> peers, int id, std::shared_ptr<Persister> persister,
                std::shared_ptr<LockQueue<ApplyMsg>> applyCh) {
  m_peers = peers;
  m_persister = persister;
  id_ = id;
  // Your initialization code here (2A, 2B, 2C).
  m_mtx.lock();
  applyChan = applyCh;
  
  m_currentTerm = 0;
  status_ = Follower;
  m_commitIndex = 0;
  m_lastApplied = 0;
  m_logs.clear();
  for (int i = 0; i < m_peers.size(); i++) {
    m_matchIndex.push_back(0);
    m_nextIndex.push_back(0);
  }
  m_votedFor = -1;

  m_lastResetElectionTime = TimeStamp<LowPrecision>::now();
  m_lastResetHearBeatTime = TimeStamp<LowPrecision>::now();

  readPersist(m_persister->ReadRaftState());


  m_mtx.unlock();

  //m_ioManager = std::make_unique<yy::IOManager>(FIBER_THREAD_NUM, FIBER_USE_CALLER_THREAD);

  // start ticker fiber to start elections
  // 启动三个循环定时器
  // todo:原来是启动了三个线程，现在是直接使用了协程，三个函数中leaderHearBeatTicker
  // 、electionTimeOutTicker执行时间是恒定的，applierTicker时间受到数据库响应延迟和两次apply之间请求数量的影响，这个随着数据量增多可能不太合理，最好其还是启用一个线程。
  // m_ioManager->scheduler([this]() -> void { this->leaderHearBeatTicker(); });
  //m_ioManager->scheduler([this]() -> void { this->electionTimeOutTicker(); });

  // std::thread t3(&Raft::applierTicker, this);
  // t3.detach();

  // std::thread t(&Raft::leaderHearBeatTicker, this);
  // t.detach();
  //
  // std::thread t2(&Raft::electionTimeOutTicker, this);
  // t2.detach();
  //
  // std::thread t3(&Raft::applierTicker, this);
  // t3.detach();

  thread_.run([this](){run();});
}

std::string Raft::persistData() {
  BoostPersistRaftNode boostPersistRaftNode;
  boostPersistRaftNode.m_currentTerm = m_currentTerm;
  boostPersistRaftNode.m_votedFor = m_votedFor;
  for (auto& item : m_logs) {
    boostPersistRaftNode.m_logs.push_back(item.SerializeAsString());
  }

  std::stringstream ss;
  boost::archive::text_oarchive oa(ss);
  oa << boostPersistRaftNode;
  return ss.str();
}

void Raft::readPersist(std::string data) {
  if (data.empty()) {
    return;
  }
  std::stringstream iss(data);
  boost::archive::text_iarchive ia(iss);
  // read class state from archive
  BoostPersistRaftNode boostPersistRaftNode;
  ia >> boostPersistRaftNode;

  m_currentTerm = boostPersistRaftNode.m_currentTerm;
  m_votedFor = boostPersistRaftNode.m_votedFor;
  m_logs.clear();
  for (auto& item : boostPersistRaftNode.m_logs) {
    raftRpcProctoc::LogEntry logEntry;
    logEntry.ParseFromString(item);
    m_logs.emplace_back(logEntry);
  }
}


Raft::Status Raft::checkStatus()
{
  return status_;
}

// Leader 

void Raft::applierTicker() {
  while (true) {
    m_mtx.lock();
    if (status_ == Leader) {
      DPrintf("[Raft::applierTicker() - raft{%d}]  m_lastApplied{%d}   m_commitIndex{%d}", id_, m_lastApplied,
              m_commitIndex);
    }
    auto applyMsgs = getApplyLogs();
    m_mtx.unlock();
    //使用匿名函数是因为传递管道的时候不用拿锁
    // todo:好像必须拿锁，因为不拿锁的话如果调用多次applyLog函数，可能会导致应用的顺序不一样
    if (!applyMsgs.empty()) {
      DPrintf("[func- Raft::applierTicker()-raft{%d}] 向kvserver報告的applyMsgs長度爲：{%d}", id_, applyMsgs.size());
    }
    for (auto& message : applyMsgs) {
      applyChan->Push(message);
    }
    // usleep(1000 * ApplyInterval);
    sleepNMilliseconds(ApplyInterval);
  }
}

void Raft::doHeartBeat() {
  std::lock_guard<std::mutex> g(m_mtx);
  myAssert(status_ == Leader, 
    format("[func-Raft::doHeartBeat()-raft{%d}] 只有leader的心跳定时器才会触发发送心跳，当前状态{%d}异常了", id_, status_));

    DPrintf("[func-Raft::doHeartBeat()-Leader: {%d}] Leader的心跳定时器触发了且拿到mutex，开始发送AE\n", id_);
    //auto appendNums = std::make_shared<int>(1);  //正确返回的节点的数量

    for (int i = 0; i < m_peers.size(); i++) {
      if (i == id_) {continue;}
      DPrintf("[func-Raft::doHeartBeat()-Leader: {%d}] Leader的心跳定时器触发了 index:{%d}\n", id_, i);
      myAssert(m_nextIndex[i] >= 1, format("rf.nextIndex[%d] = {%d}", i, m_nextIndex[i]));
      //日志压缩加入后要判断是发送快照还是发送AE
        //构造发送值
        int preLogIndex = -1;
        int PrevLogTerm = -1;
        getPrevLogInfo(i, &preLogIndex, &PrevLogTerm);
        std::shared_ptr<raftRpcProctoc::AppendEntriesArgs> appendEntriesArgs =
            std::make_shared<raftRpcProctoc::AppendEntriesArgs>();
        appendEntriesArgs->set_term(m_currentTerm);
        appendEntriesArgs->set_leaderid(id_);
        appendEntriesArgs->set_prevlogindex(preLogIndex);
        appendEntriesArgs->set_prevlogterm(PrevLogTerm);
        appendEntriesArgs->clear_entries();
        appendEntriesArgs->set_leadercommit(m_commitIndex);
 
          for (const auto& item : m_logs) {
            raftRpcProctoc::LogEntry* sendEntryPtr = appendEntriesArgs->add_entries();
            *sendEntryPtr = item;  //=是可以点进去的，可以点进去看下protobuf如何重写这个的
          
        }
        int lastLogIndex = getLastLogIndex();
        // leader对每个节点发送的日志长短不一，但是都保证从prevIndex发送直到最后
        myAssert(appendEntriesArgs->prevlogindex() + appendEntriesArgs->entries_size() == lastLogIndex,
                format("appendEntriesArgs.PrevLogIndex{%d}+len(appendEntriesArgs.Entries){%d} != lastLogIndex{%d}",
                        appendEntriesArgs->prevlogindex(), appendEntriesArgs->entries_size(), lastLogIndex));
        //构造返回值
        const std::shared_ptr<raftRpcProctoc::AppendEntriesReply> appendEntriesReply =
            std::make_shared<raftRpcProctoc::AppendEntriesReply>();
        appendEntriesReply->set_appstate(Disconnected);

        // std::thread t(&Raft::sendAppendEntries, this, i, appendEntriesArgs, appendEntriesReply,
        //               appendNums);  // 创建新线程并执行b函数，并传递参数
        // t.detach();
        // 协程来发送心跳
      }
      m_lastResetHearBeatTime = TimeStamp<LowPrecision>::now();  
    
  
}

// Candidate
void Raft::doElection() {
  std::lock_guard<std::mutex> g(m_mtx);
    m_currentTerm += 1;
    m_lastResetElectionTime = TimeStamp<LowPrecision>::now();
    m_votedFor = id_; 
    persist();
    std::shared_ptr<int> votedNum = std::make_shared<int>(1);  // 使用 make_shared 函数初始化 !! 亮点
    std::vector<std::shared_ptr<raftRpcProctoc::RequestVoteReply>> requestVoteReplies(m_peers.size());
    for (int i = 0; i < m_peers.size(); i++) {
      if (i == id_) {continue;}
      int lastLogIndex = -1, lastLogTerm = -1;
      getLastLogIndexAndTerm(&lastLogIndex, &lastLogTerm);  //获取最后一个log的term和下标

      std::shared_ptr<raftRpcProctoc::RequestVoteArgs> requestVoteArgs =
          std::make_shared<raftRpcProctoc::RequestVoteArgs>();
      requestVoteArgs->set_term(m_currentTerm);
      requestVoteArgs->set_candidateid(id_);
      requestVoteArgs->set_lastlogindex(lastLogIndex);
      requestVoteArgs->set_lastlogterm(lastLogTerm);
      auto requestVoteReply = std::make_shared<raftRpcProctoc::RequestVoteReply>();
      AsyncSendRequestVote(i, requestVoteArgs, requestVoteReply);
  }
}



// Follower
void Raft::checkAndVerbCandidate() {
  status_ = Candidate;
}


void Raft::run()
{
  while(isRunning_)
  {
    switch(checkStatus())// 状态必须变换
    {
      case Leader:
        //doHeartBeat();
        break;
      case Follower:
        //checkElectionTimeOut();
        break;
      case Candidate:
        //checkElectionTimeOut();
        break;
    }
  }
}

void Raft::doLeaderWork()
{
  asyncDoHeartBeat();
  applierTicker();
}


void Raft::doFollowerWork()
{
  checkAndVerbCandidate();
}
void Raft::doCandidateWork()
{

}




}
}