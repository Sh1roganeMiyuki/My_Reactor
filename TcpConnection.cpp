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
    last_active_time_ = std::chrono::steady_clock::now();
    int opt = 1;
    int ret = ::setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof opt);
    if (ret < 0) {
        perror("setsockopt failed"); //
    }
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this)
    );
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this)
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
    keepAlive();
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
    keepAlive();
}

// 优化
void TcpConnection::send(const void* data, size_t len) {
    if (state_ != 2) return; 

    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    // outputBuffer 为空，且当前没有在关注写事件
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        nwrote = ::write(channel_->get_fd(), data, len);
        
        if (nwrote >= 0) {
            remaining = len - nwrote;
            // 如果 remaining == 0，说明一次性全发完了
        } else { 
            nwrote = 0; // 没写进去
            if (errno != EWOULDBLOCK) { // 缓冲区满
                // 如果是 EPIPE 或 ECONNRESET，说明连接断了
                if (errno == EPIPE || errno == ECONNRESET) {
                    faultError = true;
                }
            }
        }
    }

    // 还有剩余数据
    if (!faultError && remaining > 0) {
        // 拷贝到应用层缓冲区
        outputBuffer_.append(static_cast<const char*>(data) + nwrote, remaining);
        
        // 关注 EPOLLOUT 事件
        // 当内核缓冲区变空时，epoll会通知调用handleWrite
        if (!channel_->isWriting()) {
            channel_->enableWriting(); 
        }
    }
}

void TcpConnection::handleWrite() {
    if (channel_->isWriting()) {
        int savedErrno = 0;
        // peek() 返回可读指针，readableBytes() 返回长度
        ssize_t n = ::write(channel_->get_fd(), 
                            outputBuffer_.peek(), 
                            outputBuffer_.readableBytes());
        
        if (n > 0) {
            // 移动 readIndex
            outputBuffer_.retrieve(n);
            
            // 发完
            if (outputBuffer_.readableBytes() == 0) {
                // 取消关注 EPOLLOUT，防止空转
                channel_->disableWriting();
            }
        } else {
            if (errno != EWOULDBLOCK) {
                handleClose(); 
            }
        }
    }
}