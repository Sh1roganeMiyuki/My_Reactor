#include "TimerQueue.h"
#include "EventLoop.h"
#include "TcpConnection.h"
#include <sys/timerfd.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

int createTimerfd() {
    // CLOCK_MONOTONIC: 单调时钟，不受系统时间调整影响
    int tfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (tfd < 0) {
        perror("timerfd_create failed");
    }
    return tfd;
}

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timer_channel_(new Channel(timerfd_, loop)) 
{
    timer_channel_->setReadCallback([this]() { this->handleRead(); });
    timer_channel_->enableReading();
}

TimerQueue::~TimerQueue() {
    timer_channel_->disableAll();
    timer_channel_->remove();
    ::close(timerfd_);
}

void TimerQueue::addConnection(const std::shared_ptr<TcpConnection>& conn) {
    auto now = std::chrono::steady_clock::now();
    // 30秒 后过期
    auto expire = now + std::chrono::seconds(kKeepAliveTimeout);
    
    conn->keepAlive(); // 初始刷新
    timers_.insert({expire, conn});

    if (timers_.begin()->expire == expire) {
        resetTimerfd();
    }
}

void TimerQueue::handleRead() {
    uint64_t one;
    ssize_t n = ::read(timerfd_, &one, sizeof one);
    (void)n;

    auto now = std::chrono::steady_clock::now();

    while (!timers_.empty()) {
        // 拿到最早过期的那个
        auto it = timers_.begin();
        
        if (it->expire > now) break;

        auto node = *it;
        timers_.erase(it); // 移除

        auto conn = node.connection.lock();
        if (conn) {
            // 检查它最近活跃时间 + 30s 是否大于现在
            auto lastActive = conn->getLastActiveTime();
            auto realExpire = lastActive + std::chrono::seconds(kKeepAliveTimeout);
            
            if (realExpire > now) {
                // 重新插入 set
                timers_.insert({realExpire, node.connection});
            } else {
                 std::cout << "Timeout shutting down: " << conn->name() << std::endl;
                conn->handleClose(); 
            }
        }
    }
    resetTimerfd();
}

void TimerQueue::resetTimerfd() {
    struct itimerspec newValue;
    memset(&newValue, 0, sizeof newValue);

    if (!timers_.empty()) {
        auto nextExpire = timers_.begin()->expire;
        auto now = std::chrono::steady_clock::now();
        
        // 计算时间差 (微秒)
        int64_t delay = std::chrono::duration_cast<std::chrono::microseconds>(nextExpire - now).count();
        if (delay < 100) delay = 100; // 最小延时，防止负数

        newValue.it_value.tv_sec = delay / 1000000;
        newValue.it_value.tv_nsec = (delay % 1000000) * 1000;
    }
    
    // 如果 set 空了，newValue 全为 0，相当于关闭定时器
    ::timerfd_settime(timerfd_, 0, &newValue, nullptr);
}