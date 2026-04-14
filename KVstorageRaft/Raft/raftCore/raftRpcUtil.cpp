#include "raftRpcUtil.h"

#include <mprpcchannel.h>
#include <mprpccontroller.h>
namespace yy {
namespace net {
namespace raft {
bool RaftRpcUtil::AppendEntries(raftRpcProctoc::AppendEntriesArgs *args, raftRpcProctoc::AppendEntriesReply *response) {
  MprpcController controller;
  stub_->AppendEntries(&controller, args, response, nullptr);
  return !controller.Failed();
}

bool RaftRpcUtil::InstallSnapshot(raftRpcProctoc::InstallSnapshotRequest *args,
                                  raftRpcProctoc::InstallSnapshotResponse *response) {
  MprpcController controller;
  stub_->InstallSnapshot(&controller, args, response, nullptr);
  return !controller.Failed();
}

bool RaftRpcUtil::RequestVote(raftRpcProctoc::RequestVoteArgs *args, raftRpcProctoc::RequestVoteReply *response) {
  MprpcController controller;
  stub_->RequestVote(&controller, args, response, nullptr);
  return !controller.Failed();
}


RaftRpcUtil::RaftRpcUtil(const std::string& ip, short port, yy::net::EventLoop* loop) {
  stub_ = new raftRpcProctoc::raftRpc_Stub(new yy::net::rpc::MprpcChannel(ip, port, loop));
}

RaftRpcUtil::~RaftRpcUtil() { delete stub_; }


}
}
}
