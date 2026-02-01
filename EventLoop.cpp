#include "EventLoop.h"
#include "Channel.h"  
#include "Epoller.h"
#include "sys/eventfd.h"
#include "TimerQueue.h"
class Epoller;
class Channel;


int createEventfd() {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        perror("Failed in eventfd");
        exit(1);
    }
    return evtfd;
}

EventLoop::EventLoop() 
      : looping_(false),
        quit_(false),
        thread_id_(std::this_thread::get_id()),
        server_epoller(std::make_unique<Epoller>()),
        timer_queue_(std::make_unique<TimerQueue>(this)),
        wakeup_fd_(createEventfd()),
        wakeup_channel_(std::make_unique<Channel>(wakeup_fd_, this)), 
        calling_pending_functors_(false) {
    wakeup_channel_->setReadCallback([this]() { this->handleRead(); });
    wakeup_channel_->enableReading();
}

EventLoop::~EventLoop() {
    wakeup_channel_->disableAll();
    wakeup_channel_->remove();
    ::close(wakeup_fd_);
}

void EventLoop::wakeup(){
    uint64_t one = 1;
    ssize_t n = ::write(wakeup_fd_, &one, sizeof one);
    if (n != sizeof one) {
        std::cerr << "EventLoop::wakeup() writes " << n << " bytes instead of 8" << std::endl;
    }
}
void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = ::read(wakeup_fd_, &one, sizeof one);
    if (n != sizeof one) {
        std::cerr << "EventLoop::handle_read() reads " << n << " bytes instead of 8" << std::endl;
    }
}
void EventLoop::quit() {
    quit_ = true;
    // 如果是在其他线程调用 quit，必须唤醒 loop 才能让它跳出 while
    if (!isInLoopThread()) {
        wakeup();
    }
}
void EventLoop::runInLoop(Functor cb){
    if(isInLoopThread()){
        cb();
    }else{
        queueInLoop(std::move(cb));
    }
}   
void EventLoop::queueInLoop(Functor cb){
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_functors_.emplace_back(std::move(cb));
    }

    if(!isInLoopThread() || calling_pending_functors_){
        wakeup();
    }
}



void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    calling_pending_functors_ = true;

    {
        // 关键优化：交换，减小锁的粒度
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pending_functors_);
    }

    for (const auto& func : functors) {
        func();
    }
    calling_pending_functors_ = false;
}



void EventLoop::addTimer(const std::shared_ptr<TcpConnection>& conn) {
    timer_queue_->addConnection(conn);
}

void EventLoop::loop() {
    while (!quit_) {
        std::vector<Channel *> active_channels;
        active_channels.clear();
        server_epoller->wait(1, active_channels);
        for (auto channel : active_channels) {
            channel->handle_event();
        }
        doPendingFunctors();
    }
    looping_ = false;
}
void EventLoop::addChannel(Channel *channel){
    server_epoller->add_channel(channel);
}
void EventLoop::modChannel(Channel *channel){
    server_epoller->mod_channel(channel);
}
void EventLoop::delChannel(Channel *channel){
    server_epoller->del_channel(channel);
}