#include "Epoller.h"
#include "Channel.h"
Epoller::Epoller() : epoll_fd_(epoll_create1(0)) {}
Epoller::~Epoller() { close(epoll_fd_); }

void Epoller::add_channel(Channel* channel) {
    epoll_event event;
    event.data.ptr = channel;
    event.events = channel->get_events();
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, channel->get_fd(), &event);
    channel_map_[channel->get_fd()] = channel;
    //channel->modInEpoll(true);
}
void Epoller::mod_channel(Channel* channel) {
    epoll_event event;
    event.data.ptr = channel;
    event.events = channel->get_events();
    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, channel->get_fd(), &event);
    //channel->modInEpoll(true);
}
void Epoller::del_channel(Channel* channel) {
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, channel->get_fd(), nullptr);
    channel_map_.erase(channel->get_fd());
    //channel->modInEpoll(false);
}
void Epoller::wait(int timeout_ms, std::vector<Channel*>& active_channels) {
    const int MAX_EVENTS = 1024;
    std::vector<epoll_event> events(MAX_EVENTS);
    int nfds = epoll_wait(epoll_fd_, events.data(), MAX_EVENTS, timeout_ms);
    for (int i = 0; i < nfds; ++i) {
        Channel* channel = static_cast<Channel*>(events[i].data.ptr);
        uint32_t revents = events[i].events;
        channel->set_revents(revents);
        active_channels.push_back(channel);
    }
}
