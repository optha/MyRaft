#pragma once
#include <google/protobuf/descriptor.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/TcpServer.h>
#include <functional>
#include <string>
#include <unordered_map>
#include "google/protobuf/service.h"

// 发布rpc服务的网络对象类
class MpRpcProvider {
public:
    ~MpRpcProvider();
    
    // 发布rpc方法的函数接口
    void NotifyService(google::protobuf::Service *service);

    // 启动rpc服务节点，开始提供rpc远程网络调用服务
    void Run(int nodeIndex, short port);

private:
    // 组合EventLoop
    muduo::net::EventLoop m_eventLoop;
    std::shared_ptr<muduo::net::TcpServer> m_muduo_server;

    // service服务类型信息
    struct ServiceInfo
    {
        // 服务对象
        google::protobuf::Service *m_service;
        // 保存服务方法
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor *> m_methodMap;
    };

    // 存储注册成功的服务对象和其服务方法的所有信息
    std::unordered_map<std::string, ServiceInfo> m_serviceMap;

    // 新的socket连接回调
    void OnConnection(const muduo::net::TcpConnectionPtr &);
    // 已建立连接用户的读写事件回调
    void OnMessage(const muduo::net::TcpConnectionPtr &, muduo::net::Buffer *, muduo::Timestamp);
    // Closure的回调操作，用于序列化rpc的响应和网络发送
    void SendRpcResponse(const muduo::net::TcpConnectionPtr &, google::protobuf::Message *);
};