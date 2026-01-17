#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <cstring>
#include <iostream>
#include <atomic>
#include <chrono>

// 全局原子计数器：客户端视角的 QPS
std::atomic<int64_t> g_totalRequests(0);

// 统计连接失败的次数
std::atomic<int> g_connectFailures(0);

void threadFunc(int id, const char* ip, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serverAddr.sin_addr);

    // 尝试连接
    if (connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        // 记录失败，但不刷屏打印，防止终端卡死
        g_connectFailures++;
        close(sockfd);
        return; 
    }

    const char* msg = "Hello, I am C++ Client!";
    size_t len = strlen(msg);
    char buf[4096];
    
    while (true) {
        // 1. 发送数据
        // 使用 send 并加 MSG_NOSIGNAL，防止服务器断开导致 SIGPIPE 崩溃
        ssize_t nWritten = send(sockfd, msg, len, MSG_NOSIGNAL);
        if (nWritten < 0) break; 
        
        // 2. 接收回音 (Echo)
        ssize_t nRead = read(sockfd, buf, sizeof(buf));
        if (nRead <= 0) break; // 0 表示服务器关闭，<0 表示错误
        
        // 3. 完整的一发一收结束，计数器 +1
        g_totalRequests++;
    }
    
    close(sockfd);
}

int main() {
    // 配置目标：确保这里和 Server 的监听地址一致！
    // 如果 Server 监听的是 0.0.0.0 或 127.0.0.1，这里用 127.0.0.1 没问题
    const char* SERVER_IP = "127.0.0.1";
    const int SERVER_PORT = 8888; // 你确认是 8080

    int threadCount = 200; // 并发线程数
    std::cout << "🚀 Client launching with " << threadCount << " threads -> " 
              << SERVER_IP << ":" << SERVER_PORT << std::endl;

    // 1. 启动监控线程
    std::thread monitorThread([]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // 获取并清零 QPS
            int64_t qps = g_totalRequests.exchange(0);
            int failures = g_connectFailures.load();
            
            if (failures > 0) {
                 printf("⚠️ Connect Failures: %d | Current QPS: %ld\n", failures, qps);
            } else {
                 printf("✅ Client Side QPS: %ld\n", qps);
            }
        }
    });
    monitorThread.detach();

    // 2. 启动压测线程
    std::vector<std::thread> threads;
    for(int i = 0; i < threadCount; ++i) {
        threads.emplace_back(threadFunc, i, SERVER_IP, SERVER_PORT);
        // 稍微错峰启动，避免瞬间把 backlog 塞满导致连接失败
        // std::this_thread::sleep_for(std::chrono::milliseconds(1)); 
    }

    for(auto& t : threads) t.join();
    return 0;
}
//g++ client.cpp -o client -O2 -pthread
//taskset -c 1-7 ./client