#include <string>
#include <cstdint>
#include <netinet/in.h>
#include <arpa/inet.h>

class InetAddress {
public:
    InetAddress(const std::string& ip, uint16_t port);
    ~InetAddress() = default;
    int getAddrLen() const { return sizeof(addr_); }
    sockaddr* getSockAddr() { return (sockaddr*)&addr_; }

private:
    sockaddr_in addr_;
};