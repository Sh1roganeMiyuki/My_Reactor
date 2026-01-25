#pragma once
#include <set>
#include <vector>
#include <memory>
#include <chrono>
#include "Channel.h"

class EventLoop;
class TcpConnection;

struct TimerNode {
    std::chrono::steady_clock::time_point expire;
    // 使用 weak_ptr 防止循环引用，不阻碍连接销毁
    std::weak_ptr<TcpConnection> connection; 
 
    // 重载 < 用于 set 排序
    bool operator<(const TimerNode& other) const {
        if (expire != other.expire) 
            return expire < other.expire;
        // 如果时间完全相同，比较指针地址，保证 set 不去重
        return connection.lock().get() < other.connection.lock().get();
    }
};

class TimerQueue {
public:
    explicit TimerQueue(EventLoop* loop);
    ~TimerQueue();

    // 添加新连接的监控
    void addConnection(const std::shared_ptr<TcpConnection>& conn);

private:
    // timerfd 触发时的回调
    void handleRead();
    
    // 重新设置 timerfd 的内核闹钟
    void resetTimerfd();

    EventLoop* loop_;
    int timerfd_;
    std::unique_ptr<Channel> timer_channel_;
    
    // 红黑树：按过期时间自动排序
    std::set<TimerNode> timers_;
    
    // 超时时间30s
    static const int kKeepAliveTimeout = 30; 
};