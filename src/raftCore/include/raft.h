#ifndef RAFT_H
#define RAFT_H

#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "ApplyMsg.h"
#include "Persister.h"
#include "boost/any.hpp"
#include "boost/serialization/serialization.hpp"
#include "../../common/include/config.h"
#include "../../fiber/include/monsoon.h"
#include "../../common/include/util.h"
#include "raftRpcUtil.h"

/// 网络状态
// 网络异常
constexpr int Disconnected  = 0;
constexpr int AppNormal     = 1;

/// 投票状态
// 异常
constexpr int Killed    = 0;
// 已投
constexpr int Voted     = 1;
// 过期
constexpr int Expire    = 2;
// 正常
constexpr int Normal    = 3;

class Raft : public raftRpcProto::raftRpc {
private:
    std::mutex m_mtx;
    std::vector<std::shared_ptr<RaftRpcUtil>> m_peers;
    std::shared_ptr<Persister> m_persister;
    // 自己
    int m_me;
    int m_currentTerm;
    int m_votedFor;
    // 日志条目，包含状态机要执行的指令集，以及收到领导时的任期号
    std::vector<raftRpcProto::LogEntry> m_logs;

    int m_commitIndex;
    int m_lastApplied;

    std::vector<int> m_nextIndex;
    std::vector<int> m_matchIndex;
    enum Status
    {
        Follower,
        Candidate,
        Leader
    };
    // 身份
    Status m_status;

    // client与raft的通信接口，client从channel读取日志
    std::shared_ptr<LockQueue<ApplyMsg>> applyChan;

    // 选举超时
    std::chrono::_V2::system_clock::time_point m_lastResetElectionTime;
    // 心跳超时，用于leader
    std::chrono::_V2::system_clock::time_point m_lastResetHearBeatTime;

    // 储存快照中的最后一个日志的Index和Term
    int m_lastSnapshotIncludeIndex;
    int m_lastSnapshotIncludeTerm;

    // 协程
    std::unique_ptr<monsoon::IOManager> m_ioManager = nullptr;

public:
    // 日志复制、心跳发送
    void AppendEntriesAnotherVersion(const raftRpcProto::AppendEntriesArgs *args, raftRpcProto::AppendEntriesReply *reply);
    void applierTicker();
    bool CondInstallSnapshot(int lastIncludedTerm, int lastIncludedIndex, std::string snapshot);
    void doElection();
    void doHeartBeat();

    void electionTimeOutTicker();
    std::vector<ApplyMsg> getApplyLogs();
    int getNewCommandIndex();
    void getPrevLogInfo(int server, int *preIndex, int *preTerm);
    void GetState(int *term, bool *isLeader);
    void InstallSnapshot(const raftRpcProto::InstallSnapshotRequest *args,
                         raftRpcProto::InstallSnapshotResponse *reply);
    void leaderHearBeatTicker();
    void leaderSendSnapShot(int server);
    void leaderUpdateCommitIndex();
    bool matchLog(int logIndex, int logTerm);
    void persist();
    void RequestVote(const raftRpcProto::RequestVoteArgs *args, raftRpcProto::RequestVoteReply *reply);
    bool UpToDate(int index, int term);
    int getLastLogIndex();
    int getLastLogTerm();
    void getLastLogIndexAndTerm(int *lastLogIndex, int *lastLogTerm);
    int getLogTermFromLogIndex(int logIndex);
    int GetRaftStateSize();
    int getSlicesIndexFromLogIndex(int logIndex);

    bool sendRequestVote(int server, std::shared_ptr<raftRpcProto::RequestVoteArgs> args,
                         std::shared_ptr<raftRpcProto::RequestVoteReply> reply, std::shared_ptr<int> votedNum);
    bool sendAppendEntries(int server, std::shared_ptr<raftRpcProto::AppendEntriesArgs> args,
                           std::shared_ptr<raftRpcProto::AppendEntriesReply> reply, std::shared_ptr<int> appendNums);

    void pushMsgToKvServer(ApplyMsg msg);
    void readPersist(std::string data);
    std::string persistData();

    void Start(OperaionFromRaft command, int *newLogIndex, int *newLogTerm, bool *isLeader);
    void Snapshot(int index, std::string snapshot);

    // 重写基类方法
    void AppendEntries(google::protobuf::RpcController *controller,
                       const ::raftRpcProto::AppendEntriesArgs *request,
                       ::raftRpcProto::AppendEntriesReply *response,
                       ::google::protobuf::Closure *done);
    void InstallSnapshot(google::protobuf::RpcController *controller,
                         const ::raftRpcProto::InstallSnapshotRequest *request,
                         ::raftRpcProto::InstallSnapshotResponse *response,
                         ::google::protobuf::Closure *done);
    void RequestVote(google::protobuf::RpcController *controller,
                     const ::raftRpcProto::RequestVoteArgs *request,
                     ::raftRpcProto::RequestVoteReply *response,
                     ::google::protobuf::Closure *done);

    void init(std::vector<std::shared_ptr<RaftRpcUtil>> peers, int me, std::shared_ptr<Persister> persister,
              std::shared_ptr<LockQueue<ApplyMsg>> applyCh);

private:
    class BoostPersistRaftNode
    {
    public:
        friend class boost::serialization::access;
        // When the class Archive corresponds to an output archive, the
        // & operator is defined similar to <<.  Likewise, when the class Archive
        // is a type of input archive the & operator is defined similar to >>.
        template <class Archive>
        void serialize(Archive &ar, const unsigned int version)
        {
            ar &m_currentTerm;
            ar &m_votedFor;
            ar &m_lastSnapshotIncludeIndex;
            ar &m_lastSnapshotIncludeTerm;
            ar &m_logs;
        }
        int m_currentTerm;
        int m_votedFor;
        int m_lastSnapshotIncludeIndex;
        int m_lastSnapshotIncludeTerm;
        std::vector<std::string> m_logs;
        std::unordered_map<std::string, int> umap;
    };
};

#endif