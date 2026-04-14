#include "Server.h"
#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>
#include "ObjectPool.h"
Server::Server(InetAddress& addr, EventLoop* loop) : addr_(addr), loop_(loop) 
{
    threadPool_ = std::make_unique<EventLoopThreadPool>(loop_, 0);
    // 预分配 fd 索引数组，大小根据系统 ulimit -n 决定
    connections_.resize(65536); 
}
void Server::newConnection(int sockfd, EventLoop* ioLoop) {
    // 1. 边界检查，防止越界
    if (sockfd < 0 || (size_t)sockfd >= connections_.size()) {
        ::close(sockfd);
        return;
    }

    // 2. 🚀 从对象池获取 shared_ptr，控制块内存是复用的
    auto conn = ObjectPool<TcpConnection>::getInstance().get();
    
    // 3. 🚀 初始化状态，直接传 sockfd，彻底删掉 connName 字符串拼接
    conn->reset(ioLoop, sockfd, addr_, addr_); 

    {
        // 4. 🚀 直接根据 fd 索引赋值。vector 赋值不产生新内存分配
        std::lock_guard<std::mutex> lock(connMutex_);
        connections_[sockfd] = conn;
    }

    // 5. 设置回调
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    
    // 6. 🚀 使用 Lambda 替代 std::bind，减少 functor 对象的分配开销
    conn->setCloseCallBack([this](const std::shared_ptr<TcpConnection>& c) {
        this->removeConnection(c);
    });

    conn->connectEstablished();
    ioLoop->addTimer(conn); 
}

// 同时需要修改 removeConnection 以适配 vector 架构
void Server::removeConnection(const std::shared_ptr<TcpConnection>& conn) {
    int fd = conn->get_fd();

    {
        std::lock_guard<std::mutex> lock(connMutex_);
        if (fd >= 0 && (size_t)fd < connections_.size()) {
            // 直接重置数组位置，0 分配
            connections_[fd].reset(); 
        }
    }
    
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop([conn]() {
        conn->connectDestroyed();
    });
}
void Server::start() {
    if (!threadPool_) {
        // 手动初始化一个默认的
        threadPool_ = std::make_unique<EventLoopThreadPool>(loop_, 0);
    }
    threadPool_->start();
    auto subLoops = threadPool_->getAllLoops();

    auto startAcceptor = [this](EventLoop* ioLoop) {
        auto acceptor = std::make_unique<Acceptor>(ioLoop, addr_);
        
        acceptor->newConnectionCallback(
            [this, ioLoop](int sockfd) { 
                this->newConnection(sockfd, ioLoop); 
            }
        );
        acceptor->listen(); 

        
        {
            std::lock_guard<std::mutex> lock(acceptorMutex_);
            subAcceptors_.push_back(std::move(acceptor));
        }
    };
    if (subLoops.empty()) {
        startAcceptor(loop_);
    } else {
        for (auto* subLoop : subLoops) {
            subLoop->runInLoop([subLoop, startAcceptor]() {
                startAcceptor(subLoop);
            });
        }
        
         startAcceptor(loop_); 
    }
}
Server::~Server() {
}
