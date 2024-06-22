#include "include/Persister.h"
#include "../common/include/util.h"

void Persister::Save(const std::string raftstate, const std::string snapshot)
{
    std::lock_guard<std::mutex> lg(m_mtx);
    clearRaftStateAndSnapshot();
    // 将raftstate和snapshot写入本地文件
    m_raftStateOutStream << raftstate;
    m_snapshotOutStream << snapshot;
}

std::string Persister::ReadSnapshot()
{
    std::lock_guard<std::mutex> lg(m_mtx);
    if (m_snapshotOutStream.is_open())
        m_snapshotOutStream.close();

    DEFER
    {
        // 默认是追加
        m_snapshotOutStream.open(m_snapshotFileName);
    };
    std::fstream ifs(m_snapshotFileName, std::ios_base::in);
    if (!ifs.good())
        return "";
    
    std::string snapshot;
    ifs >> snapshot;
    ifs.close();
    return snapshot;
}

void Persister::SaveRaftState(const std::string &data)
{
    std::lock_guard<std::mutex> lg(m_mtx);
    // 将raftstate和snapshot写入本地文件
    clearRaftState();
    m_raftStateOutStream << data;
    m_raftStateSize += data.size();
}

long long Persister::RaftStateSize()
{
    std::lock_guard<std::mutex> lg(m_mtx);
    return m_raftStateSize;
}

std::string Persister::ReadRaftState()
{
    std::lock_guard<std::mutex> lg(m_mtx);

    std::fstream ifs(m_raftStateFileName, std::ios_base::in);
    if (!ifs.good())
        return "";
    
    std::string snapshot;
    ifs >> snapshot;
    ifs.close();
    return snapshot;
}

Persister::Persister(const int me)
    : m_raftStateFileName("raftstatePersist" + std::to_string(me) + ".txt"),
      m_snapshotFileName("snapshotPersist" + std::to_string(me) + ".txt"),
      m_raftStateSize(0)
{
    // 检查文件状态并清空
    bool fileOpenFlag = true;
    std::fstream file(m_raftStateFileName, std::ios::out | std::ios::trunc);
    if (file.is_open())
        file.close();
    
    else
        fileOpenFlag = false;
    
    file = std::fstream(m_snapshotFileName, std::ios::out | std::ios::trunc);
    if (file.is_open())
        file.close();
    
    else
        fileOpenFlag = false;
    
    if (!fileOpenFlag)
        DPrintf("[func-Persister::Persister] file open error");
    
    // 绑定流
    m_raftStateOutStream.open(m_raftStateFileName);
    m_snapshotOutStream.open(m_snapshotFileName);
}
