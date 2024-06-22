#include "include/mprpcchannel.h"
#include "../common/include/util.h"
#include "include/rpcheader.pb.h"
#include <string>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>

// 通过channel的callmethod来调用rpc的方法，数据序列化、网络发送
void MpRpcChannel::CallMethod(const google::protobuf::MethodDescriptor *method,
                              google::protobuf::RpcController *controller, const google::protobuf::Message *request,
                              google::protobuf::Message *response, google::protobuf::Closure *done) {
    if (m_clientFd == -1) {
        std::string errMsg;
        bool rt = newConnect(m_ip.c_str(), m_port, &errMsg);
        if (!rt)
        {
            DPrintf("[func-MprpcChannel::CallMethod]重连接ip：{%s} port{%d}失败", m_ip.c_str(), m_port);
            controller->SetFailed(errMsg);
            return;
        }
        else
        {
            DPrintf("[func-MprpcChannel::CallMethod]连接ip：{%s} port{%d}成功", m_ip.c_str(), m_port);
        }
    }

    const google::protobuf::ServiceDescriptor *sd = method->service();
    std::string service_name = sd->name();
    std::string method_name = method->name();

    // 获取参数序列化字符串长度 初始化为0
    uint32_t args_size{};
    std::string args_str;
    if (request->SerializeToString(&args_str)) {
        args_size = args_str.size();
    } else {
        controller->SetFailed("serialize request error!");
        return;
    }

    RPC::RpcHeader rpcHeader;
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_args_size(args_size);

    std::string rpc_header_str;
    if (!rpcHeader.SerializeToString(&rpc_header_str)) {
        controller->SetFailed("serialize rpc header error!");
        return;
    }

    // 存储最终发送的数据
    std::string send_rpc_str;
    {
        // 创建一个StringOutputStream用于写入send_rpc_str
        google::protobuf::io::StringOutputStream string_output(&send_rpc_str);
        google::protobuf::io::CodedOutputStream coded_output(&string_output);

        // 先写入header的长度（变长编码）
        coded_output.WriteVarint32(static_cast<uint32_t>(rpc_header_str.size()));

        // 不需要手动写入header_size，因为上面的WriteVarint32已经包含了header的长度信息
        // 然后写入rpc_header本身
        coded_output.WriteString(rpc_header_str);
    }

    // 最后，将请求参数附加到send_rpc_str后面
    send_rpc_str += args_str;

    // 发送rpc请求
    // 失败会重试连接再发送，重试连接失败会直接return
    while (-1 == send(m_clientFd, send_rpc_str.c_str(), send_rpc_str.size(), 0))
    {
        char errtxt[512] = {0};
        sprintf(errtxt, "send error! errno:%d", errno);
        std::cout << "尝试重新连接，对方ip：" << m_ip << " 对方端口" << m_port << std::endl;
        close(m_clientFd);
        m_clientFd = -1;
        std::string errMsg;
        bool rt = newConnect(m_ip.c_str(), m_port, &errMsg);
        if (!rt)
        {
            controller->SetFailed(errMsg);
            return;
        }
    }

    // 接收rpc请求的响应值
    char recv_buf[1024] = {0};
    int recv_size = 0;
    if (-1 == (recv_size = recv(m_clientFd, recv_buf, 1024, 0)))
    {
        close(m_clientFd);
        m_clientFd = -1;
        char errtxt[512] = {0};
        sprintf(errtxt, "recv error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    // 反序列化
    if (!response->ParseFromArray(recv_buf, recv_size)) {
        char errtxt[1050] = {0};
        sprintf(errtxt, "parse error! response_str:%s", recv_buf);
        controller->SetFailed(errtxt);
        return;
    }
}

bool MpRpcChannel::newConnect(const char *ip, uint16_t port, std::string *errMsg)
{
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd) {
        char errtxt[512] = {0};
        sprintf(errtxt, "create socket error! errno:%d", errno);
        m_clientFd = -1;
        *errMsg = errtxt;
        return false;
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    // 连接rpc服务节点
    if (-1 == connect(clientfd, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "connect fail! errno:%d", errno);
        m_clientFd = -1;
        *errMsg = errtxt;
        return false;
    }
    m_clientFd = clientfd;
    return true;
}

MpRpcChannel::MpRpcChannel(std::string ip, short port, bool connectNow) : m_ip(ip), m_port(port), m_clientFd(-1)
{
    if (!connectNow)
        return;
    
    std::string errMsg;
    auto rt = newConnect(ip.c_str(), port, &errMsg);
    int tryCount = 3;
    while (!rt && tryCount--)
    {
        std::cout << errMsg << std::endl;
        rt = newConnect(ip.c_str(), port, &errMsg);
    }
}