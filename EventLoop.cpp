#include "EventLoop.h"
#include "Channel.h"  
#include "Epoller.h"
#include "sys/eventfd.h"
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

// EventLoop::EventLoop() 
//       : server_epoller(std::make_unique<Epoller>()),
//         looping_(false),
//         quit_(false),
//         thread_id_(std::this_thread::get_id()),
//         wakeup_fd_(createEventfd()),
//         wakeup_channel_(std::make_unique<Channel>(wakeup_fd_)), 
//         calling_pending_functors_(false) {
//     wakeup_channel_->setReadCallback([this]() { this->handle_read(); });
//     wakeup_channel_->enableReading();
// }
EventLoop::EventLoop() 
      : server_epoller(std::make_unique<Epoller>()) {
}
EventLoop::~EventLoop() {
    // wakeup_channel_->;
    // ::close(wakeup_fd_);
}

// void EventLoop::wakeup(){
//     uint64_t one = 1;
//     ssize_t n = ::write(wakeup_fd_, &one, sizeof one);
//     if (n != sizeof one) {
//         std::cerr << "EventLoop::wakeup() writes " << n << " bytes instead of 8" << std::endl;
//     }
// }
// void EventLoop::handle_read() {
//     uint64_t one = 1;
//     ssize_t n = ::read(wakeup_fd_, &one, sizeof one);
//     if (n != sizeof one) {
//         std::cerr << "EventLoop::handle_read() reads " << n << " bytes instead of 8" << std::endl;
//     }
// }
// void EventLoop::runInLoop(Functor cb){
//     if(isInLoopThread()){
//         cb();
//     }else{
//         queueInLoop(std::move(cb));
//     }
// }   
// void EventLoop::queueInLoop(Functor cb){
//     {
//         std::lock_guard<std::mutex> lock(mutex_);
//         pending_functors_.emplace_back(std::move(cb));
//     }

//     if(!isInLoopThread() || calling_pending_functors_){
//         wakeup();
//     }
// }
void EventLoop::loop() {
    while (true) {
        std::vector<Channel *> active_channels;
        active_channels.clear();
        server_epoller->wait(1, active_channels);
        for (auto channel : active_channels) {
            channel->handle_event();
        }
    }
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