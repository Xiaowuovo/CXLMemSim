# 虚拟机环境能力评估报告

## 📊 环境信息

- **虚拟化平台**: 检测到 `hypervisor` 标志
- **物理CPU**: Intel Core i7-14700K (14代酷睿)
- **架构特性**: 支持AVX2, SHA-NI等先进指令集

---

## ✅ 好消息：虚拟机PMU能力超出预期！

### 可用的关键事件

经过检查，您的虚拟机环境**实际上暴露了大量PMU事件**，包括：

#### 内存加载事件 ✅
```
✅ mem_load_retired.l3_miss           # 核心事件！L3缓存未命中
✅ mem_load_retired.l1_miss
✅ mem_load_retired.l2_miss
✅ mem_load_retired.l3_hit
✅ mem_load_l3_miss_retired.local_dram # L3 Miss后访问本地DRAM
✅ mem_load_uops_retired.dram_hit
```

#### 内存存储事件 ✅
```
✅ mem-stores (基础存储事件)
```

#### 其他有用事件 ✅
```
✅ cache-misses / cache-references
✅ topdown-* (微架构分析)
```

---

## ⚠️ 限制与不确定性

### 1. PEBS精确采样
**状态**: 需要测试验证

虽然事件列表中有内存事件，但不确定是否支持PEBS的`:pp`修饰符。

**测试方法** (需要先修复perf权限):
```bash
# 测试1: 不带PEBS修饰符
perf record -e mem_load_retired.l3_miss ls

# 测试2: 带PEBS修饰符
perf record -e mem_load_retired.l3_miss:pp ls
```

**如果支持PEBS**:
- ✅ 可以获取精确的数据地址 (Data Linear Address)
- ✅ 虚拟机中就能进行完整的Tracer开发

**如果不支持PEBS**:
- ⚠️ 只能获取事件计数，无法获取精确地址
- ⚠️ 需要使用替代方案或等待物理机

### 2. LBR (Last Branch Record)
**状态**: 通常在虚拟机中不可用

在perf list中未看到明确的`lbr`相关事件，虚拟机通常不暴露LBR。

**影响**:
- MLP (Memory Level Parallelism) 分析会受限
- 可以暂时忽略，或使用简化模型

### 3. 性能计数器权限
**当前状态**: `perf_event_paranoid = 4` (限制最严格)

**必须修改**:
```bash
sudo sysctl -w kernel.perf_event_paranoid=-1
```

---

## 🎯 虚拟机开发策略 (推荐)

基于以上发现，我们可以采用**更积极的虚拟机开发策略**：

### 方案：虚拟机作为主要开发环境 ✅

```
阶段1-3：在虚拟机完成80-90%的开发
├── ✅ 如果PEBS可用：完整的Tracer开发
├── ⚠️ 如果PEBS不可用：使用软件采样作为fallback
├── ✅ 完整的Analyzer/Injector开发
├── ✅ 完整的Qt前端开发
└── ✅ 使用STREAM等基准测试验证逻辑

阶段4：物理机最终验证
├── 真实CXL硬件特性验证
├── 大规模负载测试
└── 论文数据采集
```

### Tracer实现策略

创建**分级fallback机制**：

```cpp
class Tracer {
public:
    static std::unique_ptr<Tracer> create() {
        // 优先级1: 尝试PEBS (精确采样)
        if (check_pebs_support()) {
            return std::make_unique<PEBSTracer>();
        }

        // 优先级2: 软件采样 (虚拟机fallback)
        if (check_software_events()) {
            return std::make_unique<SoftwareTracer>();
        }

        // 优先级3: Mock数据 (纯测试)
        return std::make_unique<MockTracer>();
    }
};
```

**SoftwareTracer** (虚拟机可用版本):
- 使用 `mem_load_retired.l3_miss` (不带:pp)
- 通过 `/proc/[pid]/maps` 推断访问的地址范围
- 精度降低，但逻辑可验证

---

## 📋 立即行动项

### 1. 修复perf权限 (最高优先级)
```bash
# 临时修改
sudo sysctl -w kernel.perf_event_paranoid=-1

# 永久修改
echo "kernel.perf_event_paranoid = -1" | sudo tee -a /etc/sysctl.conf
```

### 2. 测试PEBS支持
```bash
# 创建测试脚本
cat > /tmp/test_pebs.sh << 'EOF'
#!/bin/bash
echo "测试1: 基本mem_load事件"
perf stat -e mem_load_retired.l3_miss ls 2>&1 | tail -5

echo -e "\n测试2: 尝试记录采样"
perf record -e mem_load_retired.l3_miss -c 10000 ls 2>&1 | tail -5

echo -e "\n测试3: 尝试PEBS精确采样"
perf record -e mem_load_retired.l3_miss:pp -c 10000 ls 2>&1 | tail -5
EOF

chmod +x /tmp/test_pebs.sh
# 修复权限后运行: /tmp/test_pebs.sh
```

### 3. 安装Qt开发环境
```bash
# 安装Qt6 (推荐)
sudo apt install -y \
    qt6-base-dev \
    qt6-tools-dev \
    qt6-tools-dev-tools \
    libqt6charts6-dev \
    qt6-creator  # IDE (可选)

# 验证Qt安装
qmake6 --version
# 或
cmake --find-package -DNAME=Qt6 -DCOMPILER_ID=GNU -DLANGUAGE=CXX -DMODE=EXIST
```

---

## 🏗️ 更新的项目结构 (支持Qt)

```
CXLMemSim/
├── CMakeLists.txt              # 根CMake (支持后端+Qt前端)
├── backend/
│   ├── CMakeLists.txt
│   ├── include/
│   │   ├── tracer/
│   │   │   ├── tracer_interface.h     # 抽象基类
│   │   │   ├── pebs_tracer.h          # PEBS实现 (物理机优先)
│   │   │   ├── software_tracer.h      # 软件采样 (虚拟机fallback)
│   │   │   └── mock_tracer.h          # 模拟数据 (测试)
│   │   ├── analyzer/
│   │   │   ├── timing_analyzer.h
│   │   │   └── bandwidth_model.h
│   │   └── injector/
│   │       └── delay_injector.h
│   └── src/
│       ├── tracer/
│       │   ├── tracer_factory.cpp     # 自动选择最佳Tracer
│       │   ├── pebs_tracer.cpp
│       │   ├── software_tracer.cpp
│       │   └── mock_tracer.cpp
│       ├── analyzer/
│       └── injector/
├── frontend/                   # Qt前端
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── mainwindow.h/cpp/ui
│   ├── widgets/
│   │   ├── topology_editor.h/cpp      # 拓扑图形编辑器
│   │   ├── metrics_panel.h/cpp        # 性能指标面板
│   │   └── trace_viewer.h/cpp         # Trace可视化
│   ├── models/                         # Qt数据模型
│   │   ├── topology_model.h
│   │   └── config_model.h
│   └── resources/
│       ├── icons/
│       ├── styles/
│       └── resources.qrc
├── common/                     # 前后端共享
│   ├── include/
│   │   ├── data_structures.h          # 通用数据结构
│   │   ├── config_parser.h            # JSON配置解析
│   │   └── simulator_api.h            # 后端API封装
│   └── src/
├── third_party/                # 第三方库
│   └── qcustomplot/           # 图表库 (待下载)
├── tests/
│   ├── unit/
│   ├── integration/
│   └── benchmarks/
└── docs/
```

---

## 🎨 Qt前端功能规划

### 主窗口模块

#### 1. 拓扑编辑器 (TopologyEditor)
**功能**:
- 拖拽式图形界面编辑CXL拓扑
- 节点类型: Root Complex, Switch, CXL Memory
- 连线配置: 带宽(GB/s), 延迟(ns), PCIe代数(Gen4/5)
- 自动布局和手动调整
- 导出为JSON配置文件

**技术**:
- 继承 `QGraphicsView` + `QGraphicsScene`
- 自定义 `QGraphicsItem` 子类表示节点
- 节点属性编辑用 `QDockWidget`

#### 2. 配置管理器 (ConfigManager)
**功能**:
- 树形视图显示配置层级
- 内存策略配置 (Interleave, Tiering, First-touch)
- 预设拓扑模板 (单CXL, 双Switch, Fabric)
- 配置导入/导出/版本管理

**技术**:
- `QTreeView` + `QStandardItemModel`
- `QSettings` 保存用户偏好

#### 3. 性能监控面板 (MetricsPanel)
**功能**:
- 实时延迟曲线图
- 带宽利用率图
- 拥塞热力图
- 性能对比 (原生 vs 模拟)

**技术**:
- `QCustomPlot` 或 `QtCharts`
- 定时器刷新 (`QTimer`)

#### 4. Trace查看器 (TraceViewer)
**功能**:
- 时序图显示内存访问事件
- 过滤器 (按地址范围、线程ID、时间)
- 统计信息 (访问热点、Miss率)
- 导出trace数据为CSV

**技术**:
- `QTableView` + 自定义Model
- `QSortFilterProxyModel` 过滤

#### 5. 仿真控制 (SimulationControl)
**功能**:
- 目标程序选择 (文件浏览器)
- 启动/暂停/停止仿真
- Epoch配置 (时间片大小)
- 日志输出窗口

**技术**:
- `QProcess` 启动目标程序
- `QThread` 运行后端仿真
- `QTextBrowser` 显示日志

---

## 🚀 第一周任务清单 (虚拟机环境)

### Day 1-2: 环境配置
- [ ] 修复perf权限
- [ ] 测试PEBS支持情况
- [ ] 安装Qt6开发环境
- [ ] 安装所有其他依赖

### Day 3-4: 项目框架
- [ ] 创建完整目录结构
- [ ] 编写根CMakeLists.txt (支持Qt)
- [ ] 创建Tracer抽象接口
- [ ] 实现MockTracer (从JSON读取模拟数据)

### Day 5-6: Qt原型
- [ ] 创建MainWindow基本布局
- [ ] 实现TopologyEditor基础功能 (添加节点、连线)
- [ ] 配置文件解析和序列化

### Day 7: 集成测试
- [ ] 前端加载配置并显示拓扑
- [ ] 后端读取配置并初始化模型
- [ ] 端到端流程验证

---

## 📞 下一步决策点

您现在有两个选择：

### 选项A: 先测试PMU能力 (推荐)
1. 修复perf权限
2. 运行PEBS测试脚本
3. 根据结果决定Tracer实现策略

### 选项B: 直接开始Qt前端开发
1. 安装Qt环境
2. 创建界面原型
3. 后端暂时用Mock数据

**我的建议**: 选择A，花10分钟搞清楚虚拟机的真实能力，这样可以更准确地规划后续开发。

---

**您希望我现在做什么？**
1. 帮您运行PEBS测试 (需要您先手动执行 `sudo sysctl -w kernel.perf_event_paranoid=-1`)
2. 开始创建支持Qt的项目结构
3. 编写Tracer的抽象接口定义
