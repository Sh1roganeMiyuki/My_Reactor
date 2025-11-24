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
class Channel;
class Epoller;
int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        std::cout << "Failed to create socket." << std::endl;
        return 1;
    }
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8888);
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0)
    {
        std::cout << "Failed to parse IP address." << std::endl;
        return 1;
    }
    if (bind(server_fd, (sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        std::cout << "Failed to bind socket." << std::endl;
        return 1;
    }
    if (listen(server_fd, 10) == -1)
    {
        std::cout << "Failed to listen on socket." << std::endl;
        return 1;
    }
    //Epoller server_epoller;
    
    EventLoop event_loop;
    //Epoller server_epoller(event_loop.get_server_epoller());
    Channel server_channel(server_fd, &event_loop);
    //server_channel.set_revents(EPOLLIN);
    server_channel.setReadCallback([server_fd, &event_loop]()
                                   {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        Channel* client_channel = new Channel(client_fd, &event_loop);
        //client_channel->set_revents(EPOLLIN);
        if (client_fd != -1) {
            std::cout << "New connection accepted." << std::endl;
            client_channel->setReadCallback([&event_loop, client_channel, client_fd]() {
                char buffer[1024];
                ssize_t n = read(client_fd, buffer, sizeof(buffer));
                if (n > 0) {
                    std::cout << "Received data: " << std::string(buffer, n) << std::endl;
                } else if (n == 0) {
                    std::cout << "Client disconnected." << std::endl;
                    event_loop.delChannel(client_channel);
                    delete client_channel;
                } else {
                    std::cout << "Read error." << std::endl;
                }
            });
        }
        client_channel->enableReading(); 
    });
    server_channel.enableReading();

    event_loop.loop();
    // while (true)
    // {
    //     std::vector<Channel *> active_channels;
    //     active_channels.clear();
    //     server_epoller.wait(1000, active_channels);
    //     for (auto channel : active_channels)
    //     {
    //         channel->handle_event();
    //     }
    // }
}