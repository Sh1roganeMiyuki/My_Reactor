#include "Channel.h"


class Acceptor{
public:
    Acceptor(EventLoop* loop, int listenFd);
    ~Acceptor();

    void listen(){ acceptChannel_.enableReading(); }
    void handleRead();
    void newConnectionCallback(const std::function<void(int)>& cb) {
        newConnectionCallback_ = cb;
    }
private:
    int listenFd_;
    EventLoop* loop_;
    Channel acceptChannel_;
    std::function<void(int)> newConnectionCallback_;
};