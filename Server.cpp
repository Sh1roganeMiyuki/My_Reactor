#include "Server.h"
#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>

Server::Server(InetAddress& addr, EventLoop* loop) : addr_(addr), loop_(loop) 
{
    // threadPool_ = std::make_unique<EventLoopThreadPool>(loop_, 0);
    // acceptor_.newConnectionCallback(
    //     [this](int sockfd) { this->newConnection(sockfd); }
    // );
    
}
void Server::newConnection(int sockfd, EventLoop* ioLoop) {
    // 1. 【物理防线】拦截 Accept 错误返回的 -1
    // 如果不拦截，就会生成 "Conn--1"，导致 map 节点覆盖和 unlink_chunk 崩溃
    if (sockfd < 0) return;

    std::string connName = "Conn-" + std::to_string(sockfd);
    
    // ... 这里写你原本的获取 Local/Peer Addr 代码 ...
    InetAddress localAddr("127.0.0.1", 8080); // 示例
    InetAddress peerAddr("127.0.0.1", 0);     // 示例

    // 2. 创建连接对象，归属于当前的 ioLoop
    auto conn = std::make_shared<TcpConnection>(
        ioLoop, connName, sockfd, localAddr, peerAddr
    );

    // 3. 【核心加锁】存入全局 Map
    {
        std::lock_guard<std::mutex> lock(connMutex_);
        connections_[connName] = conn;
    }

    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setCloseCallBack(
        std::bind(&Server::removeConnection, this, std::placeholders::_1)
    );

    // 4. 直接执行，因为当前已经在 ioLoop 线程里了
    conn->connectEstablished();
}

void Server::removeConnection(const std::shared_ptr<TcpConnection>& conn) {
    // 1. 【核心加锁】从全局 Map 移除
    {
        std::lock_guard<std::mutex> lock(connMutex_);
        connections_.erase(conn->name());
    }
    
    // 2. 依然通过 runInLoop 回到它自己的线程去销毁，保证线程安全
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn)
    );
}
void Server::start() {
    threadPool_->start();
    auto subLoops = threadPool_->getAllLoops();

    // 封装一个启动 Acceptor 的逻辑
    auto startAcceptor = [this](EventLoop* ioLoop) {
        // 在当前 ioLoop 线程创建一个新的 Acceptor
        auto acceptor = std::make_unique<Acceptor>(ioLoop, addr_);
        
        // 设置回调：直接把当前的 ioLoop 传进去
        acceptor->newConnectionCallback(
            [this, ioLoop](int sockfd) { 
                this->newConnection(sockfd, ioLoop); 
            }
        );
        acceptor->listen(); // 开始监听

        // 保存起来
        {
            std::lock_guard<std::mutex> lock(acceptorMutex_);
            subAcceptors_.push_back(std::move(acceptor));
        }
    };
    if (subLoops.empty()) {
        // 单线程模式：直接在主 loop 启动
        startAcceptor(loop_);
    } else {
        // 多线程模式：利用 runInLoop 把创建任务分发到每个子线程
        for (auto* subLoop : subLoops) {
            subLoop->runInLoop([subLoop, startAcceptor]() {
                startAcceptor(subLoop);
            });
        }
        
        // 【可选】如果你想让主线程也参与监听（变成 N+1 个），把下面这行解开
         startAcceptor(loop_); 
    }
}
Server::~Server() {
}
