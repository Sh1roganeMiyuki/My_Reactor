#include "Server.h"
#include <iostream>
Server::Server(InetAddress& addr, EventLoop* loop) : addr_(addr), acceptor_(loop, addr_), loop_(loop) 
{
    acceptor_.newConnectionCallback(
        [this](int sockfd) { this->newConnection(sockfd); }
    );

}
void Server::start() {
    acceptor_.listen();
}
Server::~Server() {
}
void Server::newConnection(int sockfd) {
    std::cout << "Server: New connection with fd : " << sockfd << std::endl;
}