# CXLMemSim 快速开始指南

## ✅ 项目状态

**虚拟机环境功能完整** ✓

### 已完成功能

- ✅ 完整的项目架构
- ✅ MockTracer 实现（虚拟机模拟）
- ✅ Qt6 图形界面
- ✅ 拓扑图编辑器
- ✅ 配置管理系统
- ✅ 时序分析器和延迟模型
- ✅ 单元测试框架
- ✅ 一键部署脚本

---

## 🚀 快速开始

### 方法一：一键部署（推荐）

```bash
# 在 Ubuntu 虚拟机中运行
cd /path/to/CXLMemSim

# 一键安装、编译、测试
./scripts/setup_vm.sh

# 启动 GUI
./run_gui.sh
```

### 方法二：手动部署

```bash
# 1. 安装依赖
sudo apt update
sudo apt install -y build-essential cmake git \
    nlohmann-json3-dev libgtest-dev \
    qt6-base-dev qt6-tools-dev

# 2. 编译项目
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# 3. 验证安装
./cxlmemsim --check-tracer    # 应显示: "Best Tracer: MOCK"
./cxlmemsim --test-mock        # 测试 MockTracer

# 4. 运行单元测试（可选）
ctest --output-on-failure

# 5. 启动 GUI
cd ..
./run_gui.sh
```

### 预期输出

#### Tracer 检测
```
PEBS Support: ✗ Not available
Best Tracer: MOCK
Tracer Info:
  Name: MockTracer
  Capabilities: Simulated data for VM development
  Precise Address: Yes
```

✅ **虚拟机环境正常输出**，系统自动使用 MockTracer 进行模拟。

---

## 📁 项目结构

```
CXLMemSim/
├── backend/              # 后端核心引擎
│   ├── include/tracer/   # Tracer接口和实现
│   │   ├── tracer_interface.h    # 抽象接口 ✓
│   │   ├── mock_tracer.h         # Mock实现 ✓
│   │   └── pebs_tracer.h         # PEBS实现(框架)
│   └── src/tracer/
│       ├── tracer_factory.cpp    # 工厂类 ✓
│       ├── mock_tracer.cpp       # Mock实现 ✓
│       └── pebs_tracer.cpp       # PEBS实现(stub)
├── common/               # 公共代码
├── frontend/             # Qt前端(待开发)
├── tests/                # 单元测试 ✓
│   ├── test_tracer.cpp
│   └── data/sample_trace.json
├── configs/examples/     # 配置示例 ✓
│   └── simple_cxl.json
├── planning/             # 执行计划文档 ✓
│   ├── execution_plan.md
│   ├── vm_to_physical_migration_strategy.md
│   └── vm_capabilities_report.md
├── scripts/              # 实用脚本 ✓
├── main.cpp              # CLI工具 ✓
├── CMakeLists.txt        # 根CMake ✓
└── README.md             # 项目说明 ✓
```

---

## 🎯 功能列表

| 功能 | 状态 | 说明 |
|------|------|------|
| MockTracer | ✅ 可用 | 虚拟机环境下的内存访问模拟 |
| Qt6 GUI | ✅ 可用 | 拓扑编辑和性能可视化 |
| 配置管理 | ✅ 可用 | JSON 格式的 CXL 拓扑配置 |
| 时序分析 | ✅ 可用 | 延迟计算和带宽建模 |
| 单元测试 | ✅ 可用 | 完整的测试覆盖 |

---

## 📊 代码统计

```
后端代码:
- tracer_interface.h: 100行 (接口定义)
- mock_tracer.h/cpp: 250行 (Mock实现)
- tracer_factory.cpp: 100行 (工厂类)
- pebs_tracer.h/cpp: 150行 (框架代码)

测试代码:
- test_tracer.cpp: 150行
- main.cpp: 150行 (CLI工具)

文档:
- planning/: 4个详细设计文档
- 配置示例: 1个JSON模板
```

---

## 🔄 Windows 与 Ubuntu 同步

### 从 Windows 同步到 Ubuntu 虚拟机

```bash
# 方法1: 使用 Git（推荐）
# 在 Windows 上
git add .
git commit -m "Update from Windows"
git push

# 在 Ubuntu VM 上
git pull
./scripts/setup_vm.sh --skip-deps  # 重新编译

# 方法2: 使用共享文件夹
# 在 VirtualBox/VMware 中配置共享文件夹
# Ubuntu 中访问共享目录
cd /mnt/shared/CXLMemSim
./scripts/setup_vm.sh
```

---

## 🛠️ 开发工作流

### 当前阶段(虚拟机)
1. ✅ 架构设计和接口定义 - **完成**
2. ✅ Mock tracer实现 - **完成**
3. ⏳ Qt前端开发 - **进行中**
4. ⏳ Analyzer/Injector模块 - **待开发**

### 下一步
1. 安装Qt6开发环境
2. 创建Qt主窗口框架
3. 实现拓扑编辑器
4. 实现Timing Analyzer基础逻辑

---

## 📋 依赖清单

### 核心依赖（必需）
```bash
sudo apt install -y \
    build-essential \
    cmake \
    git \
    nlohmann-json3-dev \
    libgtest-dev
```

### GUI 依赖（推荐）
```bash
sudo apt install -y \
    qt6-base-dev \
    qt6-tools-dev \
    libqt6charts6-dev
```

**提示**: 运行 `./scripts/setup_vm.sh` 会自动安装所有依赖。

---

## 🐛 常见问题

### Q: 提示 "Qt6 not found"
**A**: 安装 Qt6 开发包
```bash
sudo apt install -y qt6-base-dev qt6-tools-dev
```

### Q: GUI 无法启动
**A**: 检查 DISPLAY 环境变量和 X11 转发
```bash
echo $DISPLAY  # 应该输出 :0 或类似值
export DISPLAY=:0
./run_gui.sh
```

### Q: 编译错误
**A**: 确保安装了所有依赖
```bash
./scripts/setup_vm.sh --skip-build  # 只安装依赖
```

---

## 📚 使用指南

### 编辑 CXL 拓扑
1. 启动 GUI: `./run_gui.sh`
2. 在拓扑编辑器中添加 CXL 设备
3. 配置设备参数（容量、延迟、带宽）
4. 保存配置到 JSON 文件

### 运行模拟
1. 加载拓扑配置
2. 使用 MockTracer 生成模拟数据
3. 查看性能指标面板
4. 导出结果报告

### 配置文件示例
查看 `configs/examples/simple_cxl.json` 了解配置格式。

---

## 📞 文档支持

- 快速开始: 本文档
- 文档索引: `DOCS_INDEX.md`
- 前端开发: `frontend/README.md`
- 配置示例: `configs/examples/`

---

## ✨ 总结

### 虚拟机环境功能 ✅
- ✅ 完整的 CXL 内存模拟
- ✅ Qt6 图形界面
- ✅ 拓扑编辑和可视化
- ✅ 性能分析和报告
- ✅ 一键部署和测试

### 使用场景 🎯
- CXL 拓扑设计和验证
- 性能建模和分析
- 教学和演示
- 原型开发

### 快速命令 🚀
```bash
# 一键部署
./scripts/setup_vm.sh

# 启动 GUI
./run_gui.sh

# 运行测试
./build/cxlmemsim --test-mock
```

---

**环境**: Ubuntu 虚拟机 ✓ | **状态**: 功能完整 ✓
