#include "sys/socket.h"
#include "netinet/in.h"
#include "map"
#include "vector"
#include "iostream"
#include "string"
#include "stdlib.h"
#include <cstring>
#include <arpa/inet.h>
#include "Epoller.h"
#include "Channel.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Server.h"
class Channel;
class Epoller;
int main()
{
    InetAddress server_address("127.0.0.1" ,8080);
    EventLoop event_loop;
    Server server_fd(server_address, &event_loop);
    server_fd.start();
   // Channel server_channel(server_fd.getListenFd(), &event_loop);
    event_loop.loop();
}