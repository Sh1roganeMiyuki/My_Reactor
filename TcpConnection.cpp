// TcpConnection.cpp
#include "TcpConnection.h"
#include "Channel.h"
#include "EventLoop.h"
#include <functional> // for std::bind

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
    // 设置 Channel 的回调
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this)
    );
    
}

TcpConnection::~TcpConnection(){

}


void TcpConnection::connectEstablished() {
    state_ = 2; // Connected

    // 此时对象已经构造完成（在 Server 里已经用 make_shared 生成了）
    // 所以可以安全调用 shared_from_this()
    channel_->tie(shared_from_this());
    
    
    // 开启监听
    channel_->enableReading();
}

void TcpConnection::handleRead() {
    int savedErrno = 0;
    
    // 1. 也就是这一行，调用了你之前写的 Buffer::readFd
    // 利用 readv 直接把 Socket 里的数据读进 inputBuffer_
    ssize_t n = inputBuffer_.readFd(channel_->get_fd(), &savedErrno);

    if (n > 0) {
        // ✅ 读到了数据
        // 现在的状态：数据已经在 inputBuffer_ 里了
        // 我们只需要把 "this" 和 "buffer" 传给用户
        // 用户想读多少、想怎么解析（解决粘包），全看用户心情
        if (messageCallback_) {
            messageCallback_(shared_from_this(), &inputBuffer_);
        }
    } 
    else if (n == 0) {
        // ❌ 读到 0，意味着对方 FIN
        handleClose();
    } 
    else {
        // ⚠️ 出错
        errno = savedErrno;
        std::cerr << "TcpConnection::handleRead error" << std::endl;
        //handleError();
    }
}

void TcpConnection::handleClose() {
    //std::cout << "TcpConnection::handleClose state=" << state_ << std::endl;
    if (state_ == 2) { // Connected
        state_ = 0; // Disconnected
        channel_->disableAll(); // 停止监听
        
        // 必须保存一份 shared_ptr，防止在回调执行时自己被析构
        auto guard = shared_from_this();
        
        // 通知 Server 删掉我
        if (closeCallback_) {
            closeCallback_(guard);
        }
    }
}

void TcpConnection::connectDestroyed() {
    if (state_ == 2) {
        state_ = 0;
        channel_->disableAll(); 
        channel_->remove(); // 从 epoll 彻底移除
    }
    //std::cout << "TcpConnection::dtor cleanup done." << std::endl;
}

void TcpConnection::send(const std::string &message){
    if(state_ == 2){
        size_t n = ::write(channel_->get_fd(), message.data(), message.size());
        (void)n; 
    }
}