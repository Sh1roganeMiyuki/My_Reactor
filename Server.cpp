#include "Server.h"
#include <iostream>

//
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>

//std::atomic<int64_t> g_totalRequestCount(0);

// 

Server::Server(InetAddress& addr, EventLoop* loop) : addr_(addr), acceptor_(loop, addr_), loop_(loop) 
{
    acceptor_.newConnectionCallback(
        [this](int sockfd) { this->newConnection(sockfd); }
    );

}
void Server::start() {
    acceptor_.listen();
}
Server::~Server() {
}
void Server::newConnection(int sockfd) {
    // 1. 创建一个新的 Channel 来管理这个客户端连接
    // 注意：这里我们手动 new，后面断开时记得 delete
    Channel* connChannel = new Channel(sockfd, loop_ );
    
    // 2. 放入 map 管理
    channels_[sockfd] = connChannel;

    // 3. 设置读回调 (核心业务：Echo)
    connChannel->setReadCallback([this, sockfd, connChannel]() {
        char buf[1024];
        ssize_t n = ::read(sockfd, buf, sizeof(buf));
        // 
        //g_totalRequestCount++; 
        // 
        if (n > 0) {
            // 【收到数据 -> 原样发回】
            // 此时是单线程，直接 write 是安全的（虽然简陋，不考虑缓冲区满的情况）
            ::write(sockfd, buf, n); 
        } 
        else {
            // 【对方断开 (n=0) 或 出错 (n<0)】
            connChannel->disableAll();
            connChannel->remove(); // 从 epoll 摘除
            
            ::close(sockfd);       // 关闭 fd
            
            // 清理内存
            this->channels_.erase(sockfd);
            delete connChannel; 
        }
    });

    // 4. 开启读权限 (加入 epoll)
    connChannel->enableReading();
    
    // printf("New connection fd=%d established.\n", sockfd);
}