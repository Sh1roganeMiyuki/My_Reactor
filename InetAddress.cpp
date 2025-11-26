#include "InetAddress.h"
#include <cstring>
InetAddress::InetAddress(const std::string& ip, uint16_t port) {
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    //addr_.sin_addr.s_addr = INADDR_ANY;
    inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);
}