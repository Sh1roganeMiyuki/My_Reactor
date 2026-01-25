#pragma once
#include <memory>
#include <functional>
#include <string>
#include <any> // C++17
#include "InetAddress.h"
#include "Buffer.h"
#include <chrono>
class EventLoop;
class Channel;
class Socket; 
class TcpConnection;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*)>;

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    TcpConnection(EventLoop* loop, 
                  const std::string& name, 
                  int sockfd, 
                  const InetAddress& localAddr, 
                  const InetAddress& peerAddr);
    ~TcpConnection();

    void connectEstablished();
    void connectDestroyed();

    void setCloseCallBack(const CloseCallback& cb) { closeCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }

    std::string name() const { return name_; } // 加了 const
    bool connected() const { return state_ == 2; }

    // 增加重载，支持零拷贝发送
    void send(const std::string& message);
    void send(const void* data, size_t len); 

    // Context 上下文支持
    void setContext(const std::any& context) { context_ = context; }
    std::any* getMutableContext() { return &context_; }
    bool hasContext() const { return context_.has_value(); }

    void keepAlive() { last_active_time_ = std::chrono::steady_clock::now(); }

    std::chrono::steady_clock::time_point getLastActiveTime() const { return last_active_time_;}

    void handleClose();
private:
    void handleRead();
    
    EventLoop* loop_;
    const std::string name_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    CloseCallback closeCallback_;

    Buffer inputBuffer_;
    Buffer outputBuffer_;

    std::any context_;

    std::chrono::steady_clock::time_point last_active_time_;
    
    std::unique_ptr<Channel> channel_;
    int state_; // 0:Disconnected, 1:Connecting, 2:Connected
};