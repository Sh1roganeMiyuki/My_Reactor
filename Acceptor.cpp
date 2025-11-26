#include "Acceptor.h"
#include "Channel.h"
#include "EventLoop.h"
#include <netinet/in.h>
#include <sys/socket.h>

//class Channel;

Acceptor::Acceptor(EventLoop* loop, int listenFd) : loop_(loop), listenFd_(listenFd) ,acceptChannel_(listenFd, loop){
    acceptChannel_.setReadCallback([this]() { this->handleRead(); });
}

Acceptor::~Acceptor() {}

void Acceptor::handleRead() {
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(listenFd_, (sockaddr*)&client_addr, &client_len);
    //Channel* client_channel = new Channel(client_fd, loop_);
    if (client_fd != -1) {
        std::cout << "New connection accepted." << std::endl;
        if(newConnectionCallback_){
            newConnectionCallback_(client_fd);
        }else{
            ::close(client_fd);
        }
    }else{
        std::cout << "Accept error." << std::endl;
    }
}

