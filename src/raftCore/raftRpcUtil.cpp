#include "include/raftRpcUtil.h"
#include "../rpc/include/mprpcchannel.h"
#include "../rpc/include/mprpccontroller.h"

bool RaftRpcUtil::AppendEntries(raftRpcProto::AppendEntriesArgs *args, raftRpcProto::AppendEntriesReply *response)
{
    MpRpcController controller;
    stub_->AppendEntries(&controller, args, response, nullptr);
    return !controller.Failed();
}

bool RaftRpcUtil::InstallSnapshot(raftRpcProto::InstallSnapshotRequest *args, raftRpcProto::InstallSnapshotResponse *response)
{
    MpRpcController controller;
    stub_->InstallSnapshot(&controller, args, response, nullptr);
}

bool RaftRpcUtil::RequestVote(raftRpcProto::RequestVoteArgs *args, raftRpcProto::RequestVoteReply *response)
{
    MpRpcController controller;
    stub_->RequestVote(&controller, args, response, nullptr);
    return !controller.Failed();
}

RaftRpcUtil::RaftRpcUtil(std::string ip, short port)
{
    stub_ = new raftRpcProto::raftRpc_Stub(new MpRpcChannel(ip, port, true));
}

RaftRpcUtil::~RaftRpcUtil() { delete stub_; }