# 架构更新：虚拟机开发 + Qt前端

## 📋 环境变化分析

### 1. 虚拟机环境的影响 ⚠️

#### 1.1 硬件性能计数器(PMU)限制

**主要问题**：
- 虚拟机通常**无法访问或只能部分访问**宿主机的PMU (Performance Monitoring Unit)
- PEBS (Precision Event-Based Sampling) 在虚拟机中**大概率不可用**
- LBR (Last Branch Record) 在虚拟机中**通常被禁用**

**影响**：
- 在虚拟机中**无法**进行完整的Tracer模块开发和测试
- 无法验证PEBS/LBR相关的核心功能

**解决方案**：

##### 方案A：虚拟机中进行模拟开发 ✅ (推荐当前阶段)
```
虚拟机阶段开发内容：
├── 1. 架构设计和接口定义
├── 2. 模拟数据生成器 (Mock Tracer)
├── 3. 时序分析器 (Timing Analyzer) - 可以用模拟数据测试
├── 4. 注入器 (Injector) - 基本功能可以测试
├── 5. Qt前端界面完整开发
└── 6. 配置文件解析和拓扑建模
```

在虚拟机中创建**Mock Tracer**：
```cpp
// mock_tracer.h
class MockTracer : public ITracer {
public:
    // 从预录制的trace文件读取，或生成模拟数据
    std::vector<MemoryAccessEvent> collect_samples() override;

    // 支持回放真实硬件采集的trace
    void load_trace_file(const std::string& path);
};
```

##### 方案B：定期在物理服务器上验证
```
开发流程：
1. 虚拟机中开发 (70% 时间)
   - 编写代码、设计界面
   - 使用Mock数据测试

2. 物理机上验证 (30% 时间)
   - SSH远程登录物理服务器
   - 编译真实Tracer模块
   - 运行集成测试
   - 收集真实trace数据回传到虚拟机
```

##### 方案C：混合开发环境
- 虚拟机：前端开发 + 分析器开发
- 物理机：后端Tracer服务（通过网络通信）

---

#### 1.2 虚拟机中可用的替代方案

**可以在虚拟机中测试的功能**：

✅ **eBPF追踪** (部分可用)
- 虚拟机内核通常支持eBPF
- 可以追踪虚拟机内的内存分配
- 但性能特征与物理机不同

✅ **基于软件事件的Perf**
```bash
# 这些在虚拟机中通常可用
perf stat -e cache-misses,cache-references ./app
perf record -e cache-misses ./app
```

❌ **精确采样(PEBS)**
```bash
# 这些在虚拟机中通常不可用
perf record -e mem_load_retired.l3_miss:pp ./app  # :pp 需要PEBS
```

**检查虚拟机PMU能力**：
```bash
# 运行这个看看虚拟机支持什么
cat /proc/cpuinfo | grep -i "pmu"
perf list | grep -i "mem_"
dmesg | grep -i "pmu"
```

---

### 2. Qt前端的架构调整 🎨

#### 2.1 项目结构更新

```
CXLMemSim/
├── backend/                    # 后端核心引擎 (C++)
│   ├── ebpf/                   # eBPF追踪程序
│   ├── include/
│   │   ├── tracer/
│   │   │   ├── tracer_interface.h    # 抽象接口
│   │   │   ├── pebs_tracer.h         # 真实PEBS实现 (仅物理机)
│   │   │   └── mock_tracer.h         # 模拟实现 (虚拟机)
│   │   ├── analyzer/
│   │   └── injector/
│   ├── src/
│   └── api/                    # 后端API接口 (供前端调用)
│       ├── simulator_api.h
│       └── simulator_api.cpp
├── frontend/                   # Qt前端 (C++ Qt6)
│   ├── main.cpp
│   ├── mainwindow.h/cpp        # 主窗口
│   ├── widgets/                # 自定义控件
│   │   ├── topology_editor.h   # 拓扑图编辑器
│   │   ├── metrics_viewer.h    # 性能指标显示
│   │   └── trace_viewer.h      # Trace数据可视化
│   ├── dialogs/                # 对话框
│   ├── resources/              # Qt资源文件
│   │   ├── icons/
│   │   ├── qml/               # QML界面(可选)
│   │   └── resources.qrc
│   └── ui/                     # Qt Designer UI文件
├── common/                     # 前后端共享
│   ├── data_structures.h       # 数据结构定义
│   ├── config_parser.h         # 配置解析
│   └── ipc/                    # 进程间通信(如果前后端分离)
├── tests/
├── docs/
└── CMakeLists.txt              # 根CMake (支持Qt和后端)
```

#### 2.2 前后端通信架构

**选项1：直接链接 (Monolithic)** ✅ 推荐开始阶段
```
┌─────────────────────────────┐
│      Qt GUI (Frontend)      │
│                             │
│  ┌─────────────────────┐   │
│  │  Topology Editor    │   │
│  ├─────────────────────┤   │
│  │  Config Manager     │   │
│  └──────────┬──────────┘   │
│             │               │
│   Qt Signal/Slot 调用       │
│             │               │
│  ┌──────────▼──────────┐   │
│  │  SimulatorAPI类     │   │  ← 封装后端功能
│  └──────────┬──────────┘   │
└─────────────┼───────────────┘
              │ 直接函数调用
┌─────────────▼───────────────┐
│    Backend Core Engine      │
│  (Tracer/Analyzer/Injector) │
└─────────────────────────────┘
```

**优点**：
- 简单，无需IPC开销
- 调试方便
- 适合单机部署

**缺点**：
- GUI和后端耦合
- 后端崩溃会导致整个程序崩溃


**选项2：进程分离 (Client-Server)** (可选，后期优化)
```
┌─────────────┐           ┌──────────────┐
│  Qt GUI     │  ◄─────►  │  Backend     │
│  (Client)   │   gRPC/   │  Daemon      │
│             │   ZMQ/    │  (需要root)  │
└─────────────┘   Socket  └──────────────┘
   虚拟机                    物理机/远程
```

**优点**：
- 前端可以远程控制物理机
- 后端需要root权限时不影响前端
- 易于分布式部署

**缺点**：
- 增加通信复杂度
- 需要设计序列化协议

#### 2.3 Qt前端功能模块设计

##### 主界面布局
```
┌─────────────────────────────────────────────────────────┐
│  File  Edit  Simulation  View  Tools  Help             │
├─────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌───────────────────────────────┐   │
│  │              │  │    Topology Editor            │   │
│  │  Config      │  │                               │   │
│  │  Tree        │  │  ┌──┐     ┌────┐   ┌──────┐  │   │
│  │              │  │  │RC├────►│SW0 ├──►│CXL0  │  │   │
│  │ ● Topology   │  │  └──┘     └────┘   └──────┘  │   │
│  │ ● Memory Map │  │                               │   │
│  │ ● Policies   │  │  [图形化拖拽编辑拓扑]          │   │
│  │ ● Analysis   │  └───────────────────────────────┘   │
│  └──────────────┘                                      │
├─────────────────────────────────────────────────────────┤
│  ┌─────────────────┐  ┌─────────────────────────────┐  │
│  │ Metrics Panel   │  │  Trace Viewer               │  │
│  │                 │  │  Time ────────────────►     │  │
│  │ Latency: 250ns  │  │  ████░░░░████░░░ (热力图)   │  │
│  │ Bandwidth: 32GB/│  │                             │  │
│  │ Congestion: 15% │  │  [采样数据时序图]            │  │
│  └─────────────────┘  └─────────────────────────────┘  │
├─────────────────────────────────────────────────────────┤
│  Status: Idle  │  Target: /usr/bin/redis-server       │
└─────────────────────────────────────────────────────────┘
```

##### 核心Widget列表

1. **TopologyEditor** (拓扑编辑器)
   - 继承QGraphicsView
   - 拖拽式添加节点(RC, Switch, CXL Device)
   - 连线配置带宽/延迟
   - 导出为JSON配置

2. **ConfigManager** (配置管理)
   - QTreeView显示配置层级
   - 参数编辑(延迟、带宽、策略)
   - 预设模板加载

3. **MetricsViewer** (指标可视化)
   - 实时延迟/带宽曲线(QCustomPlot)
   - 拥塞热力图
   - 性能对比(原生 vs 模拟)

4. **TraceViewer** (Trace查看器)
   - 时序图显示内存访问
   - 过滤、搜索功能
   - 导出trace数据

5. **SimulationControl** (仿真控制)
   - 启动/暂停/停止仿真
   - Epoch配置
   - 目标程序选择

---

### 3. 更新后的开发计划

#### 阶段一：虚拟机开发基础 (2025.11 - 2026.01)

**虚拟机中完成**：
- [x] 环境配置 (不含PEBS验证)
- [ ] 创建抽象Tracer接口
- [ ] 实现MockTracer (从JSON文件读取模拟数据)
- [ ] 开发Analyzer和Injector (使用模拟数据)
- [ ] 搭建Qt前端框架
- [ ] 实现拓扑编辑器

**物理机中完成** (首次访问物理机时)：
- [ ] 验证PEBS/LBR可用性
- [ ] 实现PEBSTracer (真实硬件版本)
- [ ] 采集真实trace样本回传到虚拟机

**交付物**：
- Mock版本的完整系统 (可在虚拟机运行)
- Qt界面原型
- 真实trace数据样本

#### 阶段二：Qt前端完整开发 (2026.02 - 2026.03)

**全部在虚拟机完成**：
- [ ] 完成所有Qt界面功能
- [ ] 集成QCustomPlot实现可视化
- [ ] 实现配置导入/导出
- [ ] 添加trace回放功能
- [ ] 编写前端单元测试

**物理机验证**：
- [ ] 定期同步代码到物理机
- [ ] 运行集成测试
- [ ] 性能基准测试

#### 阶段三 & 四：保持不变

---

## 🛠️ Qt开发环境配置

### 安装Qt

```bash
# Ubuntu 22.04+
sudo apt install -y \
    qt6-base-dev \
    qt6-tools-dev \
    qt6-tools-dev-tools \
    libqt6charts6-dev \
    qt6-documentation-tools

# 或者下载Qt官方安装器
wget https://download.qt.io/official_releases/online_installers/qt-unified-linux-x64-online.run
chmod +x qt-unified-linux-x64-online.run
./qt-unified-linux-x64-online.run
```

### 安装图表库

```bash
# QCustomPlot (推荐，轻量级)
cd /home/xiaowu/work/CXLMemSim/third_party
git clone https://gitlab.com/DerManu/QCustomPlot.git

# 或使用Qt Charts (Qt官方)
sudo apt install libqt6charts6-dev
```

### 更新CMakeLists.txt (支持Qt)

```cmake
cmake_minimum_required(VERSION 3.20)
project(CXLMemSim VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt设置
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Gui
    Charts  # 可选
)

# 后端库
add_subdirectory(backend)
add_subdirectory(common)

# Qt前端
add_subdirectory(frontend)

# 主可执行文件
add_executable(cxlmemsim
    frontend/main.cpp
    frontend/mainwindow.cpp
    # ... 其他源文件
)

target_link_libraries(cxlmemsim
    PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    backend_core
    common
)
```

---

## 📊 虚拟机 vs 物理机对比

| 功能模块 | 虚拟机 | 物理机 | 备注 |
|---------|--------|--------|------|
| Qt前端开发 | ✅ 完全支持 | ✅ 完全支持 | 虚拟机更方便 |
| Mock Tracer | ✅ 完全支持 | ✅ 完全支持 | 用于测试 |
| PEBS Tracer | ❌ 不支持 | ✅ 完全支持 | **核心功能** |
| eBPF追踪 | ⚠️ 部分支持 | ✅ 完全支持 | 虚拟机可测试基本功能 |
| Analyzer | ✅ 完全支持 | ✅ 完全支持 | 与硬件无关 |
| Injector | ✅ 完全支持 | ✅ 完全支持 | 与硬件无关 |
| 拓扑建模 | ✅ 完全支持 | ✅ 完全支持 | 纯软件 |
| 真实负载测试 | ❌ 数据不可信 | ✅ 完全支持 | **最终验证** |

---

## 🚀 推荐的开发流程

### 当前阶段 (虚拟机)
1. **Week 1-2**: 搭建项目框架 + Qt界面原型
2. **Week 3-4**: 实现Mock Tracer + Analyzer逻辑
3. **Week 5-6**: 完成Qt前端所有功能
4. **Week 7**: 准备首次物理机测试

### 首次物理机访问
1. **Day 1**:
   - 检查物理机PMU能力
   - 编译并运行PEBS测试
   - 收集真实trace样本
2. **Day 2**:
   - 运行完整集成测试
   - 性能基准测试
   - 记录问题列表
3. 将trace样本和日志带回虚拟机

### 后续迭代
- 虚拟机: 持续开发新功能
- 物理机: 每2-3周同步一次验证

---

## 📋 立即行动项 (更新)

### 1. 在虚拟机中检查PMU能力
```bash
# 看看虚拟机能提供什么
cat /proc/cpuinfo | grep flags
perf list | head -50
perf stat -e cache-misses ls  # 测试基本功能
```

### 2. 决定Qt版本
```bash
# 检查系统Qt版本
apt-cache search qt6-base
# 或
apt-cache search qt5-default
```

### 3. 更新项目规划
- 标记哪些功能在虚拟机开发
- 标记哪些功能需要物理机验证
- 设计Mock Tracer的接口

---

**您希望我现在帮您做什么？**
1. 先检查虚拟机的PMU能力？
2. 开始设计Qt界面原型？
3. 创建Mock Tracer框架？
4. 更新项目目录结构支持Qt？
