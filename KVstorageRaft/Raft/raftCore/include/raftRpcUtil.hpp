
#ifndef RAFTRPC_H
#define RAFTRPC_H

#include "raftRPC.pb.h"


class RaftRpcUtil {
 private:

  raftRpcProctoc::raftRpc_Stub *stub_;

 public:
  bool AppendEntries(raftRpcProctoc::AppendEntriesArgs *args, raftRpcProctoc::AppendEntriesReply *response);

  bool InstallSnapshot(raftRpcProctoc::InstallSnapshotRequest *args, raftRpcProctoc::InstallSnapshotResponse *response);

  bool RequestVote(raftRpcProctoc::RequestVoteArgs *args, raftRpcProctoc::RequestVoteReply *response);
  RaftRpcUtil(std::string ip, short port);

  ~RaftRpcUtil();
};

#endif  // RAFTRPC_H
