//
// Created by swx on 23-5-30.
//
#include "Persister.h"

// todo:会涉及反复打开文件的操作，没有考虑如果文件出现问题会怎么办？？
void Persister::Save(const std::string raftstate) {
  std::lock_guard<std::mutex> lg(m_mtx);
  clearRaftState();
  // 将raftstate和snapshot写入本地文件
  m_raftStateOutStream << raftstate;
}


void Persister::SaveRaftState(const std::string &data) {
  std::lock_guard<std::mutex> lg(m_mtx);
  // 将raftstate和snapshot写入本地文件
  clearRaftState();
  m_raftStateOutStream << data;
  m_raftStateSize = data.size();////////////////////////////////////////////////////
}

long long Persister::RaftStateSize() {
  std::lock_guard<std::mutex> lg(m_mtx);

  return m_raftStateSize;
}

std::string Persister::ReadRaftState() {
  std::lock_guard<std::mutex> lg(m_mtx);

  std::fstream ifs(m_raftStateFileName, std::ios_base::in);
  if (!ifs.good()) {
    return "";
  }
  std::string snapshot;
  ifs >> snapshot;
  ifs.close();
  return snapshot;
}

Persister::Persister(const int me)
    : m_raftStateFileName("raftstatePersist" + std::to_string(me) + ".txt"),
      m_raftStateSize(0) {

  m_raftStateOutStream.open(m_raftStateFileName);

}

Persister::~Persister() {
  if (m_raftStateOutStream.is_open()) {
    m_raftStateOutStream.close();
  }

}

void Persister::clearRaftState() {
  m_raftStateSize = 0;
  // 关闭文件流
  if (m_raftStateOutStream.is_open()) {
    m_raftStateOutStream.close();
  }
  // 重新打开文件流并清空文件内容
  m_raftStateOutStream.open(m_raftStateFileName, std::ios::out | std::ios::trunc);
}


