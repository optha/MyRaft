#include "include/mprpccontroller.h"

MpRpcController::MpRpcController()
{
    m_failed = false;
    m_errText = "";
}

void MpRpcController::Reset()
{
    m_failed = false;
    m_errText = "";
}

bool MpRpcController::Failed() const { return m_failed; }

std::string MpRpcController::ErrorText() const { return m_errText; }

void MpRpcController::SetFailed(const std::string &reason)
{
    m_failed = true;
    m_errText = reason;
}