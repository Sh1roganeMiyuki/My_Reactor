#include "TimerQueue.h"
#include "EventLoop.h"
#include "TcpConnection.h"
#include <sys/timerfd.h>
#include <unistd.h>
#include <iostream>

// 让 timerfd 变成周期性的！每 1 秒自动响一次
int createTimerfd() {
    int tfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    struct itimerspec newValue{};
    // 初始延迟 1 秒
    newValue.it_value.tv_sec = 1; 
    newValue.it_value.tv_nsec = 0;
    // 之后每 1 秒触发一次
    newValue.it_interval.tv_sec = 1; 
    newValue.it_interval.tv_nsec = 0;
    ::timerfd_settime(tfd, 0, &newValue, nullptr);
    return tfd;
}

TimerEntry::~TimerEntry() {
    auto conn = weakConn_.lock();
    if (conn) {
        conn->handleClose(); 
    }
}

// 初始化时间轮，大小为 30（即 30 秒超时）
TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timer_channel_(new Channel(timerfd_, loop)),
      wheel_(30),             // 30 个桶
      current_bucket_(0)      // 初始指针指向 0
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
    // 1. 创建包裹对象
    auto entry = std::make_shared<TimerEntry>(conn);
    // 2. 让连接记住自己的包裹
    conn->setTimerEntry(entry);
    // 3. 放到 29 秒后的槽位里 (当前时间 + 29)
    size_t next_bucket = (current_bucket_ + 29) % wheel_.size();
    wheel_[next_bucket].insert(entry);
}

void TimerQueue::refreshConnection(const std::shared_ptr<TcpConnection>& conn) {
    // 拿到之前的包裹对象
    auto entry = std::static_pointer_cast<TimerEntry>(conn->getTimerEntry());
    if (entry) {
        // 直接再扔进最新槽位！旧槽位不用管，时间到了它会自动释放一层引用计数
        size_t next_bucket = (current_bucket_ + 29) % wheel_.size();
        wheel_[next_bucket].insert(entry);
    }
}

void TimerQueue::handleRead() {
    // 必须读出 timerfd 的 8 字节，否则会一直触发 epoll
    uint64_t one;
    ::read(timerfd_, &one, sizeof one);

    current_bucket_ = (current_bucket_ + 1) % wheel_.size();
    
    // std::cout << "DEBUG: Timer tick, bucket index: " << current_bucket_ 
    //           << " size: " << wheel_[current_bucket_].size() << std::endl;

    wheel_[current_bucket_].clear(); 
}