#pragma once
#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "sys/types.h"
#include "InetAddress.h"
#include "Acceptor.h"
#include "TcpConnection.h"
#include "EventLoopThreadPool.h"

class Server {
public:
    using ConnectionCallback = std::function<void(const std::shared_ptr<TcpConnection>&)>;
    using MessageCallback = std::function<void(const std::shared_ptr<TcpConnection>&, Buffer*)>;
    
    void setThreadNum(int numThreads) { threadPool_ = std::make_unique<EventLoopThreadPool>(loop_, numThreads); }

    // 暴露设置接口
    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
    Server(InetAddress& addr, EventLoop* loop);
    ~Server();
    //int getListenFd() const { return listen_fd_; }
    void start();
    void newConnection(int sockfd);
private:

    void removeConnection(const std::shared_ptr<TcpConnection>& conn);

    std::unique_ptr<EventLoopThreadPool> threadPool_; 

    ///int listen_fd_;
    InetAddress addr_;
    Acceptor acceptor_;
    EventLoop* loop_;
    using ConnectionMap = std::map<std::string, std::shared_ptr<TcpConnection>>; 
    ConnectionMap connections_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
};