# My_Reactor: High-Performance C++ Network Library

![Language](https://img.shields.io/badge/language-C%2B%2B11-blue.svg)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)

## 🚀 核心战果
在 16 核云服务器环境下，单机回环测试突破 **1,161,412 QPS**，P99 延迟稳定在 **1.24ms**。

## 🌟 核心特性
- **架构演进**：支持从 Main-Sub Reactor 到 **Shared-Nothing** (SO_REUSEPORT) 架构的切换，消灭全局锁竞争。
- **I/O 模型**：基于 **epoll 水平触发 (LT)** 模式，配合非阻塞 I/O 和状态机循环。
- **系统优化**：应用 **矢量化 I/O (readv/writev)** 减少系统调用；利用 **CPU 亲和性 (taskset)** 治理调度抖动。
- **高性能定时器**：基于高效数据结构实现的连接超时剔除机制。

## 📊 性能调优实验
本项目不仅是代码实现，更包含完整的排查与调优记录：
- [Log-01: 系统调用瓶颈分析](./docs/01_wsl_bottleneck.md)
- [Log-02: 硬件环境干扰因素](./docs/02_hardware_limits.md)
- [Log-03: 线程绑定与 Cache 污染溯源](./docs/03-cpu-affinity-and-softirq.md)
- [Log-04: 主从架构性能坍塌](./docs/04_MasterSlave_Bottleneck_Analysis.md)
- [Log-05: Shared-Nothing 百万级吞吐突破](./docs/05_SO_REUSEPORT_And_FurtherBottleneck.md)

## 🛠️ 快速开始
```bash
mkdir build && cd build
cmake ..
make -j16
./http_server 8000 8
