#include "TcpConnection.h"
#include "Channel.h"
#include "EventLoop.h"
#include <functional>
#include <iostream>
#include <unistd.h> 
#include <netinet/tcp.h> 

TcpConnection::TcpConnection(EventLoop* loop, 
                             const std::string& name, 
                             int sockfd,
                             const InetAddress& localAddr, 
                             const InetAddress& peerAddr)
    : loop_(loop),
      name_(name),
      state_(1), 
      channel_(new Channel(sockfd, loop)) 
{
    int opt = 1;
    int ret = ::setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof opt);
    if (ret < 0) {
        perror("setsockopt failed"); //
    }
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this)
    );
}

TcpConnection::~TcpConnection(){
}

void TcpConnection::connectEstablished() {
    state_ = 2; // Connected
    channel_->tie(shared_from_this());
    channel_->enableReading();

    if (connectionCallback_) {
        connectionCallback_(shared_from_this());
    }
}

void TcpConnection::handleRead() {
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->get_fd(), &savedErrno);

    if (n > 0) {
        if (messageCallback_) {
            messageCallback_(shared_from_this(), &inputBuffer_);
        }
    } 
    else if (n == 0) {
        handleClose();
    } 
    else {
        errno = savedErrno;
        handleClose(); // 出错直接关闭
    }
}

void TcpConnection::handleClose() {
    if (state_ == 2) { 
        state_ = 0; 
        channel_->disableAll(); 
        
        auto guard = shared_from_this();
        if (closeCallback_) {
            closeCallback_(guard);
        }
    }
}

void TcpConnection::connectDestroyed() {
    if (state_ == 2) {
        state_ = 0;
        channel_->disableAll(); 
        channel_->remove(); 
    }
}

// 旧接口复用新逻辑
void TcpConnection::send(const std::string &message){
    send(message.data(), message.size());
}

// 优化
void TcpConnection::send(const void* data, size_t len) {
    if (state_ == 2) {
        ssize_t nwrote = ::write(channel_->get_fd(), data, len);
        
        if (nwrote < 0) {
        }
    }
}