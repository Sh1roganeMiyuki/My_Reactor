#pragma once
#include "Channel.h"
#include "InetAddress.h"

class Acceptor{
public:
    Acceptor(EventLoop* loop, InetAddress& listenAddr);
    ~Acceptor();

    void listen();
    void handleRead();
    void newConnectionCallback(const std::function<void(int)>& cb) {
        newConnectionCallback_ = cb;
    }
private:
    int listenFd_;
    InetAddress listenAddr_;
    EventLoop* loop_;
    Channel acceptChannel_;
    std::function<void(int)> newConnectionCallback_;
};