#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "sys/types.h"
#include "InetAddress.h"
#include "Acceptor.h"
class Server {
public:
    Server(InetAddress& addr, EventLoop* loop);
    ~Server();
    //int getListenFd() const { return listen_fd_; }
    void start();
    void newConnection(int sockfd);
private:
    ///int listen_fd_;
    InetAddress addr_;
    Acceptor acceptor_;
    EventLoop* loop_;
};