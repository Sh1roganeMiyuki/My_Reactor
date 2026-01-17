// TcpConnection.h
#pragma once
#include <memory>
#include <functional>
#include <string>
#include "InetAddress.h"
#include "Buffer.h"
// 前置声明，减少头文件引用
class EventLoop;
class Channel;
class Socket; 

class TcpConnection;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*)>;
// ⭐ 必须继承 enable_shared_from_this
// 这样你才能在类内部调用 shared_from_this() 拿到自己的智能指针
class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    TcpConnection(EventLoop* loop, 
                  const std::string& name, 
                  int sockfd, 
                  const InetAddress& localAddr, 
                  const InetAddress& peerAddr);
    ~TcpConnection();

    // ⭐ 核心：连接建立成功后调用
    void connectEstablished();

    // ⭐ 核心：连接断开后调用
    void connectDestroyed();

    void setCloseCallBack(const CloseCallback& cb) { closeCallback_ = cb; }
    
    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }

    std::string name(){ return name_; };
    bool connected() const { return state_ == 2; }
    void send(const std::string& message);

private:
    // 回调函数，给 Channel 用的
    void handleRead();
    void handleClose();
    EventLoop* loop_;
    const std::string name_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    CloseCallback closeCallback_;

    Buffer inputBuffer_;
    Buffer outputBuffer_;
    
    // 这里先用 unique_ptr 管理，以后可以换成你的 Buffer/Socket 类
    std::unique_ptr<Channel> channel_;
    int state_; // 0:Disconnected, 1:Connecting, 2:Connected
};