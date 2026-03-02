#include "Server.h"
#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>

Server::Server(InetAddress& addr, EventLoop* loop) : addr_(addr), loop_(loop) 
{
    
}
void Server::newConnection(int sockfd, EventLoop* ioLoop) {
    if (sockfd < 0) return;

    std::string connName = "Conn-" + std::to_string(sockfd);
    
    InetAddress localAddr("127.0.0.1", 8000); 
    InetAddress peerAddr("127.0.0.1", 0);     

    auto conn = std::make_shared<TcpConnection>(
        ioLoop, connName, sockfd, localAddr, peerAddr
    );

    {
        std::lock_guard<std::mutex> lock(connMutex_);
        connections_[connName] = conn;
    }

    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setCloseCallBack(
        std::bind(&Server::removeConnection, this, std::placeholders::_1)
    );

    conn->connectEstablished();
}

void Server::removeConnection(const std::shared_ptr<TcpConnection>& conn) {
    {
        std::lock_guard<std::mutex> lock(connMutex_);
        connections_.erase(conn->name());
    }
    
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn)
    );
}
void Server::start() {
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
