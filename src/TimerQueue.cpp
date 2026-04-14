#include "TimerQueue.h"
#include "EventLoop.h"
#include "TcpConnection.h"
#include <sys/timerfd.h>
#include <unistd.h>
#include <iostream>

// 每 1 秒响一次
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

// 初始化时间轮
TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      //timer_channel_(new Channel(timerfd_, loop)),
      wheel_(30),             // 30 个桶
      current_bucket_(0)      // 初始指针指向 0
{
    // timer_channel_->setReadCallback([this]() { this->handleRead(); });
    // timer_channel_->enableReading();
    for(auto& bucket : wheel_) {
        // 预留空间
        bucket.reserve(10000); 
    }
}

TimerQueue::~TimerQueue() {
    timer_channel_->disableAll();
    timer_channel_->remove();
    ::close(timerfd_);
}

void TimerQueue::addConnection(const std::shared_ptr<TcpConnection>& conn) {
    // 这里我们假设 TcpConnection 已经自己管理了一个 TimerEntry
    // 如果没有，可以调用 conn->getOrCreateTimerEntry()
    auto entry = conn->getTimerEntry(); 
    
    size_t next_bucket = (current_bucket_ + 29) % wheel_.size();
    
    // vector::push_back 在 reserve 过的空间内是 0 分配的
    wheel_[next_bucket].push_back(entry);
}

void TimerQueue::refreshConnection(const std::shared_ptr<TcpConnection>& conn) {
    // 获取连接自带的那个持久 entry
    auto entry = conn->getIdleTimerEntry(); 
    
    // push 到 29 秒后的桶
    size_t next_bucket = (current_bucket_ + 29) % wheel_.size();
    
    //  0 分配操作
    wheel_[next_bucket].push_back(entry); 
}

void TimerQueue::handleRead() {
    // 读出 timerfd 的 8 字节，否则一直触发 epoll
    uint64_t one;
    ::read(timerfd_, &one, sizeof one);

    current_bucket_ = (current_bucket_ + 1) % wheel_.size();
    
    // std::cout << "DEBUG: Timer tick, bucket index: " << current_bucket_ 
    //           << " size: " << wheel_[current_bucket_].size() << std::endl;

    wheel_[current_bucket_].clear(); 
}