#include "raft.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <memory>


#include "../../../include/util.hpp"

namespace yy
{
namespace raft 
{

constexpr int invalidLog=-100;

void Raft::AppendEntries1(const raftRpcProctoc::AppendEntriesArgs* args, raftRpcProctoc::AppendEntriesReply* reply) {
  std::lock_guard<std::mutex> locker(m_mtx);
  reply->set_appstate(AppNormal); 
  if (args->term() < currentTerm_) {
    reply->set_success(false);
    reply->set_term(currentTerm_);
    reply->set_updatenextindex(invalidLog); 
    return;  
  }
  else if (args->term() > currentTerm_) {
    status_ = Follower;
    currentTerm_ = args->term();
    votedFor_ = -1;  
  }
  if (args->prevlogindex() > getLastLogIndexAndTerm()[0]) {
    reply->set_success(false);
    reply->set_term(currentTerm_);
    reply->set_updatenextindex(getLastLogIndexAndTerm()[0] + 1);
    return;
  }
  else if (matchLog(args->prevlogindex(), args->prevlogterm())) {
    for (int i = 0; i < args->entries_size(); i++) {
      auto log = args->entries(i);
      if (log.logindex() > getLastLogIndexAndTerm()[0]) {
        logs_.push_back(log);
      } 
      else 
      { 
        if (logs_[getSlicesIndexFromLogIndex(log.logindex())].logterm() != log.logterm()) {
          logs_[getSlicesIndexFromLogIndex(log.logindex())] = log;
        }
      }
    }

    if (args->leadercommit() > commitedMaxIndex_) {
      commitedMaxIndex_ = std::min(args->leadercommit(), getLastLogIndexAndTerm()[0]);
      // 这个地方不能无脑跟上getLastLogIndex()，因为可能存在args->leadercommit()落后于 getLastLogIndex()的情况
    }
    reply->set_success(true);
    reply->set_term(currentTerm_);

    return;
  } else {
    reply->set_updatenextindex(args->prevlogindex());
    reply->set_success(false);
    reply->set_term(currentTerm_);
    return;
  }
}

std::vector<ApplyMsg> Raft::getApplyLogs() {
  std::vector<ApplyMsg> applyMsgs;
  myAssert(commitedMaxIndex_ <= getLastLogIndexAndTerm()[0], format("[func-getApplyLogs-rf{%d}] commitIndex{%d} >getLastLogIndex{%d}",
                                                      id_, commitedMaxIndex_, getLastLogIndexAndTerm()[0]   ));

  while (AppliedMaxIndex_ < commitedMaxIndex_) {
    AppliedMaxIndex_++;
    ApplyMsg applyMsg;
    applyMsg.CommandValid = true;
    applyMsg.SnapshotValid = false;
    applyMsg.Command = logs_[getSlicesIndexFromLogIndex(AppliedMaxIndex_)].command();
    applyMsg.CommandIndex = AppliedMaxIndex_;
    applyMsgs.emplace_back(applyMsg);
  }
  return applyMsgs;
}

// 获取新命令应该分配的Index
int Raft::getNewCommandIndex() {
  //	如果len(logs)==0,就为快照的index+1，否则为log最后一个日志+1
  auto lastLogIndex = getLastLogIndexAndTerm()[0];
  return lastLogIndex + 1;
}

// getPrevLogInfo
// leader调用，传入：服务器index，传出：发送的AE的preLogIndex和PrevLogTerm
void Raft::getPrevLogInfo(int server, int* preIndex, int* preTerm) {
  auto nextIndex = nextIndex_[server];
  *preIndex = nextIndex - 1;
  *preTerm = logs_[getSlicesIndexFromLogIndex(*preIndex)].logterm();
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
  *term = currentTerm_;
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
  auto array=getLastLogIndexAndTerm();
  return term > array[1] || (term == array[1] && index >= array[0]);
}



std::array<int, 2> Raft::getLastLogIndexAndTerm() {
  std::array<int, 2> result;
  result[0] = logs_[logs_.size() - 1].logindex();
  result[1] = logs_[logs_.size() - 1].logterm();
  return result;
}

int Raft::getLogTermFromLogIndex(int logIndex) {
  return logs_[getSlicesIndexFromLogIndex(logIndex)].logterm();
  
}

int Raft::getSlicesIndexFromLogIndex(int logIndex) {
  int SliceIndex = logIndex  - 1;
  return SliceIndex;
}


void Raft::RequestVote(const raftRpcProctoc::RequestVoteArgs* args, raftRpcProctoc::RequestVoteReply* reply) {
  std::lock_guard<std::mutex> lg(m_mtx);
  persist();
  if (args->term() < currentTerm_) {
    reply->set_term(currentTerm_);
    reply->set_votestate(Expire);
    reply->set_votegranted(false);
  }
  else if (args->term() > currentTerm_) {
    status_ = Follower;
    currentTerm_ = args->term();
    votedFor_ = -1;
    auto lastLogIndexAndTerm = getLastLogIndexAndTerm();
    int lastLogTerm = lastLogIndexAndTerm[1];
    //只有没投票，且candidate的日志的新的程度 ≥ 接受者的日志新的程度 才会授票
    if (!checkDataToCandidate(args->lastlogindex(), args->lastlogterm())) {
      reply->set_term(currentTerm_);
      reply->set_votestate(LogExpire);
      reply->set_votegranted(false);
    }
    else if (votedFor_ != -1 && votedFor_ != args->candidateid()) {
      reply->set_term(currentTerm_);
      reply->set_votestate(Voted);
      reply->set_votegranted(false);
    } else {
      votedFor_ = args->candidateid();
      reply->set_term(currentTerm_);
      reply->set_votestate(Normal);
      reply->set_votegranted(true);
    }    
  }
  else 
  {
    // 我也是候选人，拒绝投票
    myAssert(status_ == Candidate, 
      format("[func-RequestVote-rf{%d}]  term相等了，但是状态不是Candidate，而是{%d}", id_, status_));
    reply->set_term(currentTerm_);
    reply->set_votestate(Voted); // 投给自己了，拒绝投票
    reply->set_votegranted(false);
  }

}




bool Raft::AsyncSendRequestVote(int server, std::shared_ptr<raftRpcProctoc::RequestVoteArgs> args,
                           std::shared_ptr<raftRpcProctoc::RequestVoteReply> reply) {
  bool ok = m_peers[server]->RequestVote(args.get(), reply.get());
  return true;
}
bool Raft::SendAppendEntries(int server, std::shared_ptr<raftRpcProctoc::AppendEntriesArgs> args,
                             std::shared_ptr<raftRpcProctoc::AppendEntriesReply> reply) {
  bool ok = m_peers[server]->AppendEntries(args.get(), reply.get());

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
  newLogEntry.set_logterm(currentTerm_);
  newLogEntry.set_logindex(getNewCommandIndex());

  sendAndSyncAppendEntries(newLogEntry);

  persist();
  *newLogIndex = newLogEntry.logindex();
  *newLogTerm = newLogEntry.logterm();
  *isLeader = true;
}

void Raft::sendAndSyncAppendEntries(const raftRpcProctoc::LogEntry& newLogEntry) {
  logs_.push_back(newLogEntry);
  std::vector<std::shared_ptr<raftRpcProctoc::AppendEntriesReply>> appendEntriesReplies(m_peers.size(),
    std::make_shared<raftRpcProctoc::AppendEntriesReply>()
  );
  std::vector<std::shared_ptr<raftRpcProctoc::AppendEntriesArgs>> appendEntriesArgsList(m_peers.size(),
    std::make_shared<raftRpcProctoc::AppendEntriesArgs>()
  );  
  for(int i = 0; i < m_peers.size(); i++) {
    if (i == id_) {continue;}
    auto appendEntriesArgs = appendEntriesArgsList[i];
        
    appendEntriesArgs->set_term(currentTerm_);
    appendEntriesArgs->set_leaderid(id_);
    int PrevLogTerm = -1;
    int PrevLogIndex = -1;
    getPrevLogInfo(i, &PrevLogIndex, &PrevLogTerm);
    appendEntriesArgs->set_prevlogindex(PrevLogIndex);
    appendEntriesArgs->set_prevlogterm(PrevLogTerm);
    *appendEntriesArgs->add_entries() = newLogEntry;
    appendEntriesArgs->set_leadercommit(commitedMaxIndex_);
    appendEntriesArgs->entries_size();
    SendAppendEntries(i, appendEntriesArgs, appendEntriesReplies[i]);
  }
  // 挂起 等待所有AE RPC的回复

  int appendNums = 0;
  for (int server = 0; server < m_peers.size(); server++) {
    if (server == id_) {continue;}
    auto reply = appendEntriesReplies[server];
    // reply可能是空，没有回应

    std::lock_guard<std::mutex> lg1(m_mtx);

  if (reply->term() > currentTerm_) {
    status_ = Follower;
    currentTerm_ = reply->term();
    votedFor_ = -1;
    break;
  } 
  else if (!reply->success()&&reply->updatenextindex() != invalidLog) {
      nextIndex_[server] = reply->updatenextindex();
  } else {
    auto args = appendEntriesArgsList[server];
    appendNums += 1;
    matchIndex_[server] = std::max(matchIndex_[server], args->prevlogindex() + args->entries_size());
    nextIndex_[server] = matchIndex_[server] + 1;
    int lastLogIndex = getLastLogIndexAndTerm()[0];

    myAssert(nextIndex_[server] <= lastLogIndex + 1,
             format("error msg:rf.nextIndex[%d] > lastLogIndex+1, len(rf.logs) = %d   lastLogIndex{%d} = %d", server,
                    logs_.size(), server, lastLogIndex));
    if (appendNums >= 1 + m_peers.size() / 2) {
      if (args->entries(args->entries_size() - 1).logterm() == currentTerm_) { // 防御性检查
        commitedMaxIndex_ = std::max(commitedMaxIndex_, args->prevlogindex() + args->entries_size());
      }
      break;                
    }
  }
  }
}

Raft::Raft():
currentTerm_(0),
votedFor_(-1),
status_(Follower),
commitedMaxIndex_(0),
AppliedMaxIndex_(0) 
{
}

void Raft::init(std::vector<std::shared_ptr<RaftRpcUtil>> peers, int id, std::shared_ptr<Persister> persister,
                std::shared_ptr<LockQueue<ApplyMsg>> applyCh) {
  m_peers = peers;
  m_persister = persister;
  id_ = id;
  // Your initialization code here (2A, 2B, 2C).
  m_mtx.lock();
  applyChan = applyCh;
  logs_.clear();
  for (int i = 0; i < m_peers.size(); i++) {
    matchIndex_.push_back(0);
    nextIndex_.push_back(0);
  }

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
  boostPersistRaftNode.currentTerm_ = currentTerm_;
  boostPersistRaftNode.votedFor_ = votedFor_;
  for (auto& item : logs_) {
    boostPersistRaftNode.logs_.push_back(item.SerializeAsString());
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

  currentTerm_ = boostPersistRaftNode.currentTerm_;
  votedFor_ = boostPersistRaftNode.votedFor_;
  logs_.clear();
  for (auto& item : boostPersistRaftNode.logs_) {
    raftRpcProctoc::LogEntry logEntry;
    logEntry.ParseFromString(item);
    logs_.emplace_back(logEntry);
  }
}


Raft::Status Raft::checkStatus()
{
  return status_;
}

// Leader 
void Raft::doHeartBeat() {
  std::lock_guard<std::mutex> g(m_mtx);
  myAssert(status_ == Leader, 
    format("[func-Raft::doHeartBeat()-raft{%d}] 只有leader的心跳定时器才会触发发送心跳，当前状态{%d}异常了", id_, status_));
  std::vector<std::shared_ptr<raftRpcProctoc::AppendEntriesReply>> appendEntriesReplies(m_peers.size(),
    std::make_shared<raftRpcProctoc::AppendEntriesReply>()
  );
  for (int i = 0; i < m_peers.size(); i++) {
    if (i == id_) {continue;}
    DPrintf("[func-Raft::doHeartBeat()-Leader: {%d}] Leader的心跳定时器触发了 index:{%d}\n", id_, i);
    myAssert(nextIndex_[i] >= 1, format("rf.nextIndex[%d] = {%d}", i, nextIndex_[i]));
      int preLogIndex = -1;
      int PrevLogTerm = -1;
      getPrevLogInfo(i, &preLogIndex, &PrevLogTerm);
      std::shared_ptr<raftRpcProctoc::AppendEntriesArgs> appendEntriesArgs =
          std::make_shared<raftRpcProctoc::AppendEntriesArgs>();
      appendEntriesArgs->set_term(currentTerm_);
      appendEntriesArgs->set_leaderid(id_);
      // appendEntriesArgs->set_prevlogindex(preLogIndex);
      // appendEntriesArgs->set_prevlogterm(PrevLogTerm);
      appendEntriesArgs->clear_entries();
      // appendEntriesArgs->set_leadercommit(commitedMaxIndex_);

      // for (const auto& item : logs_) {
      //   *appendEntriesArgs->add_entries()=item;
      // }
      
      //int lastLogIndex = getLastLogIndex();
      auto appendEntriesReply = appendEntriesReplies[i];
      appendEntriesReply->set_appstate(Disconnected);

      SendAppendEntries(i, appendEntriesArgs, appendEntriesReply);
    }
}

// Candidate
void Raft::doElection() {
  std::lock_guard<std::mutex> g(m_mtx);
    currentTerm_ += 1;
    votedFor_ = id_; 
    persist();
    std::vector<std::shared_ptr<raftRpcProctoc::RequestVoteReply>> requestVoteReplies(m_peers.size(),
      std::make_shared<raftRpcProctoc::RequestVoteReply>()
    );
    int lastLogIndex = -1, lastLogTerm = -1;
    auto lastLogIndexAndTerm = getLastLogIndexAndTerm();
    lastLogIndex = lastLogIndexAndTerm[0];
    lastLogTerm = lastLogIndexAndTerm[1];
    for (int i = 0; i < m_peers.size(); i++) {
      if (i == id_) {continue;}
      std::shared_ptr<raftRpcProctoc::RequestVoteArgs> requestVoteArgs =
          std::make_shared<raftRpcProctoc::RequestVoteArgs>();
      requestVoteArgs->set_term(currentTerm_);
      requestVoteArgs->set_candidateid(id_);
      requestVoteArgs->set_lastlogindex(lastLogIndex);
      requestVoteArgs->set_lastlogterm(lastLogTerm);
      auto requestVoteReply = requestVoteReplies[i]; 
      AsyncSendRequestVote(i, requestVoteArgs, requestVoteReply);
  }
  // 挂起阻塞

  int voteCount = 0;
  for(int i = 0; i < m_peers.size(); i++) {
    if (i == id_) {continue;}
    auto requestVoteReply = requestVoteReplies[i];
    if (requestVoteReply->votegranted()) {
      voteCount += 1;
      if(voteCount >= 1 + m_peers.size() / 2) {
        status_ = Leader;
        DPrintf("[func-Raft::doElection()-Candidate: {%d}] Candidate赢得了选举，变成了Leader\n", id_);
        return;
      }
    }

  }
  status_ = Follower;
}

// Follower
void Raft::checkAndVerbCandidate() {
  // 阻塞通知
  status_ = Candidate;
}

// All 
void Raft::applierTicker() {
    m_mtx.lock();
    auto applyMsgs = getApplyLogs();
    m_mtx.unlock();
    for (auto& message : applyMsgs) {
      applyChan->Push(message);
    }
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
  applierTicker();
}
void Raft::doCandidateWork()
{
  doElection();
  applierTicker();
}


}
}