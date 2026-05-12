# **My_Reactor**：高性能、可观测的 C++ 异步网络开发框架
*一款基于 Shared-Nothing 架构，从内核协议栈到应用层业务逻辑进行全链路极致压榨的 C++ 网络底座。*


![Language](https://img.shields.io/badge/language-C%2B%2B17-blue.svg)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)

## 🚀 性能基准 (Benchmarks)
*   **理论极限 (Echo Mode)**：16 核环境下单机回环突破 **1,161,412 QPS**。
*   **仿真业务负载 (Real-world Simulation)**：在注入 **4MB Cache-miss 压力与堆内存分配** 模拟逻辑下，仍能保持 **34.4w QPS**，P99 延迟线性收敛。
*   **高并发扩展性 (C10K Challenge)**：实测 4,000 个长连接并发，QPS 稳定在 **65w+**，未出现锁竞争导致的性能暴跌。
---

## 🌟 核心特性 (Key Features)
*   **高性能并发模型**：支持 **One-Loop-Per-Thread** 模型，可选 **Shared-Nothing (SO_REUSEPORT)** 零锁分发架构。
*   **低损耗 I/O 设计**：基于 `epoll` LT 模式，深度应用 **Scatter-Gather I/O (readv/writev)** 减少系统调用。
*   **鲁棒的连接管理**：基于 **双向引用计数** 的对象生命周期管理，配合**高精度时间轮 (Time-Wheel)** 实现 O(1) 复杂度的超时剔除。
*   **可观测性 (Observability)**：内置 **性能剖析探测点**，支持通过火焰图、上下文切换分析对业务瓶颈进行“开窗式”诊断。

---

## 📊 性能调优实战日志 
*   [WSL1 与原生 Linux 的系统调用翻译损耗定量分析](docs/01_wsl_bottleneck.md)
*   [Thermal Throttling (热节流) 对压测波动的影响及 BIOS 策略调优](docs/02_hardware_limits.md)
*   [Cache Miss 对高并发网关吞吐量的量化破坏力](docs/03-cpu-affinity-and-softirq.md)
*   [多线程 Acceptor 竞争与主从架构性能坍塌分析](docs/04_MasterSlave_Bottleneck_Analysis.md)
*   [Shared-Nothing 百万级回显吞吐突破](docs/05_SO_REUSEPORT_And_FurtherBottleneck.md)
*   [业务模拟 与 Cachegrind 缓存击穿推演](docs/06_Cachegrind_Profiling_L1_Pollution_And_L2_Stress_Test.md)
*   [Heap Profiling 微观内存分配清除](docs/07_Heap_Profiling_Zero_Allocation_And_Steady_State_Optimization.md)
*   [零拷贝部署 与 MSG_ZEROCOPY 幻象](docs/08_Perf_Trace_Zero_Copy_Profiling_And_Virtual_Network_Bottlenecks.md)
*   [零拷贝部署 与 MSG_ZEROCOPY 幻象](docs/09_network_kcp_cross_platform_alignment.md)

---

## 📂 项目结构
```text
.
├── include/      # 核心库头文件 (.h)
├── src/          # 核心库实现 (.cpp)
├── example/      # 示例程序 (如 http_server 入口)
├── test/         # 单元测试与压测客户端
├── docs/         # 调优日志、火焰图 (SVG)
├── scripts/      # 辅助脚本 (Lua/Shell)
└── CMakeLists.txt
```

---

## 🛠️ 快速开始

### 1. 环境准备
- Linux 系统 (支持 epoll 与 SO_REUSEPORT)
- CMake 3.0+
- G++ 7.0+ (支持 C++17)

### 2. 编译构建
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 3. 运行压测服务
```bash
# 监听 8000 端口，开启 8 个工作线程
./http_server 8000 8
```

> ## 💻 快速上手 (API Preview)
> ```cpp
> MyReactor::HttpServer server;
> server.setMessageCallback([](const std::shared_ptr<TcpConnection>& conn, Buffer* buf) {
>     // 极简异步回调接口
>     std::string data = buf->retrieveAllAsString();
>     if (data.find("\r\n\r\n") != std::string::npos) {
>         conn->send("HTTP/1.1 200 OK\r\nContent-Length: 11\r\n\r\nHello World");
>     }
> });
> server.start(8000, 8); // 监听8000端口，启动8个工作线程
> ```
