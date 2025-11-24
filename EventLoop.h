#include <iostream>
#include <vector>
#include <functional>
#include <Epoller.h>
class Epoller;
//class Channel;
class EventLoop {
public:
    EventLoop() {}
    ~EventLoop() {}
    void loop() { }

    void addChannel(Channel *channel){}
    void modChannel(Channel *channel){}
    void delChannel(Channel *channel){}
    //Epoller get_server_epoller() { return server_epoller; }
private:
    Epoller server_epoller;
    //Channel *server_channel;
};