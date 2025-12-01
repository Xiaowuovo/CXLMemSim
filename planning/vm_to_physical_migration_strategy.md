# 虚拟机开发 → 物理服务器部署策略

## 🔍 虚拟机限制确认

### 测试结果分析
```bash
Error: PMU Hardware doesn't support sampling/overflow-interrupts
```

**结论**：
- ❌ 虚拟机**不支持**PEBS精确采样 (这是预期的)
- ❌ 虚拟机**不支持**基于中断的采样
- ✅ 虚拟机**支持** `perf stat` (统计计数)
- ✅ 虚拟机**支持**eBPF追踪

---

## 🏗️ 架构设计：抽象层解决方案

### 核心思想：接口隔离 + 编译时选择

通过设计良好的抽象层，使代码可以：
1. 在虚拟机中**完整开发和测试**（使用Mock/Simulation模式）
2. 在物理机上**无缝切换**到真实硬件采样

---

## 📐 分层架构设计

```
┌─────────────────────────────────────────────┐
│          Qt Frontend (GUI)                  │  ← 完全平台无关
│   - Topology Editor                         │     虚拟机开发100%
│   - Config Manager                          │
│   - Metrics Viewer                          │
└─────────────────┬───────────────────────────┘
                  │ Qt Signal/Slot
┌─────────────────▼───────────────────────────┐
│      Simulator Core (Platform Agnostic)     │  ← 平台无关逻辑
│   - TimingAnalyzer                          │     虚拟机开发100%
│   - BandwidthModel                          │
│   - TopologyGraph                           │
│   - ConfigParser                            │
└─────────────────┬───────────────────────────┘
                  │ Abstract Interface
┌─────────────────▼───────────────────────────┐
│      ITracer (Abstract Interface)           │  ← 关键抽象层
└─────────────────┬───────────────────────────┘
                  │
        ┌─────────┼─────────┐
        │         │         │
┌───────▼───┐ ┌──▼──────┐ ┌▼──────────┐
│ MockTracer│ │ Software│ │PEBSTracer │
│           │ │ Tracer  │ │           │
│ (VM Dev)  │ │ (VM可用)│ │(仅物理机) │
└───────────┘ └─────────┘ └───────────┘
```

---

## 💻 代码实现：抽象接口设计

### 1. Tracer抽象基类

```cpp
// backend/include/tracer/tracer_interface.h
#pragma once
#include <vector>
#include <memory>
#include <cstdint>

namespace cxlsim {

// 内存访问事件结构
struct MemoryAccessEvent {
    uint64_t timestamp_ns;      // 时间戳
    uint64_t virtual_addr;      // 虚拟地址
    uint64_t physical_addr;     // 物理地址 (如果可获取)
    uint32_t tid;               // 线程ID
    uint32_t cpu;               // CPU核心
    uint64_t latency_cycles;    // 延迟周期数 (如果可测量)
    bool is_load;               // true=读, false=写
    bool is_l3_miss;            // 是否L3 Miss

    // 扩展字段
    uint64_t ip;                // 指令指针 (如果可获取)
    uint32_t data_source;       // 数据来源 (DRAM/远程等)
};

// Tracer抽象接口
class ITracer {
public:
    virtual ~ITracer() = default;

    // 初始化追踪器
    virtual bool initialize(pid_t target_pid) = 0;

    // 启动追踪
    virtual bool start() = 0;

    // 停止追踪
    virtual void stop() = 0;

    // 收集一个epoch的采样数据
    virtual std::vector<MemoryAccessEvent> collect_samples() = 0;

    // 获取追踪器类型名称
    virtual std::string name() const = 0;

    // 获取追踪器能力描述
    virtual std::string capabilities() const = 0;

    // 检查是否支持精确地址采样
    virtual bool supports_precise_address() const = 0;
};

// Tracer工厂类 (自动选择最佳实现)
class TracerFactory {
public:
    enum class TracerType {
        AUTO,           // 自动检测
        PEBS,          // Intel PEBS (仅物理机)
        SOFTWARE,      // 软件采样 (虚拟机可用)
        MOCK           // 模拟数据 (开发测试)
    };

    // 创建最佳可用的Tracer
    static std::unique_ptr<ITracer> create(TracerType type = TracerType::AUTO);

    // 检测系统支持的Tracer类型
    static TracerType detect_best_tracer();

    // 检查PEBS是否可用
    static bool is_pebs_available();
};

} // namespace cxlsim
```

### 2. Mock Tracer实现 (虚拟机开发用)

```cpp
// backend/include/tracer/mock_tracer.h
#pragma once
#include "tracer_interface.h"
#include <random>
#include <nlohmann/json.hpp>

namespace cxlsim {

class MockTracer : public ITracer {
public:
    MockTracer();
    ~MockTracer() override = default;

    bool initialize(pid_t target_pid) override;
    bool start() override;
    void stop() override;
    std::vector<MemoryAccessEvent> collect_samples() override;

    std::string name() const override { return "MockTracer"; }
    std::string capabilities() const override {
        return "Simulated data for development";
    }
    bool supports_precise_address() const override { return true; }

    // Mock专用功能
    void load_trace_file(const std::string& json_path);
    void set_simulation_params(double l3_miss_rate, uint64_t avg_latency_ns);

private:
    std::vector<MemoryAccessEvent> preloaded_trace_;
    size_t current_index_ = 0;

    // 随机数生成器 (用于生成模拟数据)
    std::mt19937_64 rng_;
    double l3_miss_rate_ = 0.05;  // 模拟5% L3 Miss率
    uint64_t avg_latency_ns_ = 100;

    MemoryAccessEvent generate_random_event();
};

} // namespace cxlsim
```

```cpp
// backend/src/tracer/mock_tracer.cpp
#include "tracer/mock_tracer.h"
#include <fstream>
#include <chrono>

namespace cxlsim {

MockTracer::MockTracer()
    : rng_(std::random_device{}()) {
}

bool MockTracer::initialize(pid_t target_pid) {
    // Mock模式不需要真实的进程
    return true;
}

bool MockTracer::start() {
    current_index_ = 0;
    return true;
}

void MockTracer::stop() {
    // Nothing to do
}

std::vector<MemoryAccessEvent> MockTracer::collect_samples() {
    std::vector<MemoryAccessEvent> samples;

    // 如果有预加载的trace，使用它
    if (!preloaded_trace_.empty()) {
        size_t batch_size = std::min(size_t(1000),
                                     preloaded_trace_.size() - current_index_);
        samples.insert(samples.end(),
                      preloaded_trace_.begin() + current_index_,
                      preloaded_trace_.begin() + current_index_ + batch_size);
        current_index_ += batch_size;
        return samples;
    }

    // 否则生成随机模拟数据
    std::uniform_int_distribution<> count_dist(100, 500);
    int num_samples = count_dist(rng_);

    for (int i = 0; i < num_samples; ++i) {
        samples.push_back(generate_random_event());
    }

    return samples;
}

void MockTracer::load_trace_file(const std::string& json_path) {
    std::ifstream file(json_path);
    nlohmann::json j;
    file >> j;

    preloaded_trace_.clear();
    for (const auto& event : j["events"]) {
        MemoryAccessEvent e;
        e.timestamp_ns = event["timestamp"];
        e.virtual_addr = event["addr"];
        e.tid = event["tid"];
        e.is_l3_miss = event["l3_miss"];
        preloaded_trace_.push_back(e);
    }
}

MemoryAccessEvent MockTracer::generate_random_event() {
    MemoryAccessEvent e;

    auto now = std::chrono::steady_clock::now();
    e.timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();

    // 模拟虚拟地址 (假设程序使用0x1000000-0x10000000范围)
    std::uniform_int_distribution<uint64_t> addr_dist(0x1000000, 0x10000000);
    e.virtual_addr = addr_dist(rng_) & ~0x3F; // 对齐到64字节

    e.tid = 1234;
    e.cpu = 0;
    e.is_load = true;

    // 根据miss率决定是否miss
    std::uniform_real_distribution<> miss_dist(0.0, 1.0);
    e.is_l3_miss = (miss_dist(rng_) < l3_miss_rate_);

    if (e.is_l3_miss) {
        std::normal_distribution<> latency_dist(avg_latency_ns_, avg_latency_ns_ * 0.2);
        e.latency_cycles = std::max(uint64_t(50), uint64_t(latency_dist(rng_)));
    }

    return e;
}

} // namespace cxlsim
```

### 3. PEBS Tracer实现 (仅物理机)

```cpp
// backend/include/tracer/pebs_tracer.h
#pragma once
#include "tracer_interface.h"
#include <linux/perf_event.h>

namespace cxlsim {

class PEBSTracer : public ITracer {
public:
    PEBSTracer();
    ~PEBSTracer() override;

    bool initialize(pid_t target_pid) override;
    bool start() override;
    void stop() override;
    std::vector<MemoryAccessEvent> collect_samples() override;

    std::string name() const override { return "PEBSTracer"; }
    std::string capabilities() const override {
        return "Precise Event-Based Sampling with data address";
    }
    bool supports_precise_address() const override { return true; }

private:
    int perf_fd_ = -1;
    void* mmap_buf_ = nullptr;
    size_t mmap_size_ = 0;
    pid_t target_pid_ = -1;

    bool setup_perf_event();
    void parse_perf_buffer(std::vector<MemoryAccessEvent>& out);
};

} // namespace cxlsim
```

### 4. Tracer工厂实现

```cpp
// backend/src/tracer/tracer_factory.cpp
#include "tracer/tracer_factory.h"
#include "tracer/pebs_tracer.h"
#include "tracer/mock_tracer.h"
#include <sys/syscall.h>
#include <unistd.h>
#include <linux/perf_event.h>
#include <iostream>

namespace cxlsim {

bool TracerFactory::is_pebs_available() {
    // 尝试打开一个PEBS事件来检测支持
    struct perf_event_attr pe;
    memset(&pe, 0, sizeof(pe));
    pe.type = PERF_TYPE_RAW;
    pe.size = sizeof(pe);
    pe.config = 0x1cd;  // MEM_LOAD_RETIRED.L3_MISS
    pe.sample_period = 10000;
    pe.sample_type = PERF_SAMPLE_ADDR | PERF_SAMPLE_TIME | PERF_SAMPLE_TID;
    pe.precise_ip = 2;  // 需要PEBS
    pe.disabled = 1;

    int fd = syscall(__NR_perf_event_open, &pe, -1, 0, -1, 0);
    if (fd >= 0) {
        close(fd);
        return true;
    }

    return false;
}

TracerFactory::TracerType TracerFactory::detect_best_tracer() {
    // 1. 尝试PEBS
    if (is_pebs_available()) {
        std::cout << "[TracerFactory] PEBS is available - using hardware sampling" << std::endl;
        return TracerType::PEBS;
    }

    // 2. Fallback到Mock
    std::cout << "[TracerFactory] PEBS not available - using Mock tracer for development" << std::endl;
    std::cout << "[TracerFactory] This is expected in VM environments" << std::endl;
    return TracerType::MOCK;
}

std::unique_ptr<ITracer> TracerFactory::create(TracerType type) {
    if (type == TracerType::AUTO) {
        type = detect_best_tracer();
    }

    switch (type) {
        case TracerType::PEBS:
            if (!is_pebs_available()) {
                std::cerr << "[TracerFactory] PEBS requested but not available!" << std::endl;
                return nullptr;
            }
            return std::make_unique<PEBSTracer>();

        case TracerType::MOCK:
            return std::make_unique<MockTracer>();

        default:
            std::cerr << "[TracerFactory] Unknown tracer type!" << std::endl;
            return nullptr;
    }
}

} // namespace cxlsim
```

---

## 🔄 虚拟机到物理机的迁移流程

### 阶段1: 虚拟机开发 (当前)

```bash
# 在虚拟机中
cd /home/xiaowu/work/CXLMemSim
git init
git add .
git commit -m "Initial commit"

# 推送到远程仓库 (GitHub/GitLab)
git remote add origin <your-repo-url>
git push -u origin main
```

**开发内容**:
- ✅ 完整的Qt前端
- ✅ Analyzer/Injector/拓扑建模
- ✅ MockTracer完整测试
- ✅ 单元测试和集成测试
- ✅ 文档和配置示例

### 阶段2: 首次物理机部署

#### 方法A: Git Clone (推荐)
```bash
# 在物理服务器上
ssh user@physical-server
cd ~/work
git clone <your-repo-url> CXLMemSim
cd CXLMemSim

# 安装依赖
./scripts/install_dependencies.sh

# 编译 (会自动检测并启用PEBS)
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# 测试PEBS是否启用
./cxlmemsim --check-tracer
# 输出应显示: "Using PEBSTracer"
```

#### 方法B: rsync同步
```bash
# 从虚拟机同步到物理机
rsync -avz --exclude 'build' \
    /home/xiaowu/work/CXLMemSim/ \
    user@physical-server:~/work/CXLMemSim/
```

### 阶段3: 持续开发

```
虚拟机 (开发环境)          物理机 (测试环境)
     │                           │
     │  1. 开发新功能             │
     │  2. 用Mock测试             │
     │  3. Git commit             │
     ├────── Git push ──────────► │
     │                           │ 4. Git pull
     │                           │ 5. 重新编译
     │                           │ 6. 运行真实测试
     │ ◄────── 反馈结果 ──────────┤
     │                           │
```

---

## 📦 CMake编译系统设计

### 根CMakeLists.txt (自动检测环境)

```cmake
cmake_minimum_required(VERSION 3.20)
project(CXLMemSim VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 检测PEBS支持
include(CheckCXXSourceCompiles)

set(PEBS_TEST_CODE "
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <unistd.h>
int main() {
    struct perf_event_attr pe;
    pe.precise_ip = 2;
    syscall(__NR_perf_event_open, &pe, -1, 0, -1, 0);
    return 0;
}
")

check_cxx_source_compiles("${PEBS_TEST_CODE}" HAVE_PEBS_SUPPORT)

if(HAVE_PEBS_SUPPORT)
    message(STATUS "PEBS support detected - building with hardware tracer")
    add_compile_definitions(ENABLE_PEBS_TRACER)
else()
    message(WARNING "PEBS not available - using Mock tracer only")
    message(WARNING "This is expected in VM environments")
endif()

# Qt设置
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts)

# 添加子目录
add_subdirectory(backend)
add_subdirectory(common)
add_subdirectory(frontend)

# 主可执行文件
add_executable(cxlmemsim
    main.cpp
)

target_link_libraries(cxlmemsim
    PRIVATE
    backend_core
    common
    frontend_gui
)
```

### backend/CMakeLists.txt

```cmake
# backend/CMakeLists.txt

# 后端核心库
add_library(backend_core STATIC
    src/tracer/tracer_factory.cpp
    src/tracer/mock_tracer.cpp
    src/analyzer/timing_analyzer.cpp
    src/analyzer/bandwidth_model.cpp
    src/injector/delay_injector.cpp
)

# 仅在支持PEBS时编译
if(HAVE_PEBS_SUPPORT)
    target_sources(backend_core PRIVATE
        src/tracer/pebs_tracer.cpp
    )
endif()

target_include_directories(backend_core
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(backend_core
    PUBLIC
    common
)
```

---

## 🧪 自动化测试策略

### 虚拟机中的测试

```cpp
// tests/test_tracer_abstraction.cpp
#include <gtest/gtest.h>
#include "tracer/tracer_factory.h"

TEST(TracerFactory, AutoDetection) {
    auto tracer = TracerFactory::create(TracerFactory::TracerType::AUTO);
    ASSERT_NE(tracer, nullptr);

    std::cout << "Using: " << tracer->name() << std::endl;
    std::cout << "Capabilities: " << tracer->capabilities() << std::endl;
}

TEST(MockTracer, BasicFunctionality) {
    auto tracer = TracerFactory::create(TracerFactory::TracerType::MOCK);
    ASSERT_TRUE(tracer->initialize(1234));
    ASSERT_TRUE(tracer->start());

    auto samples = tracer->collect_samples();
    EXPECT_GT(samples.size(), 0);

    tracer->stop();
}

TEST(MockTracer, LoadTraceFile) {
    auto mock = dynamic_cast<MockTracer*>(
        TracerFactory::create(TracerFactory::TracerType::MOCK).get()
    );

    // 加载预录制的trace
    mock->load_trace_file("tests/data/sample_trace.json");
    mock->start();

    auto samples = mock->collect_samples();
    EXPECT_GT(samples.size(), 0);
}
```

### 物理机上的验证测试

```cpp
// tests/test_pebs_tracer.cpp (仅在物理机编译)
#ifdef ENABLE_PEBS_TRACER

#include <gtest/gtest.h>
#include "tracer/pebs_tracer.h"

TEST(PEBSTracer, Availability) {
    ASSERT_TRUE(TracerFactory::is_pebs_available())
        << "PEBS should be available on physical hardware";
}

TEST(PEBSTracer, RealSampling) {
    auto tracer = std::make_unique<PEBSTracer>();

    // 追踪一个简单的程序
    pid_t pid = fork();
    if (pid == 0) {
        // 子进程: 运行一个产生内存访问的程序
        execl("/bin/ls", "ls", nullptr);
        exit(1);
    }

    ASSERT_TRUE(tracer->initialize(pid));
    ASSERT_TRUE(tracer->start());

    sleep(1);  // 让程序运行一会

    auto samples = tracer->collect_samples();
    tracer->stop();

    EXPECT_GT(samples.size(), 0) << "Should capture some L3 misses";

    // 验证地址字段有效
    if (!samples.empty()) {
        EXPECT_NE(samples[0].virtual_addr, 0);
    }
}

#endif // ENABLE_PEBS_TRACER
```

---

## 🚀 迁移Checklist

### 虚拟机开发完成标准
- [ ] Qt前端所有功能实现
- [ ] MockTracer能加载和回放trace
- [ ] Analyzer逻辑完全测试
- [ ] 配置文件解析和序列化
- [ ] 单元测试覆盖率>80%
- [ ] 用户手册和API文档
- [ ] Git仓库干净，无临时文件

### 物理机首次部署
- [ ] 安装所有依赖 (`./scripts/install_dependencies.sh`)
- [ ] 配置perf权限
- [ ] 编译项目 (`cmake .. && make`)
- [ ] 检测Tracer类型 (`./cxlmemsim --check-tracer`)
- [ ] 运行单元测试 (`ctest`)
- [ ] 运行PEBS验证测试
- [ ] 采集真实trace样本

### 真实数据回传虚拟机
```bash
# 在物理机上采集真实trace
./cxlmemsim --mode=trace-only \
    --target=/usr/bin/redis-server \
    --output=real_trace.json \
    --duration=60

# 传回虚拟机
scp real_trace.json xiaowu@vm-host:/home/xiaowu/work/CXLMemSim/tests/data/

# 在虚拟机中用真实数据测试
./cxlmemsim --mock-trace=tests/data/real_trace.json
```

---

## 📊 开发进度里程碑

| 阶段 | 环境 | 时间 | 交付物 |
|------|------|------|--------|
| **Phase 1** | 虚拟机 | Week 1-2 | 项目框架 + 抽象接口 |
| **Phase 2** | 虚拟机 | Week 3-6 | Qt前端 + Mock完整功能 |
| **Phase 3** | 虚拟机 | Week 7-8 | 单元测试 + 文档 |
| **Phase 4** | 物理机 | Week 9 | 首次部署 + PEBS验证 |
| **Phase 5** | 虚拟机+物理机 | Week 10-14 | 迭代开发 + 真实测试 |
| **Phase 6** | 物理机 | Week 15-16 | 最终验证 + 论文数据 |

---

## 🎯 下一步行动

现在您已经清楚了虚拟机的限制和迁移策略，让我们开始实际开发：

### 立即执行 (本周)

1. **创建项目结构**
```bash
cd /home/xiaowu/work/CXLMemSim
./scripts/create_project_structure.sh  # 我会创建这个脚本
```

2. **编写Tracer抽象接口**
   - 创建上述的接口定义
   - 实现MockTracer
   - 实现TracerFactory

3. **设置Git仓库**
```bash
git init
git add .
git commit -m "Initial project structure with tracer abstraction"
```

4. **安装Qt环境**
```bash
sudo apt install qt6-base-dev qt6-tools-dev libqt6charts6-dev
```

---

**您希望我现在做什么？**

1. ✅ 创建完整的项目目录结构脚本
2. ✅ 生成Tracer抽象层的完整代码
3. ✅ 创建支持虚拟机/物理机的CMakeLists.txt
4. ✅ 编写第一个Qt窗口原型

选择一个，或者我按顺序全部完成？🚀
