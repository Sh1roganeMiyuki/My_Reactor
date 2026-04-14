#include "TcpConnection.h"
#include "Channel.h"
#include "EventLoop.h"
#include <functional>
#include <iostream>
#include <unistd.h> 
#include <netinet/tcp.h> 

// TcpConnection::TcpConnection(EventLoop* loop, 
//                              const std::string& name, 
//                              int sockfd,
//                              const InetAddress& localAddr, 
//                              const InetAddress& peerAddr)
//     : loop_(loop),
//       name_(name),
//       state_(1), 
//       channel_(new Channel(sockfd, loop)),
//       last_timer_push_time_(std::chrono::steady_clock::now())
// {
//     last_timer_refresh_time_ = std::chrono::steady_clock::now();
//     //last_active_time_ = std::chrono::steady_clock::now();
//     int opt = 1;
//     int ret = ::setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof opt);
//     if (ret < 0) {
//         perror("setsockopt failed"); //
//     }
//     channel_->setReadCallback(
//         std::bind(&TcpConnection::handleRead, this)
//     );
//     channel_->setWriteCallback(
//         std::bind(&TcpConnection::handleWrite, this)
//     );
// }

TcpConnection::TcpConnection()
    : loop_(nullptr),
      state_(0),
      channel_(new Channel(-1, nullptr)) {
}

void TcpConnection::reset(EventLoop* loop, 
                          int sockfd,
                          const InetAddress& localAddr, 
                          const InetAddress& peerAddr) {
    loop_ = loop;
    state_ = 1; // Connecting

    // 重置 Channel
    channel_->reset(sockfd, loop_); 

    if (!idle_timer_entry_) {
        // 这里的 TimerEntry 内部持有 TcpConnection 的 weak_ptr
        idle_timer_entry_ = std::make_shared<TimerEntry>(shared_from_this());
    }

    // 保留内存，重置索引
    inputBuffer_.retrieveAll(); 
    outputBuffer_.retrieveAll();
    
    context_.reset(); // 清空 std::any 上下文
    
    // 重新绑定回调
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
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

std::shared_ptr<TimerEntry> TcpConnection::getTimerEntry() {
    if (!timer_entry_) {
        // 仅在第一次时分配。
        timer_entry_ = std::make_shared<TimerEntry>(shared_from_this());
    }
    return timer_entry_;
}

void TcpConnection::handleRead() {
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->get_fd(), &savedErrno);
    keepAlive();
    if (n > 0) {
        auto now = std::chrono::steady_clock::now();
        // 将连接重新扔进时间轮
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_timer_push_time_).count() >= 1) {
            loop_->refreshTimer(shared_from_this()); 
            last_timer_push_time_ = now; // 更新最后一次刷新的时间
        }
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
    if (state_ == 2 || state_ == 0) {
        state_ = 0;
        channel_->disableAll(); 
        channel_->remove(); 
        if(channel_->get_fd() != -1) {
            ::close(channel_->get_fd());
            channel_->reset(-1, nullptr); // 置空 fd 避免析构重复关闭
        }
    }
}

// 旧接口复用新逻辑
void TcpConnection::send(const std::string &message){
    send(message.data(), message.size());
    keepAlive();
}
void TcpConnection::send(const void* data, size_t len) {
    if (loop_->isInLoopThread()) {
        sendInLoop(data, len);
    } else {
        // 深拷贝数据， data 指针在回调执行时可能已失效
        std::string message(static_cast<const char*>(data), len);
        
        // 捕获 shared_ptr 保证 conn 活着，捕获 string 副本
        loop_->runInLoop(
            [conn = shared_from_this(), msg = std::move(message)]() {
                conn->sendInLoop(msg.data(), msg.size());
            }
        );
    }
}
void TcpConnection::sendInLoop(const void* data, size_t len) {
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