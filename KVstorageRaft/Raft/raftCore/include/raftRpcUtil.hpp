
#ifndef RAFTRPC_H
#define RAFTRPC_H

#include "raftRPC.pb.h"

#include "Net/include/EventLoopThread.h"

namespace yy {
namespace net {
namespace raft {

class RaftRpcUtil {
 private:

  raftRpcProctoc::raftRpc_Stub *stub_;

 public:
  bool AppendEntries(raftRpcProctoc::AppendEntriesArgs *args, raftRpcProctoc::AppendEntriesReply *response);

  bool InstallSnapshot(raftRpcProctoc::InstallSnapshotRequest *args, raftRpcProctoc::InstallSnapshotResponse *response);

  bool RequestVote(raftRpcProctoc::RequestVoteArgs *args, raftRpcProctoc::RequestVoteReply *response);
  RaftRpcUtil(const std::string& ip, short port, yy::net::EventLoop* loop);

  ~RaftRpcUtil();
};


class RaftRpcUtils
{
private:
  std::vector<std::shared_ptr<RaftRpcUtil>> utils;
  EventLoopThread loopThread;
public:
  RaftRpcUtils(const std::vector<std::pair<std::string, short>>& IpAddrs)
  {
    EventLoop* loop = loopThread.run();
    for(const auto& ip_port : IpAddrs)
    {
      utils.emplace_back(std::make_shared<RaftRpcUtil>(ip_port.first, ip_port.second, loop));
    }
  }
  std::shared_ptr<RaftRpcUtil> operator [](size_t index)
  {
    assert(index < utils.size());
    return utils[index];
  }
};


}
}
}



#endif  // RAFTRPC_H
