#ifndef RAFTRPC_H
#define RAFTRPC_H

#include "../../raftRpcProto/include/raftRPC.pb.h"

class RaftRpcUtil {
private:
    raftRpcProto::raftRpc_Stub *stub_;

public:
    // 主动调用其他节点的方法

    // 日志一致性检查
    bool AppendEntries(raftRpcProto::AppendEntriesArgs *args, raftRpcProto::AppendEntriesReply *response);

    // 快照机制
    bool InstallSnapshot(raftRpcProto::InstallSnapshotRequest *args, raftRpcProto::InstallSnapshotResponse *response);

    // 拉票请求
    bool RequestVote(raftRpcProto::RequestVoteArgs *args, raftRpcProto::RequestVoteReply *response);

    RaftRpcUtil(std::string ip, short port);
    ~RaftRpcUtil();
};

#endif