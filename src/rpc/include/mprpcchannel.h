#ifndef MPRPCCHANNEL_H
#define MPRPCCHANNEL_H

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <string>
#include <algorithm>

class MpRpcChannel : public google::protobuf::RpcChannel {
public:
    // rpc所有的方法都在这里被调用
    void CallMethod(const google::protobuf::MethodDescriptor *method, google::protobuf::RpcController *controller,
                    const google::protobuf::Message *request, google::protobuf::Message *response,
                    google::protobuf::Closure *done) override;
    MpRpcChannel(std::string ip, short port, bool connectNow);

private:
    int m_clientFd;
    const std::string m_ip; // 保存ip和端口，如果断了可以尝试重连
    const uint16_t m_port;

    /// @brief 连接ip和端口,并设置m_clientFd
    /// @param ip ip地址，本机字节序
    /// @param port 端口，本机字节序
    /// @return 成功返回空字符串，否则返回失败信息
    bool newConnect(const char *ip, uint16_t port, std::string *errMsg);
};

#endif