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
    InetAddress server_address("127.0.0.1" ,8888);
    EventLoop event_loop;
    Server server_fd(server_address, &event_loop);
    // 1. 设置连接回调
    // 注意：这里的参数类型必须和 Server.h 里的定义严格匹配
    server_fd.setConnectionCallback([](const std::shared_ptr<TcpConnection>& conn) {
        if (conn->connected()) {
            std::cout << "✅ Client connected! Name: " << conn->name() << std::endl;
        } else {
            std::cout << "❌ Client disconnected! Name: " << conn->name() << std::endl;
        }
    });

    // 2. 设置消息回调
    // 参数：Buffer* 是指针，用来读数据
    server_fd.setMessageCallback([](const std::shared_ptr<TcpConnection>& conn, Buffer* buf) {
        // 从 Buffer 里取出所有数据
        std::string msg = buf->retrieveAllAsString();
        
       // std::cout << "📨 Recv from " << conn->name() << ": " << msg << std::endl;
        
        // 把收到的数据原样发回去 (Echo)
        conn->send(msg);
    });
    server_fd.start();
    event_loop.loop();
}

//g++ main.cpp Server.cpp Channel.cpp Epoller.cpp EventLoop.cpp InetAddress.cpp Acceptor.cpp TcpConnection.cpp Buffer.cpp -o server -O2 -pthread
//taskset -c 0 ./server