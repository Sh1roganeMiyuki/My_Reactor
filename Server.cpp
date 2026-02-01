#include "Server.h"
#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>

Server::Server(InetAddress& addr, EventLoop* loop) : addr_(addr), acceptor_(loop, addr_), loop_(loop) 
{
    threadPool_ = std::make_unique<EventLoopThreadPool>(loop_, 0);
    acceptor_.newConnectionCallback(
        [this](int sockfd) { this->newConnection(sockfd); }
    );
    
}
void Server::newConnection(int sockfd) {
    if (sockfd < 0) return; 
    
    EventLoop* ioLoop = threadPool_->getNextLoop();
    if (!ioLoop) {
        std::cerr << "ioLoop is null" << std::endl;
        return;
    }
    std::string connName = "Conn-" + std::to_string(sockfd);
    InetAddress localAddr("127.0.0.1", 8080);
    InetAddress peerAddr("127.0.0.1", 0);
    
    auto conn = std::make_shared<TcpConnection>(
        ioLoop, connName, sockfd, localAddr, peerAddr
    );

    connections_[connName] = conn;


    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setCloseCallBack(
    [this](const std::shared_ptr<TcpConnection>& conn) { 
        this->removeConnection(conn); 
    }
);
    // 将 conn 放入 map
    connections_[connName] = conn;

    
    conn->connectEstablished();
    loop_->addTimer(conn); 


    ioLoop->runInLoop([conn]() {
        conn->connectEstablished();
    });
}

void Server::removeConnection(const std::shared_ptr<TcpConnection>& conn) {
    loop_->runInLoop([this, conn]() {
        if (connections_.erase(conn->name()) == 0) return;

        // 关键：不要在这里让它析构！
        // 我们把 conn 指针再次通过 std::bind 传给它所属的子线程。
        // 这会使得引用计数重新 +1，保证了在任务执行完之前，对象不会死。
        EventLoop* ioLoop = conn->getLoop();
        
        // 我们让子线程去执行 connectDestroyed
        ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    });
}
void Server::start() {
    threadPool_->start();
    acceptor_.listen();
}
Server::~Server() {
}
