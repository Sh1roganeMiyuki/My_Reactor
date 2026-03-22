#pragma once
#include <vector>
#include <unordered_set>
#include <memory>
#include "Channel.h"

class TcpConnection;
class EventLoop;

// 魔法对象：利用 RAII 机制自动断开超时连接
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
    // 新增：刷新连接（心跳）
    void refreshConnection(const std::shared_ptr<TcpConnection>& conn);

private:
    void handleRead();

    EventLoop* loop_;
    int timerfd_;
    std::unique_ptr<Channel> timer_channel_;
    
    // 时间轮核心数据结构：30 个槽位，每个槽位是一个哈希集合
    using EntryPtr = std::shared_ptr<TimerEntry>;
    std::vector<std::unordered_set<EntryPtr>> wheel_;
    size_t current_bucket_;
};