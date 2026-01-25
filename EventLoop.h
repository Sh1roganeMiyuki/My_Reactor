#pragma once
#include <iostream>
#include <vector>
#include <functional>
#include "Epoller.h"
#include <memory> // for unique_ptr
        #include "thread"
#include "mutex"
class Epoller;
class Channel;
class TimerQueue;
class TcpConnection;
class EventLoop {
public:
    //using Functor = std::function<void()>;
    EventLoop();
    ~EventLoop();
    void loop();
    //void quit();


    //void wakeup();
    //void runInLoop(Functor cb);
    //void queueInLoop(Functor cb);

    void addChannel(Channel *channel);
    void modChannel(Channel *channel);
    void delChannel(Channel *channel);

    void addTimer(const std::shared_ptr<TcpConnection>& conn);

    //bool isInLoopThread() const { return thread_id_ == std::this_thread::get_id(); }
private:
    //void handle_read();         // 读门铃
   // void doPendingFunctors();   // 读任务队列

    //const std::thread::id thread_id_;

    //bool looping_;
    //bool quit_;

    //int wakeup_fd_;
    //std::unique_ptr<Channel> wakeup_channel_;

    std::unique_ptr<Epoller> server_epoller;
    
    std::unique_ptr<TimerQueue> timer_queue_;
    //bool calling_pending_functors_;         // 防止死锁
    //std::vector<Functor> pending_functors_; // 信箱
    //std::mutex mutex_;
};