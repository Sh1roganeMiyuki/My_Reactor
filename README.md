# My_Reactor: High-Performance C++ Network Library

![Language](https://img.shields.io/badge/language-C%2B%2B17-blue.svg)
![Platform](https://img.shields.io/badge/platform-Linux-lightgerm.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)

## 🚀 核心战果
在 **16 核云服务器** 环境下，通过 **Shared-Nothing 架构迭代** 与 **系统级调优**，本项目成功实现：
- **单机吞吐量突破 1,161,412 QPS**（基于裸 TCP Echo/Dummy HTTP 压测）。
- **P99 尾部延迟稳定在 1.24ms**。
- **压榨率达环境限制 90% 以上**（通过 iperf3 建立 PPS 标尺定性分析）。

---

## 🌟 核心特性
*   **架构演进**：支持从 **Main-Sub Reactor** 到 **Shared-Nothing (SO_REUSEPORT)** 架构的无缝切换，彻底消除全局 Acceptor 锁竞争。
*   **I/O 模型**：基于 **epoll 水平触发 (LT)** 模式，配合非阻塞 I/O 与状态机逻辑，支撑海量并发连接。
*   **系统级优化**：
    *   应用 **矢量化 I/O (readv/writev)** 合并系统调用，显著降低内核态切换损耗。
    *   利用 **CPU 亲和性 (taskset 绑核)** 治理超线程干扰，优化 Cache 命中率。
*   **高性能定时器**：基于 `std::set` 与红黑树实现的延迟关闭机制，支持 $O(\log N)$ 的随机节点剔除，有效防御僵尸连接攻击。

---

## 📊 性能调优实验 (核心深度)
本项目不仅是代码实现，更包含完整的性能排查与溯源记录（见 `docs/` 目录）：
*   [Log-01: 系统调用瓶颈分析](docs/01_wsl_bottleneck.md) - 定位上下文切换对吞吐量的影响。
*   [Log-02: 硬件环境干扰因素](docs/02_hardware_limits.md) - 探索虚拟化环境下的 PPS 上限。
*   [Log-03: 线程绑定与 Cache 污染溯源](docs/03-cpu-affinity-and-softirq.md) - 解决跨核调度导致的延迟抖动。
*   [Log-04: 主从架构性能坍塌](docs/04_MasterSlave_Bottleneck_Analysis.md) - 分析高负载下 Acceptor 分发瓶颈。
*   [Log-05: Shared-Nothing 百万级吞吐突破](docs/05_SO_REUSEPORT_And_FurtherBottleneck.md) - 压榨 Linux 内核协议栈的最后 10% 性能。

---

## 📂 项目结构
```text
.
├── include/      # 核心库头文件 (.h)
├── src/          # 核心库实现 (.cpp)
├── example/      # 示例程序 (如 http_server)
├── test/         # 单元测试与压测客户端
├── docs/         # 性能分析日志、火焰图 (SVG)
├── scripts/      # 辅助压测脚本 (Lua)
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

---

## 📈 性能观测工具
项目开发中通过以下工具链拒绝“黑盒优化”：
- **perf / FlameGraph**：定性指令耗时。
- **htop / pidstat**：监控上下文切换与内核软中断。
- **iperf3**：建立网络限制标尺。
