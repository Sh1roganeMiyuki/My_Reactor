#include "Acceptor.h"
#include "Channel.h"
#include "EventLoop.h"
#include <netinet/in.h>
#include <sys/socket.h>

class Channel;

Acceptor::Acceptor(EventLoop* loop, InetAddress& listenAddr) : 
                    loop_(loop), listenAddr_(listenAddr), 
                    listenFd_(::socket(AF_INET, SOCK_STREAM, 0)) , acceptChannel_(listenFd_, loop_){ 
    if(listenFd_ < 0){
        std::cerr << "Socket creation failed" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    int opt = 1;
    setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if(bind(listenFd_, listenAddr_.getSockAddr(), listenAddr_.getAddrLen()) < 0){
        std::cerr << "Bind failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "Server started" << std::endl;
    acceptChannel_.setReadCallback([this]() { this->handleRead(); });
}

Acceptor::~Acceptor() {
    
    ::close(listenFd_);
}
void Acceptor::listen() { 
    acceptChannel_.enableReading(); 
    if(::listen(listenFd_, SOMAXCONN) < 0){
        std::cerr << "Listen failed" << std::endl;
        exit(EXIT_FAILURE);
    }
}
void Acceptor::handleRead() {
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(listenFd_, (sockaddr*)&client_addr, &client_len);

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

