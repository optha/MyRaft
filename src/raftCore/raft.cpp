#include "include/raft.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <memory>
#include "../common/include/config.h"
#include "../common/include/util.h"

void Raft::AppendEntriesAnotherVersion(const raftRpcProto::AppendEntriesArgs *args, raftRpcProto::AppendEntriesReply *reply)
{
    std::lock_guard<std::mutex> locker(m_mtx);
    // 到这里代表网络正常
    reply->set_appstate(AppNormal);

    // 当收到的请求中term值比自己小，表示该Leader过期，拒绝该请求
    if (args->term() < m_currentTerm)
    {
        reply->set_success(false);
        reply->set_term(m_currentTerm);
        reply->set_updatenextindex(-100);
        DPrintf("Leader{%d}的term{%d} < rf{%d}.term{%d}\n", args->leaderid(), args->term(), m_me, m_currentTerm);
        return;
    }

    DEFER { persist(); };

    if (args->term() > m_currentTerm)
    {
        m_status = Follower;
        m_currentTerm = args->term();
        m_votedFor = -1;
    }
    myAssert(args->term() == m_currentTerm, format("assert {args.Term == rf.currentTerm} fail"));
    // 如果发生网络分区，那么candidate可能会收到同一个term的leader的消息，要转变为Follower
    m_status = Follower;
    m_lastResetElectionTime = now();

    //	比较日志，日志有3种情况
    if (args->prevlogindex() > getLastLogIndex())
    {
        reply->set_success(false);
        reply->set_term(m_currentTerm);
        reply->set_updatenextindex(getLastLogIndex() + 1);
        return;
    }
    else if (args->prevlogindex() < m_lastSnapshotIncludeIndex)
    {
        // 如果prevlogIndex还没有更上快照
        reply->set_success(false);
        reply->set_term(m_currentTerm);
        reply->set_updatenextindex(
            m_lastSnapshotIncludeIndex +
            1);
    }
    //	本机日志有那么长，冲突(same index,different term),截断日志
    // 注意：这里目前当args.PrevLogIndex == rf.lastSnapshotIncludeIndex与不等的时候要分开考虑，可以看看能不能优化这块
    if (matchLog(args->prevlogindex(), args->prevlogterm()))
    {
        //	todo：	整理logs
        // ，不能直接截断，必须一个一个检查，因为发送来的log可能是之前的，直接截断可能导致“取回”已经在follower日志中的条目
        // 那意思是不是可能会有一段发来的AE中的logs中前半是匹配的，后半是不匹配的，这种应该：1.follower如何处理？ 2.如何给leader回复
        // 3. leader如何处理

        for (int i = 0; i < args->entries_size(); i++)
        {
            auto log = args->entries(i);
            if (log.logindex() > getLastLogIndex())
            {
                // 超过就直接添加日志
                m_logs.push_back(log);
            }
            else
            {
                // 没超过就比较是否匹配，不匹配再更新，而不是直接截断
                //  todo ： 这里可以改进为比较对应logIndex位置的term是否相等，term相等就代表匹配
                //   todo：这个地方放出来会出问题,按理说index相同，term相同，log也应该相同才对
                //  rf.logs[entry.Index-firstIndex].Term ?= entry.Term

                if (m_logs[getSlicesIndexFromLogIndex(log.logindex())].logterm() == log.logterm() &&
                    m_logs[getSlicesIndexFromLogIndex(log.logindex())].command() != log.command())
                {
                    // 相同位置的log ，其logTerm相等，但是命令却不相同，不符合raft的前向匹配，异常了！
                    myAssert(false, format("[func-AppendEntries-rf{%d}] 两节点logIndex{%d}和term{%d}相同，但是其command{%d:%d}   "
                                           " {%d:%d}却不同！！\n",
                                           m_me, log.logindex(), log.logterm(), m_me,
                                           m_logs[getSlicesIndexFromLogIndex(log.logindex())].command(), args->leaderid(),
                                           log.command()));
                }
                if (m_logs[getSlicesIndexFromLogIndex(log.logindex())].logterm() != log.logterm())
                {
                    // 不匹配就更新
                    m_logs[getSlicesIndexFromLogIndex(log.logindex())] = log;
                }
            }
        }

        // 错误写法like：  rf.shrinkLogsToIndex(args.PrevLogIndex)
        // rf.logs = append(rf.logs, args.Entries...)
        // 因为可能会收到过期的log！！！ 因此这里是大于等于
        myAssert(
            getLastLogIndex() >= args->prevlogindex() + args->entries_size(),
            format("[func-AppendEntries1-rf{%d}]rf.getLastLogIndex(){%d} != args.PrevLogIndex{%d}+len(args.Entries){%d}",
                   m_me, getLastLogIndex(), args->prevlogindex(), args->entries_size()));
        // if len(args.Entries) > 0 {
        //	fmt.Printf("[func-AppendEntries  rf:{%v}] ] : args.term:%v, rf.term:%v  ,rf.logs的长度：%v\n", rf.me, args.Term,
        // rf.currentTerm, len(rf.logs))
        // }
        if (args->leadercommit() > m_commitIndex)
        {
            m_commitIndex = std::min(args->leadercommit(), getLastLogIndex());
            // 这个地方不能无脑跟上getLastLogIndex()，因为可能存在args->leadercommit()落后于 getLastLogIndex()的情况
        }

        // 领导会一次发送完所有的日志
        myAssert(getLastLogIndex() >= m_commitIndex,
                 format("[func-AppendEntries1-rf{%d}]  rf.getLastLogIndex{%d} < rf.commitIndex{%d}", m_me,
                        getLastLogIndex(), m_commitIndex));
        reply->set_success(true);
        reply->set_term(m_currentTerm);

        //        DPrintf("[func-AppendEntries-rf{%v}] 接收了来自节点{%v}的log，当前lastLogIndex{%v}，返回值：{%v}\n",
        //        rf.me,
        //                args.LeaderId, rf.getLastLogIndex(), reply)

        return;
    }
    else
    {
        // 优化
        // PrevLogIndex 长度合适，但是不匹配，因此往前寻找 矛盾的term的第一个元素
        // 为什么该term的日志都是矛盾的呢？也不一定都是矛盾的，只是这么优化减少rpc而已
        // ？什么时候term会矛盾呢？很多情况，比如leader接收了日志之后马上就崩溃等等
        reply->set_updatenextindex(args->prevlogindex());

        for (int index = args->prevlogindex(); index >= m_lastSnapshotIncludeIndex; --index)
        {
            if (getLogTermFromLogIndex(index) != getLogTermFromLogIndex(args->prevlogindex()))
            {
                reply->set_updatenextindex(index + 1);
                break;
            }
        }
        reply->set_success(false);
        reply->set_term(m_currentTerm);
        // 对UpdateNextIndex待优化  todo  找到符合的term的最后一个
        //        DPrintf("[func-AppendEntries-rf{%v}]
        //        拒绝了节点{%v}，因为prevLodIndex{%v}的args.term{%v}不匹配当前节点的logterm{%v}，返回值：{%v}\n",
        //                rf.me, args.LeaderId, args.PrevLogIndex, args.PrevLogTerm,
        //                rf.logs[rf.getSlicesIndexFromLogIndex(args.PrevLogIndex)].LogTerm, reply)
        //        DPrintf("[func-AppendEntries-rf{%v}] 返回值: reply.UpdateNextIndex从{%v}优化到{%v}，优化了{%v}\n", rf.me,
        //                args.PrevLogIndex, reply.UpdateNextIndex, args.PrevLogIndex - reply.UpdateNextIndex) //
        //                很多都是优化了0
        return;
    }

    // fmt.Printf("[func-AppendEntries,rf{%v}]:len(rf.logs):%v, rf.commitIndex:%v\n", rf.me, len(rf.logs), rf.commitIndex)
}

    std::lock_guard<std::mutex> locker(m_mtx);

    // 能到这一步就表示网络是正常的
    reply->set_appstate(AppNormal);

    // 收到Leader回复中的term值比自己小，此时Leader过期，拒绝该请求
    if (args->term() < m_currentTerm) {
        reply->set_success(false);
        reply->set_term(m_currentTerm);
        reply->set_updatenextindex(-100);
        DPrintf("Leader{%d}的term = %d < 当前节点{%d}的term = %d， 拒绝该请求", args->leaderid(), args->term(), m_me, m_currentTerm);
        return;
    }

    DEFER { persist(); };

    // 收到Leader回复中的term值比自己大
    if (args->term() > m_currentTerm) {
        m_status = Follower;
        m_currentTerm = args->term();
        m_votedFor = -1;
    }
    myAssert(args->term() == m_currentTerm, format("args.term != rf.currentTerm"));
    m_status = Follower;
    m_lastResetElectionTime = now();

    if (args->prevlogindex() > getLastLogIndex()) {
        reply->set_success(false);
        reply->set_term(m_currentTerm);
        reply->set_updatenextindex(m_lastSnapshotIncludeIndex + 1);
    }

    if (matchLog(args->prevlogindex(), args->prevlogterm())) {
        for (int i = 0; i < args->entries_size(); i++)
        {
            auto log = args->entries(i);
            if (log.logindex() > getLastLogIndex())
            {
                // 超过就直接添加日志
                m_logs.push_back(log);
            } else {
                if (m_logs[getSlicesIndexFromLogIndex(log.logindex())].logterm() == log.logterm() &&
                    m_logs[getSlicesIndexFromLogIndex(log.logindex())].command() != log.command())
                {
                    // 相同位置的log ，其logTerm相等，但是命令却不相同，不符合raft的前向匹配，异常了！
                    myAssert(false, format("[func-AppendEntries-rf{%d}] 两节点logIndex{%d}和term{%d}相同，但是其command{%d:%d}   "
                                           " {%d:%d}却不同！！\n",
                                           m_me, log.logindex(), log.logterm(), m_me,
                                           m_logs[getSlicesIndexFromLogIndex(log.logindex())].command(), args->leaderid(),
                                           log.command()));
                }
                if (m_logs[getSlicesIndexFromLogIndex(log.logindex())].logterm() != log.logterm())
                {
                    // 不匹配就更新
                    m_logs[getSlicesIndexFromLogIndex(log.logindex())] = log;
                }
            }
        }
        myAssert(getLastLogIndex() >= args->prevlogindex() + args->entries_size(), format("[func-AppendEntries1-rf{%d}]rf.getLastLogIndex(){%d} != args.PrevLogIndex{%d}+len(args.Entries){%d}", m_me, getLastLogIndex(), args->prevlogindex(), args->entries_size()));

        if (args->leadercommit() > m_commitIndex)
        {
            m_commitIndex = std::min(args->leadercommit(), getLastLogIndex());
        }
    }
}