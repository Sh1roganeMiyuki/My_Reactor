#include "Channel.h"
Channel::Channel(int fd , EventLoop *loop) : fd_(fd), loop_(loop), is_in_loop_(false), events_(0), revents_(0){};

Channel::~Channel() {close(this->get_fd());};

void Channel::handle_event(){
    if(tied_){
        std::shared_ptr<void> guard = tie_.lock();
        if(guard){
            handle_event_with_guard(revents_);
        }
    }else{
        handle_event_with_guard(revents_);
    }
}
void Channel::remove(){
    loop_->delChannel(this);
    is_in_loop_ = false;
}
void Channel::update(){
    if(!is_in_loop_){
        loop_->addChannel(this);
        is_in_loop_ = true;
    }else{
        loop_->modChannel(this);
    }
    ////loop_->updateChannel(this);
}
void Channel::enableReading(){
    events_ |= EPOLLIN;
    update();
}

void Channel::tie(const std::shared_ptr<void>& obj){
    tie_ = obj;
    tied_ = true;
}
void Channel::handle_event_with_guard(uint32_t revents){
    if((revents_ & EPOLLERR) && error_callback_){
        error_callback_();
    }
    if((revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) && read_callback_){
        read_callback_();
    }
    if((revents_ & EPOLLOUT) && write_callback_){
        write_callback_();
    }
}