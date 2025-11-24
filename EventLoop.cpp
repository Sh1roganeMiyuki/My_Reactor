#include "EventLoop.h"
//#include "Channel.h"  
#include "Epoller.h"
class Epoller;
class Channel;
EventLoop::EventLoop() {
}
EventLoop::~EventLoop() {
}
void EventLoop::loop() {
    while (true) {
        std::vector<Channel *> active_channels;
        active_channels.clear();
        server_epoller.wait(0, active_channels);
        for (auto channel : active_channels) {
            channel->handle_event();
        }
    }
}
void EventLoop::addChannel(Channel *channel){
    server_epoller.add_channel(channel);
}
void EventLoop::modChannel(Channel *channel){
    server_epoller.mod_channel(channel);
}
void EventLoop::delChannel(Channel *channel){
    server_epoller.del_channel(channel);
}