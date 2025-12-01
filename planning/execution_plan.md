# CXL内存仿真平台执行计划

## 📋 项目概述

**项目名称**: CXLMemSim - CXL内存仿真平台
**目标**: 构建一个兼顾仿真精度、执行速度与拓扑灵活性的软件定义CXL内存仿真平台
**技术路线**: 基于Epoch的用户态执行驱动(Execution-Driven)架构
**核心特性**:
- 利用Intel PEBS/LBR硬件性能计数器进行精确采样
- 使用eBPF进行内核级内存分配追踪
- 模拟CXL.io、CXL.cache、CXL.mem三层协议栈
- 支持复杂拓扑结构(交换机、Fabric)的延迟与带宽建模

---

## 🎯 四阶段开发计划

### 第一阶段: 基础设施搭建与原型验证 (2025.11 - 2026.01)

#### 阶段目标
打通从硬件采样到用户态分析的数据通路，建立可工作的原型系统

#### 关键任务

**1.1 环境准备与配置** ✅ **[第一步 - 当前任务]**
- [ ] 配置Linux内核参数
  - 设置 `perf_event_paranoid` 允许用户态访问PEBS/LBR
  - 验证CPU支持的PMU事件(LLC_MISS, PEBS, LBR)
  - 配置大页内存支持(Hugepage)
- [ ] 安装依赖工具链
  - 安装BCC/libbpf工具链
  - 安装perf工具并验证功能
  - 配置编译环境(CMake, C++17)
- [ ] 创建项目目录结构
  ```
  CXLMemSim/
  ├── backend/           # 后端核心引擎
  │   ├── ebpf/         # eBPF追踪程序
  │   ├── include/      # 头文件
  │   └── src/          # 源代码
  │       ├── tracer/   # 追踪器模块
  │       ├── analyzer/ # 时序分析器
  │       └── injector/ # 延迟注入器
  ├── frontend/          # 前端配置层
  │   └── components/   # 拓扑定义、内存映射管理
  ├── common/           # 公共数据结构
  ├── tests/            # 测试用例
  └── docs/             # 文档
  ```

**交付物**:
- 环境配置验证报告
- 可编译的项目框架
- 基础的CMakeLists.txt

---

**1.2 Tracer模块开发**
- [ ] 封装perf_event_open系统调用
  - 实现PerfEventConfig类管理PMU事件配置
  - 支持PEBS采样(MEM_LOAD_RETIRED.L3_MISS)
  - 支持LBR记录采集
- [ ] 实现Ring Buffer数据读取
  - 映射内核perf ring buffer到用户空间
  - 解析perf_event_header格式
  - 提取数据线性地址(Data Linear Address)
- [ ] 实现Epoch同步机制
  - 定时批量读取采样数据(默认10ms epoch)
  - 提取TSC时间戳重建时间序列
- [ ] 基础过滤逻辑
  - 过滤非目标进程的事件
  - 基于地址范围过滤本地DRAM访问

**交付物**:
- Tracer类实现(tracer.h/cpp)
- 单元测试: 能打印目标程序的LLC Miss事件

---

**1.3 Injector模块开发**
- [ ] 实现线程控制机制
  - 利用Linux信号(SIGSTOP/SIGCONT)暂停/恢复线程
  - 或使用ptrace系统调用附加到目标进程
- [ ] 实现混合休眠策略
  - 长延迟(>50us): 使用nanosleep
  - 短延迟(<50us): 使用自旋循环(rdtsc busy-wait)
- [ ] 验证延迟注入精度
  - 测量实际注入延迟与目标延迟的偏差
  - 校准系统调用开销

**交付物**:
- Injector类实现(injector.h/cpp)
- Demo: 能够对简单程序(如死循环)注入固定延迟

---

**1.4 eBPF内存追踪**
- [ ] 编写eBPF探针程序
  - 挂载到sys_mmap/sys_brk跟踪点
  - 挂载到page_fault事件
  - 捕获虚拟地址范围并传递到用户空间
- [ ] 实现用户态eBPF加载器
  - 使用libbpf加载BPF程序
  - 读取perf event数据
  - 维护全局虚拟地址映射表

**交付物**:
- eBPF程序(memory_tracker.bpf.c)
- 加载器(bpf_loader.cpp)
- 测试: 追踪目标程序的内存分配并打印

---

**阶段一验证**:
- [ ] 集成测试: 运行STREAM基准测试
  - Tracer能捕获到内存访问
  - eBPF能追踪到分配的地址范围
  - Injector能暂停/恢复STREAM线程
- [ ] 输出: 内存访问热力图工具

---

### 第二阶段: CXL核心模型实现 (2026.02 - 2026.03)

#### 阶段目标
实现延迟与带宽的数学模型，完成基本的CXL仿真逻辑

#### 关键任务

**2.1 配置文件系统**
- [ ] 定义JSON配置格式
  ```json
  {
    "topology": {
      "root_complex": {
        "id": "RC0",
        "local_dram_size_gb": 64
      },
      "switches": [
        {
          "id": "SW0",
          "latency_ns": 40,
          "ports": 8
        }
      ],
      "cxl_devices": [
        {
          "id": "CXL0",
          "type": "Type3",
          "capacity_gb": 128,
          "link_gen": "Gen5",
          "link_width": "x16",
          "bandwidth_gbps": 64,
          "base_latency_ns": 170
        }
      ],
      "connections": [
        {"from": "RC0", "to": "SW0", "link": "Gen5x16"},
        {"from": "SW0", "to": "CXL0", "link": "Gen5x16"}
      ]
    },
    "memory_policy": {
      "type": "interleave",
      "local_first_gb": 32
    }
  }
  ```
- [ ] 实现配置解析器
  - 使用nlohmann/json库
  - 验证配置有效性
- [ ] 构建拓扑图数据结构
  - 使用有向加权图表示拓扑
  - 节点: Host, Switch, Endpoint
  - 边: Link {bandwidth, latency, current_load}

**交付物**:
- TopologyBuilder类
- 示例配置文件(configs/default_topology.json)

---

**2.2 延迟计算模型**
- [ ] 实现基础延迟公式
  ```cpp
  T_total = T_base_latency + T_switch + T_fabric
  ```
- [ ] 实现ROB掩盖效应(MLP修正)
  - 利用LBR分析指令窗口
  - 检测并行Miss(时间窗口<100 cycles)
  - 计算MLP因子: `T_effective = T_total / MLP_factor`
- [ ] 路径计算
  - Dijkstra算法找最短路径
  - 累加路径上所有Switch延迟

**交付物**:
- LatencyModel类(latency_model.h/cpp)
- 单元测试: 验证不同拓扑下的延迟计算

---

**2.3 带宽与拥塞模型**
- [ ] 实现时间序列流量统计
  - 将Epoch细分为微秒级micro-slice
  - 在每个micro-slice累加链路流量
- [ ] 实现拥塞判决
  ```cpp
  if (B_total > Link_BW * Slice_Time) {
    T_congestion = (B_total - Link_BW * Slice_Time) / Link_BW;
  }
  ```
- [ ] 多级链路拥塞处理
  - 遍历路径上所有链路
  - 累加或取最大拥塞延迟

**交付物**:
- BandwidthModel类(bandwidth_model.h/cpp)
- 拥塞模拟测试

---

**2.4 时序分析器集成**
- [ ] 实现TimingAnalyzer主控类
  - 接收Tracer的采样数据
  - 查询eBPF映射表确定访问的CXL设备
  - 调用LatencyModel和BandwidthModel计算延迟
  - 调用Injector注入延迟
- [ ] Epoch驱动循环
  ```cpp
  while (running) {
    // 1. 等待Epoch结束
    sleep_until_epoch_end();

    // 2. 收集采样数据
    auto samples = tracer.collect_samples();

    // 3. 分析延迟
    for (auto& sample : samples) {
      auto device = mapper.lookup(sample.addr);
      auto delay = analyzer.calculate_delay(device, sample);
      injector.inject(sample.tid, delay);
    }
  }
  ```

**交付物**:
- TimingAnalyzer类
- CXLMemSim Alpha版本可执行文件

---

**阶段二验证**:
- [ ] 运行STREAM基准测试
  - 配置: 50% DRAM + 50% 模拟CXL(Gen5 x8)
  - 预期: 带宽应受限于32GB/s
  - 预期: 延迟增加约170ns
- [ ] 运行Intel MLC
  - 验证带宽-线程数曲线在6-8线程饱和
- [ ] 输出: 性能对比报告(原生 vs 仿真)

---

### 第三阶段: 高级特性与协议仿真 (2026.03 - 2026.04)

#### 阶段目标
实现CXL.cache一致性模拟、内存分层策略、多线程优化

#### 关键任务

**3.1 CXL.cache一致性模拟**
- [ ] 实现共享页表追踪
  - 利用/proc/[pid]/pagemap读取页表状态
  - 标记共享页(多个线程访问)
- [ ] 模拟Back-Invalidate延迟
  - 检测对共享页的写操作
  - 注入额外的Snoop延迟(典型30-50ns)
- [ ] 模拟Snoop过滤器
  - 维护目录结构追踪缓存行状态
  - 模拟RdOwn/RFO操作

**交付物**:
- CoherenceModel类
- 多线程共享内存测试

---

**3.2 内存分层与迁移策略**
- [ ] 实现页面热度统计
  - 统计每页的访问频率
  - 维护热页/冷页列表
- [ ] 实现页面迁移模拟
  - 模拟热页从CXL迁移到本地DRAM
  - 注入迁移开销(数据拷贝延迟)
- [ ] 支持多种分配策略
  - First-touch: 线程首次访问时分配到本地
  - Interleave: 地址交错分配
  - Tiering: 热数据本地，冷数据CXL

**交付物**:
- MemoryPolicy类
- 策略对比实验

---

**3.3 协议开销建模**
- [ ] Flit打包效率
  - CXL 2.0: 68B Flit
  - CXL 3.0: 256B Flit, 有效载荷93%
  - 调整Link_BW = Physical_BW * efficiency
- [ ] CXL事务开销
  - 模拟TLP头部开销
  - 模拟CRC/FEC校验延迟

**交付物**:
- 更新的BandwidthModel
- 协议开销验证测试

---

**3.4 多线程支持优化**
- [ ] 实现无锁数据结构
  - 使用lock-free queue传递采样数据
  - 减少锁竞争开销
- [ ] Epoch同步优化
  - Per-thread epoch机制
  - 避免全局barrier
- [ ] 注入器并发控制
  - 确保不重复注入延迟
  - 处理线程调度导致的时间偏差

**交付物**:
- 多线程性能测试(32+ threads)
- 并发正确性验证

---

**阶段三验证**:
- [ ] 运行Redis基准测试
  - 配置: KV数据存储在CXL内存
  - 对比不同策略(Tiering vs Interleave)
- [ ] 运行图计算(如BFS)
  - 测试随机访问模式下的拥塞效应
- [ ] 输出: 策略优化建议报告

---

### 第四阶段: 验证、测试与文档 (2026.04 - 2026.05)

#### 阶段目标
全面验证精度，完成真实工作负载测试，撰写毕业设计文档

#### 关键任务

**4.1 精度验证**
- [ ] Intel MLC校准
  - 空载延迟测试(目标: ±10ns误差)
  - 满载带宽测试(目标: ±5%误差)
  - 生成带宽-延迟曲线与真实硬件对比
- [ ] 拥塞测试
  - 多核竞争同一CXL链路
  - 验证延迟抖动符合排队理论

**交付物**:
- 精度验证报告
- 误差分析文档

---

**4.2 真实工作负载测试**

**测试1: LLM推理(Llama.cpp)**
- [ ] 配置: 将KV Cache放置在CXL内存
- [ ] 测量Prefill vs Decode阶段性能
- [ ] 预期: Prefill延迟敏感(↓20%), Decode带宽敏感(↓30%)

**测试2: 内存数据库(Redis)**
- [ ] 配置: 数据集部分在CXL
- [ ] 测试GET/SET操作延迟分布
- [ ] 对比不同内存分层策略

**测试3: HPC工作负载(NAS-PB或Graph500)**
- [ ] 测试交错访问策略
- [ ] 对比对象级交错 vs 页级交错

**交付物**:
- 应用性能分析报告
- 最佳实践指南

---

**4.3 系统文档**
- [ ] 架构设计文档
  - 系统架构图
  - 模块交互流程
  - 数据结构说明
- [ ] API文档
  - 配置文件格式
  - 编程接口(如自定义策略)
- [ ] 用户手册
  - 安装指南
  - 快速开始教程
  - 常见问题FAQ
- [ ] 毕业设计论文
  - 研究背景与意义
  - 相关工作综述
  - 系统设计与实现
  - 实验评估
  - 总结与展望

**交付物**:
- 完整的docs/目录
- 毕业设计论文初稿

---

**4.4 开源准备(可选)**
- [ ] 代码清理与注释
- [ ] 添加开源许可证(MIT/Apache 2.0)
- [ ] 准备README.md
- [ ] 创建GitHub仓库
- [ ] 撰写技术博客

---

## 📊 里程碑与交付物总结

| 阶段 | 时间 | 核心交付物 | 验证标准 |
|------|------|-----------|---------|
| 阶段一 | 2025.11-2026.01 | Tracer+Injector+eBPF原型 | 能输出内存访问热力图 |
| 阶段二 | 2026.02-2026.03 | CXLMemSim Alpha | STREAM带宽限制在预期范围 |
| 阶段三 | 2026.03-2026.04 | 一致性+策略+多线程优化 | Redis/图计算性能符合预期 |
| 阶段四 | 2026.04-2026.05 | 完整系统+文档+论文 | LLM推理结果与真实硬件趋势吻合 |

---

## 🔧 技术栈

### 核心技术
- **语言**: C++17
- **构建**: CMake 3.20+
- **内核追踪**: eBPF (libbpf), Linux Perf
- **硬件特性**: Intel PEBS, LBR
- **配置管理**: nlohmann/json

### 依赖库
- `libbpf`: eBPF程序加载
- `libelf`: ELF文件解析
- `boost::graph` (可选): 拓扑图算法

### 测试工具
- Intel MLC (Memory Latency Checker)
- STREAM Benchmark
- Llama.cpp
- Redis

---

## 📈 成功标准

### 性能目标
- 仿真速度: 慢于原生3-6倍(目标<10倍)
- 延迟精度: ±10ns (相对真实CXL硬件)
- 带宽精度: ±5%

### 功能目标
- 支持CXL 1.1/2.0/3.0协议建模
- 支持多级交换机拓扑
- 支持至少3种内存分配策略
- 支持多线程并发(>32线程)

### 学术目标
- 验证结果能重现至少2篇顶会论文的实验趋势
- 提供可复现的实验配置

---

## 🚀 第一步行动项 (本周任务)

### 1. 环境配置检查
```bash
# 检查CPU PMU支持
perf list | grep mem_load_retired.l3_miss
perf list | grep lbr

# 检查perf权限
cat /proc/sys/kernel/perf_event_paranoid
# 如果>1, 需要设置为-1或0

# 检查大页支持
grep Huge /proc/meminfo
```

### 2. 安装依赖
```bash
# Ubuntu/Debian
sudo apt install -y \
  build-essential \
  cmake \
  libbpf-dev \
  libelf-dev \
  linux-tools-common \
  linux-tools-generic \
  clang \
  llvm

# 验证eBPF环境
bpftool --version
```

### 3. 创建项目结构
```bash
cd /home/xiaowu/work/CXLMemSim
mkdir -p backend/{ebpf,include,src/{tracer,analyzer,injector}}
mkdir -p frontend/components
mkdir -p common tests docs configs
touch README.md
```

### 4. 编写第一个测试程序
创建一个简单的PEBS测试程序验证环境:
```cpp
// tests/test_pebs.cpp
// 使用perf_event_open捕获LLC_MISS事件
```

---

## 📞 联系与支持

- **项目路径**: `/home/xiaowu/work/CXLMemSim`
- **文档参考**: `docx/1.docx`, `docx/江涛-CXL内存仿真平台设计与实现-任务书.docx`
- **进度追踪**: 使用planning/目录下的Markdown文件记录

---

**下一步**: 执行第一阶段任务1.1 - 环境准备与配置
