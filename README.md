# CXLMemSim - CXL Memory Simulator

CXL 内存模拟器，用于在虚拟机环境中开发和测试 CXL 内存拓扑与性能分析。

## 特性

- ️ **虚拟机友好**: 使用 MockTracer 模拟内存访问，无需物理硬件
- 🎨 **Qt6 GUI**: 专业的图形界面，支持拓扑编辑和性能可视化
- 📊 **多拓扑支持**: 支持复杂的 CXL fabric 和交换机配置
- ⚡ **轻量级**: 纯模拟模式，适合开发和原型验证
- 🔧 **易于部署**: 一键安装脚本，5分钟完成环境搭建

## 架构

```
Qt 前端 → 模拟器核心 → MockTracer
            ↓
      分析器 + 延迟注入器
```

## 快速开始

### 一键部署（推荐）

```bash
# 克隆或下载项目到 Ubuntu 虚拟机
cd /path/to/CXLMemSim

# 一键安装、编译、测试
./scripts/setup_vm.sh

# 启动 GUI
./run_gui.sh
```

### 手动部署

```bash
# 1. 安装依赖
sudo apt update
sudo apt install -y build-essential cmake git \
    nlohmann-json3-dev libgtest-dev \
    qt6-base-dev qt6-tools-dev

# 2. 编译项目
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# 3. 验证安装
./cxlmemsim --check-tracer
# 应显示: "Best Tracer: MOCK"

# 4. 启动 GUI
cd ..
./run_gui.sh
```

📖 **详细文档**: 查看 [DOCS_INDEX.md](DOCS_INDEX.md) 了解所有文档

## 文档

- [� 快速开始指南](QUICKSTART.md) - 5分钟上手教程
- [� 文档索引](DOCS_INDEX.md) - 所有文档的导航
- [🎨 前端开发](frontend/README.md) - Qt GUI 开发指南
- [⚙️ 配置示例](configs/examples/) - CXL 拓扑配置文件

## 开发状态

✅ **虚拟机环境功能完整**

- [x] 项目结构
- [x] Tracer 抽象层
- [x] MockTracer 实现（VM 模拟）
- [x] Qt6 前端 GUI
- [x] 拓扑图编辑器
- [x] 时序分析器
- [x] 延迟/带宽模型
- [x] 配置管理系统

## 系统要求

- **操作系统**: Ubuntu 20.04+ (或其他 Linux 发行版)
- **编译器**: GCC 9+ 或 Clang 10+
- **构建工具**: CMake 3.20+
- **GUI 框架**: Qt 6.2+ (可选，用于图形界面)
- **开发库**: nlohmann-json, Google Test

**注意**: 本项目已适配为纯虚拟机环境，不需要物理 CXL 硬件

## 使用示例

```bash
# 检查 Tracer 状态
./build/cxlmemsim --check-tracer

# 测试 Mock Tracer
./build/cxlmemsim --test-mock

# 启动 GUI 编辑 CXL 拓扑
./run_gui.sh
```

## 项目结构

```
CXLMemSim/
├── backend/          # 后端核心引擎
│   ├── tracer/       # 内存访问追踪器
│   ├── topology/     # 拓扑图管理
│   └── analyzer/     # 时序分析器
├── frontend/         # Qt6 图形界面
├── common/           # 公共代码（配置解析等）
├── configs/          # 配置文件示例
├── tests/            # 单元测试
└── scripts/          # 部署和工具脚本
```

## License

TBD

## Authors

Jiang Tao - CXL Memory Simulation Platform
