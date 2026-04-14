#include "Server.h"
#include "EventLoop.h"
#include "TcpConnection.h"
#include <iostream>
#include <string>
#include <any> // for std::any_cast
#include <signal.h>

#include <atomic>
#include <cstdio>

// std::atomic<size_t> g_alloc_count{0};

// void* operator new(size_t size) {
//     g_alloc_count.fetch_add(1, std::memory_order_relaxed);
//     return malloc(size);
// }


int main() {
    // 忽略 SIGPIPE 信号，防止服务器意外退出
    signal(SIGPIPE, SIG_IGN); 

    EventLoop loop;
    InetAddress addr("0.0.0.0", 8000);
    Server server(addr, &loop);


    server.setThreadNum(8); 
    std::cout << "🚀 HTTP Server running on 8000..." << std::endl;

    server.setConnectionCallback([](const std::shared_ptr<TcpConnection>& conn) {
        // if (conn->connected()) {
        //     std::cout << "Client " << conn->name() << " connected" << std::endl;
        // } else {
        //     std::cout << "Client " << conn->name() << " disconnected" << std::endl;
        // }
    });

    // server.setMessageCallback([](const std::shared_ptr<TcpConnection>& conn, Buffer* buf) {
    //     if (!conn->hasContext()) {
    //         conn->setContext(std::string());
    //     }

    //     std::string* requestData = std::any_cast<std::string>(conn->getMutableContext());

    //     // 读取数据到 Context
    //     requestData->append(buf->retrieveAllAsString());

    //     // 检查报文定界
    //     size_t eof = requestData->find("\r\n\r\n");
    //     if (eof != std::string::npos) {
    //         // 模拟业务逻辑

    //         // 建立 4MB 的向量，撑爆 L2 Cache 
    //         // 强迫 CPU 去 L3 甚至主存取数据，模拟真实 Session 查询的内存延迟
    //         static const std::vector<int> big_session_table = [](){
    //             std::vector<int> v(1024 * 1024); // 4MB 空间
    //             for(size_t i = 0; i < v.size(); ++i) v[i] = static_cast<int>(i ^ 0xDEADBEEF);
    //             return v;
    //         }();

    //         // 模拟深层协议解析
    //         // 增加查找深度，模拟解析复杂的 HTTP Header
    //         size_t ua_pos = requestData->find("User-Agent:");
    //         size_t cookie_pos = requestData->find("Cookie:");
    //         size_t auth_pos = requestData->find("Authorization:"); 

    //         // 强迫堆内存分配
    //         // 我们拼接一个长字符串，强迫框架调用 malloc/free，模拟真实的业务对象构建损耗。
    //         std::string business_trace_id = "TRACE_ID_LONG_PREFIX_FOR_ALLOCATION_TEST_"; // > 40 bytes
    //         business_trace_id.reserve(128); // 明确强制堆分配
    //         business_trace_id += "NODE_001_";
    //         if (ua_pos != std::string::npos) {
    //             // 提取一段足够长的内容，确保不触发 SSO
    //             business_trace_id += requestData->substr(ua_pos, 32); 
    //         }

    //         // 破坏预取器的随机内存访问 
    //         // 使用一个简单的伪随机算法（线性同余）来产生无法被 CPU 预取器预测的访问索引
    //         int lookup_sum = 0;
    //         uint32_t seed = static_cast<uint32_t>(requestData->size() + reinterpret_cast<uintptr_t>(conn.get()));
    //         for (int i = 0; i < 15; ++i) {
    //             seed = (seed * 1103515245 + 12345) & 0x7fffffff;
    //             lookup_sum += big_session_table[seed % big_session_table.size()];
    //         }

    //         // 模拟中度 CPU 计算
    //         // 2000 次循环约等于 10-20μs 的计算负载，模拟 Protobuf 解码或业务规则校验
    //         uint64_t hash_val = 0xCBF29CE484222325ULL;
    //         for (int i = 0; i < 2000; ++i) {
    //             hash_val ^= (uint64_t)lookup_sum + i;
    //             hash_val *= 0x100000001B3ULL; // FNV-1a 风格计算
    //         }

    //         // 强迫编译器保留结果，防止被优化掉
    //         volatile uint64_t sink = hash_val + business_trace_id.size();
    //         (void)sink;


    //         // 3. 构建标准响应
    //         static const std::string body = [](){
    //             std::string s = "Hello World";
    //             s.reserve(11000);
    //             for(size_t i = 0; i < 1000; ++i) s += "0123456789";
    //             return s;
    //         }();

    //         // 使用 reserve 避免 response 拼接时的多次分配
    //         std::string response;
    //         response.reserve(body.size() + 200); 
    //         response += "HTTP/1.1 200 OK\r\n";
    //         response += "Content-Type: text/plain\r\n";
    //         response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    //         response += "Connection: Keep-Alive\r\n\r\n";
    //         response += body;

    //         conn->send(response);

    //         // 4. 重置 Context (Pipeline 支持)
    //         requestData->clear();
    //     }
    // });

    server.setMessageCallback([](const std::shared_ptr<TcpConnection>& conn, Buffer* buf) {
        // 1. 彻底清空 Buffer，不转换成 string，避免产生临时对象
        buf->retrieveAll(); 

        // 2. 发送一个预先准备好的静态字符串
        // 使用静态变量，整个运行期间只分配一次内存
        static const std::string response = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 11\r\n"
            "Connection: Keep-Alive\r\n"
            "\r\n"
            "Hello World";

        conn->send(response);
    });
    // server.setMessageCallback([](const std::shared_ptr<TcpConnection>& conn, Buffer* buf) {
    //     // 1. 仅仅消耗掉数据，不转 string，不回包
    //     buf->retrieveAll(); 
    //     // 2. 什么都不做，连 conn->send 都不调
    // });

    // std::thread([](){
    //     while(true) {
    //         printf("Current Total Allocations: %zu\n", g_alloc_count.load());
    //         std::this_thread::sleep_for(std::chrono::seconds(1));
    //     }
    // }).detach();
    //std::cout << "🚀 HTTP Server running on 8080..." << std::endl;
    server.start();
    loop.loop();

    return 0;
}
//g++ -g main.cpp Server.cpp TcpConnection.cpp EventLoop.cpp Epoller.cpp Channel.cpp Buffer.cpp InetAddress.cpp Acceptor.cpp TimerQueue.cpp EventLoopThread.cpp EventLoopThreadPool.cpp -o http_server -O2 -pthread -std=c++17
//taskset -c 0,2,4,6 ./http_server


//taskset -c 1-3 wrk -t4 -c500 -d30s --latency http://127.0.0.1:8000/    
//taskset -c 1-3 wrk -t4 -c500 -d30s -H "Connection: close" --latency http://127.0.0.1:8000/
//pidof http_server
//pidstat -w -t -p 1499 1
//sudo perf record -F 99 -g -p 1181 -- sleep 30
//pidstat -w -t -C wrk 1

// echo 3 | sudo tee /proc/sys/vm/drop_caches

//cd ~/CppNetLearn/My4.0Rewrite