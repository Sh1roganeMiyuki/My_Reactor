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
    //Channel server_channel(server_fd.getListenFd(), &event_loop);
    // std::thread logThread([]() {
    //     while (true) {
    //         // 1. 睡 1 秒
    //         std::this_thread::sleep_for(std::chrono::seconds(1));

    //         // 2. 原子操作：获取当前计数值，并重置为 0
    //         // exchange 会返回旧值，并将 g_totalRequestCount 设为 0
    //         int64_t count = g_totalRequestCount.exchange(0);

    //         // 3. 打印结果
    //         // 使用 printf 比 cout 稍微快一点点，而且不会被多线程打乱格式
    //         printf("Current QPS: %ld\n", count);
    //     }
    // });
    // 分离线程，让它在后台跑，主线程继续往下走
    //logThread.detach(); 
    //
    event_loop.loop();
}

//g++ main.cpp Server.cpp Channel.cpp Epoller.cpp EventLoop.cpp InetAddress.cpp Acceptor.cpp -o server -O2 -pthread
//taskset -c 0 ./server