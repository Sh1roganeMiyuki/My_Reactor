#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int main() {
    // 1. 创建 socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Socket 创建失败" << std::endl;
        return -1;
    }

    // 2. 配置服务器地址 (确保端口和你的 Server 一致)
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8000); 
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    // 3. 连接服务器
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "连接服务器失败，请确保 Server 已启动并在 8000 端口监听" << std::endl;
        return -1;
    }

    std::cout << "成功连接到服务器！" << std::endl;

    // 4. 发送一次数据，激活服务器的时间轮
    const char* msg = "hello server, i am alive";
    send(sock, msg, strlen(msg), 0);
    std::cout << "已发送初始心跳。现在开始沉默 3 秒，等待超时..." << std::endl;

    // 5. 关键：睡眠 35 秒 (超过服务器设定的 30 秒)
    sleep(35);
    std::cout << "Im wake up!" << std::endl;
    // 6. 尝试读取数据，看服务器是否已经断开了
    char buffer[1024] = {0};
    int n = read(sock, buffer, 1024);
    if (n == 0) {
        std::cout << "验证成功：服务器已主动断开连接（超时踢出）。" << std::endl;
    } else if (n < 0) {
        perror("read 错误");
    } else {
        std::cout << "验证失败：连接依然存活，收到数据: " << buffer << std::endl;
    }

    close(sock);
    return 0;
}