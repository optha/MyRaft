#pragma once
#include <google/protobuf/service.h>
#include <string>

class MpRpcController : public google::protobuf::RpcController
{
public:
    MpRpcController();
    void Reset();
    bool Failed() const;
    std::string ErrorText() const;
    void SetFailed(const std::string &reason);

    // google::protobuf::RpcController中的虚函数，目前未实现
    void StartCancel() {}
    bool IsCanceled() const { return false; }
    void NotifyOnCancel(google::protobuf::Closure *callback) {}

private:
    // RPC方法执行过程中的状态
    bool m_failed;
    // RPC方法执行过程中的错误信息
    std::string m_errText;
};