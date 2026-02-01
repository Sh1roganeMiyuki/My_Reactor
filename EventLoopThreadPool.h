#pragma once
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool {
public:
    EventLoopThreadPool(EventLoop* baseLoop, int numThreads); // numThreads: 线程数
    ~EventLoopThreadPool();

    void start();
    EventLoop* getNextLoop(); // 获取下一个 Loop

private:
    EventLoop* baseLoop_;
    int numThreads_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;
};