#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>

class EventLoop;

class EventLoopThread {
public:
    EventLoopThread();
    ~EventLoopThread();

    EventLoop* startLoop(); // 启动线程并返回 loop 指针

private:
    void threadFunc();

    EventLoop* loop_;
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
};