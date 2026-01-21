#include "Server.h"
#include "EventLoop.h"
#include "TcpConnection.h"
#include <iostream>
#include <string>
#include <any> // for std::any_cast

int main() {
    EventLoop loop;
    InetAddress addr("0.0.0.0", 8000);
    Server server(addr, &loop);

    server.setConnectionCallback([](const std::shared_ptr<TcpConnection>& conn) {
        // if (conn->connected()) {
        //     std::cout << "Client " << conn->name() << " connected" << std::endl;
        // } else {
        //     std::cout << "Client " << conn->name() << " disconnected" << std::endl;
        // }
    });

    server.setMessageCallback([](const std::shared_ptr<TcpConnection>& conn, Buffer* buf) {
        
        if (!conn->hasContext()) {
            conn->setContext(std::string());
        }

        std::string* requestData = std::any_cast<std::string>(conn->getMutableContext());

        requestData->append(buf->retrieveAllAsString());

        size_t eof = requestData->find("\r\n\r\n");
        
        if (eof != std::string::npos) {

            //std::cout << "✅ Received Full HTTP Request:\n" << *requestData << std::endl;

            std::string response = 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 11\r\n"
                "Connection: Keep-Alive\r\n" // 保持长连接
                "\r\n"
                "Hello World";
            
            conn->send(response);

            requestData->clear();
        } else {
        }
    });

    //std::cout << "🚀 HTTP Server running on 8080..." << std::endl;
    server.start();
    loop.loop();

    return 0;
}
//g++ main.cpp Server.cpp TcpConnection.cpp EventLoop.cpp Epoller.cpp Channel.cpp Buffer.cpp InetAddress.cpp Acceptor.cpp -o http_server -O2 -pthread -std=c++17
//taskset -c 0 ./http_server


//taskset -c 1-3 wrk -t4 -c500 -d30s --latency http://127.0.0.1:8000/        9.5w指令    