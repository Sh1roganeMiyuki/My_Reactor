#pragma once

#include <sys/epoll.h>
#include <unistd.h>
#include "Channel.h"
#include <vector>
#include <map>
class Channel;
class Epoller {
public:
    Epoller();
    ~Epoller();

    void add_channel(Channel *channel);
    void mod_channel(Channel *channel);
    void del_channel(Channel *channel);
    
    void wait(int timeout_ms, std::vector<Channel*>& active_channels);
private:
    int epoll_fd_;
    // uint32_t events_;
    // uint32_t revents_;
    std::map<int, Channel*> channel_map_;
};