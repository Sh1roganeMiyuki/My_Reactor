#pragma once
#include <vector>
#include <unordered_set>
#include <memory>
#include "Channel.h"

class TcpConnection;
class EventLoop;

struct TimerEntry {
    std::weak_ptr<TcpConnection> weakConn_;
    explicit TimerEntry(std::weak_ptr<TcpConnection> conn) : weakConn_(conn) {}
    ~TimerEntry(); 
};

class TimerQueue {
public:
    explicit TimerQueue(EventLoop* loop);
    ~TimerQueue();

    void addConnection(const std::shared_ptr<TcpConnection>& conn);
    // 刷新连接
    void refreshConnection(const std::shared_ptr<TcpConnection>& conn);

private:
    void handleRead();

    EventLoop* loop_;
    int timerfd_;
    std::unique_ptr<Channel> timer_channel_;
    
    using EntryPtr = std::shared_ptr<TimerEntry>;
    std::vector<std::unordered_set<EntryPtr>> wheel_;
    size_t current_bucket_;
};