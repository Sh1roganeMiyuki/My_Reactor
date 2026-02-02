#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <asm-generic/socket.h>

int main() {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    // ... error checks ...

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(8000);
    bind(listen_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    listen(listen_fd, SOMAXCONN);

    char response[] = "HTTP/1.1 200 OK\r\nContent-Length: 11\r\n\r\nHello World";
    char buffer[1024];

    while (1) {
        int conn_fd = accept(listen_fd, NULL, NULL);
        if (conn_fd > 0) {
            if (fork() == 0) { // 每个连接一个进程
                close(listen_fd);
                while (read(conn_fd, buffer, 1024) > 0) {
                    write(conn_fd, response, sizeof(response) - 1);
                }
                close(conn_fd);
                exit(0);
            }
            close(conn_fd);
        }
    }
    return 0;
}