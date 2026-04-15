#include "TcpConnection.h"
#include "Channel.h"
#include "EventLoop.h"
#include <functional>
#include <iostream>
#include <unistd.h> 
#include <netinet/tcp.h> 
#include <linux/errqueue.h>
#include <linux/net_tstamp.h> 
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

    int one = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_ZEROCOPY, &one, sizeof(one));

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
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
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
    //handleError(); 
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

    // 1. 只有在没有积压数据时，才尝试直接发送
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        // 尝试零拷贝发送
        nwrote = ::send(channel_->get_fd(), data, len, MSG_ZEROCOPY);
        
        if (nwrote >= 0) {
            remaining = len - nwrote;
        } else { 
            nwrote = 0; 
            // !!! 核心修复点：处理 ENOBUFS
            // 当零拷贝通知队列（Error Queue）满了，或者内核锁定内存达到上限时
            if (errno == ENOBUFS) {
                //handleError(); 
                // 此时内核拒绝了零拷贝请求。为了不让 QPS 归零，我们必须降级为普通发送
                // 这次发送会发生内存拷贝，但它保证了连接不会断流
                nwrote = ::send(channel_->get_fd(), data, len, 0); 
                if (nwrote >= 0) {
                    remaining = len - nwrote;
                }
            } else if (errno != EWOULDBLOCK) {
                // 真正的网络错误
                if (errno == EPIPE || errno == ECONNRESET) {
                    faultError = true;
                }
            }
        }
    }

    // 2. 处理没发完或者因为缓冲区满（EWOULDBLOCK）剩下的数据
    if (!faultError && remaining > 0) {
        // 数据进入 outputBuffer_，这里不可避免地会发生一次 memcpy
        outputBuffer_.append(static_cast<const char*>(data) + nwrote, remaining);
        
        // 注册写事件
        if (!channel_->isWriting()) {
            channel_->enableWriting(); 
        }
    }
}

void TcpConnection::handleWrite() {
    if (channel_->isWriting()) {
        // 这里的内存已经是拷贝过的了，直接用普通 send 发出去，清空缓冲区
        ssize_t n = ::send(channel_->get_fd(), 
                           outputBuffer_.peek(), 
                           outputBuffer_.readableBytes(),
                           0); // 必须为 0
        
        if (n > 0) {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0) {
                channel_->disableWriting();
            }
        } else if (errno != EWOULDBLOCK) {
            handleClose(); 
        }
    }
}

void TcpConnection::handleError() {
    // 1. 必须初始化 msghdr 和 iovec
    // 即便我们只关心控制消息（Notification），recvmsg 也要求必须有 iov 结构
    struct msghdr msg = {};
    struct iovec iov;
    char dummy_buf[1024];     // 接收数据的占位缓冲区
    char control_buf[1024];   // 真正存放内核通知（cmsghdr）的缓冲区

    iov.iov_base = dummy_buf;
    iov.iov_len = sizeof(dummy_buf);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control_buf;
    msg.msg_controllen = sizeof(control_buf);

    // 2. 必须使用 while 循环清空错误队列
    // 在高并发下，内核可能一次性返回多个发送完成的通知
    while (true) {
        // MSG_ERRQUEUE 标志告诉内核：我们要读的是错误队列，而不是普通数据
        ssize_t res = ::recvmsg(channel_->get_fd(), &msg, MSG_ERRQUEUE);
        
        if (res < 0) {
            // 如果返回 EAGAIN 或 EINTR，说明当前队列已读完或被中断
            break;
        }

        // 3. 解析控制消息 (Control Message)
        for (struct cmsghdr *cm = CMSG_FIRSTHDR(&msg); cm != nullptr; cm = CMSG_NXTHDR(&msg, cm)) {
            // 检查是否是我们要的 IP 层错误通知
            if (cm->cmsg_level == SOL_IP && cm->cmsg_type == IP_RECVERR) {
                struct sock_extended_err *see = (struct sock_extended_err *)CMSG_DATA(cm);
                
                // 确认是来自零拷贝（ZEROCOPY）的通知
                if (see->ee_origin == SO_EE_ORIGIN_ZEROCOPY) {
                    // see->ee_info: 起始序列号 (First ID)
                    // see->ee_data: 结束序列号 (Last ID)
                    // 在这个范围 [ee_info, ee_data] 内的所有 send 调用对应的内存都已经发送完成
                    
                    uint32_t first_id = see->ee_info;
                    uint32_t last_id = see->ee_data;

                    
                   // printf("ZeroCopy Notify: ID %u to %u complete.\n", first_id, last_id);
                    
                }
            }
        }
        
        // 每次循环后重置 msg_controllen，否则下次 recvmsg 会报错
        msg.msg_controllen = sizeof(control_buf);
    }
}