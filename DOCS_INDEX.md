# CXLMemSim 文档索引

本文档为 CXLMemSim 项目的完整文档导航，帮助您快速找到所需信息。

---

## 📖 核心文档（必读）

### [README.md](README.md)
**用途**: 项目概览和快速开始  
**适合**: 第一次使用的用户  
**内容**:
- 项目简介和特性
- 一键部署命令
- 系统要求
- 基本使用示例

### [QUICKSTART.md](QUICKSTART.md)
**用途**: 详细的快速开始指南  
**适合**: 想要深入了解部署过程的用户  
**内容**:
- 详细的安装步骤
- 手动部署说明
- 常见问题解答
- Windows 与 Ubuntu 同步方法
- 完整的使用指南

---

## 🎨 前端开发文档

### [frontend/README.md](frontend/README.md)
**用途**: Qt6 GUI 开发指南  
**适合**: 前端开发者  
**内容**:
- GUI 架构说明
- 主要组件介绍
- 开发环境配置
- 界面扩展指南

---

## ⚙️ 配置文件

### [configs/examples/simple_cxl.json](configs/examples/simple_cxl.json)
**用途**: CXL 拓扑配置示例  
**适合**: 所有用户  
**内容**:
- Root Complex 配置
- CXL 设备配置（容量、延迟、带宽）
- 连接关系定义
- 内存策略配置
- 模拟参数设置

---

## 🔧 脚本文档

### [scripts/setup_vm.sh](scripts/setup_vm.sh)
**用途**: Ubuntu 虚拟机一键部署脚本  
**功能**:
- 自动安装所有依赖
- 编译项目
- 运行测试
- 启动 GUI（可选）

### [run_gui.sh](run_gui.sh)
**用途**: GUI 启动脚本（已优化 VM 环境）  
**功能**:
- 自动配置 Qt 环境变量
- 检查 X11 显示
- 启动图形界面

---

## 📚 技术文档（高级）

### 后端 API
- **tracer_interface.h**: 内存追踪器抽象接口
- **mock_tracer.h**: MockTracer 实现（VM 模拟）
- **topology_graph.h**: CXL 拓扑图数据结构
- **timing_analyzer.h**: 时序分析器核心组件
- **latency_model.h**: 延迟计算模型
- **config_parser.h**: JSON 配置解析器

### 测试文件
- **tests/test_tracer.cpp**: Tracer 单元测试
- **tests/test_config_parser.cpp**: 配置解析测试
- **tests/test_integration.cpp**: 集成测试

---

## 📋 文档用途说明

### 用户类型指南

#### 🆕 新用户（首次使用）
1. 阅读 [README.md](README.md) 了解项目
2. 运行 `./scripts/setup_vm.sh` 一键部署
3. 查看 [QUICKSTART.md](QUICKSTART.md) 学习基本操作

#### 👨‍💻 开发者（修改代码）
1. 阅读 [QUICKSTART.md](QUICKSTART.md) 了解项目结构
2. 查看 [frontend/README.md](frontend/README.md) 学习 GUI 开发
3. 参考头文件中的 API 注释

#### 🎓 研究者（性能分析）
1. 研究 `configs/examples/simple_cxl.json` 配置格式
2. 了解 `backend/include/analyzer/` 中的分析模型
3. 使用 GUI 创建自定义 CXL 拓扑

---

## 🗂️ 项目结构对应文档

```
CXLMemSim/
├── README.md                 # 项目概览 ⭐
├── QUICKSTART.md             # 快速开始 ⭐
├── DOCS_INDEX.md             # 本文档 ⭐
│
├── backend/                  # 后端核心引擎
│   ├── include/              # 头文件（API 文档）
│   │   ├── tracer/           # 内存追踪器
│   │   ├── topology/         # 拓扑管理
│   │   └── analyzer/         # 性能分析
│   └── src/                  # 实现文件
│
├── frontend/                 # Qt6 GUI
│   └── README.md             # 前端开发指南 ⭐
│
├── common/                   # 公共代码
│   └── include/              
│       └── config_parser.h   # 配置解析 API
│
├── configs/                  # 配置文件
│   └── examples/
│       └── simple_cxl.json   # 配置示例 ⭐
│
├── scripts/                  # 部署脚本
│   ├── setup_vm.sh           # VM 部署脚本 ⭐
│   ├── install_dependencies.sh
│   └── verify_environment.sh
│
├── tests/                    # 单元测试
│   ├── test_tracer.cpp
│   ├── test_config_parser.cpp
│   └── test_integration.cpp
│
└── run_gui.sh                # GUI 启动脚本 ⭐
```

---

## ❓ 快速查找

### 我想...

| 目标 | 查看文档 |
|------|---------|
| 快速部署项目 | [README.md](README.md) → 运行 `./scripts/setup_vm.sh` |
| 了解详细安装步骤 | [QUICKSTART.md](QUICKSTART.md) |
| 学习如何使用 GUI | [QUICKSTART.md](QUICKSTART.md) → "使用指南" 章节 |
| 修改前端界面 | [frontend/README.md](frontend/README.md) |
| 创建 CXL 拓扑配置 | [configs/examples/simple_cxl.json](configs/examples/simple_cxl.json) |
| 解决编译问题 | [QUICKSTART.md](QUICKSTART.md) → "常见问题" 章节 |
| 从 Windows 同步代码 | [QUICKSTART.md](QUICKSTART.md) → "Windows 与 Ubuntu 同步" |
| 理解 MockTracer 原理 | `backend/include/tracer/mock_tracer.h` |
| 添加新的性能指标 | `backend/include/analyzer/timing_analyzer.h` |

---

## 🚀 推荐学习路径

### 路径 1: 快速体验（10分钟）
1. 阅读 [README.md](README.md) - 2分钟
2. 运行 `./scripts/setup_vm.sh` - 5分钟
3. 启动 GUI `./run_gui.sh` - 1分钟
4. 打开示例配置 `configs/examples/simple_cxl.json` - 2分钟

### 路径 2: 深入学习（1小时）
1. 阅读 [QUICKSTART.md](QUICKSTART.md) - 15分钟
2. 手动部署和测试 - 20分钟
3. 阅读 [frontend/README.md](frontend/README.md) - 10分钟
4. 研究配置文件和代码注释 - 15分钟

### 路径 3: 开发贡献（1天）
1. 完成路径 2 的所有内容
2. 阅读所有头文件中的 API 注释
3. 运行并理解所有单元测试
4. 尝试修改和扩展功能

---

## 📝 文档维护

### 已移除的文档
以下文档已被移除，因为本项目已适配为纯虚拟机环境：
- ~~DEPLOYMENT_QUICKSTART.md~~ - 物理服务器部署（已不需要）
- ~~docs/PHYSICAL_DEPLOYMENT.md~~ - 物理服务器详细部署（已不需要）
- ~~planning/*.md~~ - 开发计划文档（已完成，归档）

### 核心文档列表
保留的核心文档（带 ⭐ 标记）：
1. README.md - 项目入口
2. QUICKSTART.md - 快速开始指南
3. DOCS_INDEX.md - 本文档
4. frontend/README.md - 前端开发指南
5. configs/examples/simple_cxl.json - 配置示例

---

## 💡 提示

- 所有文档都包含中文说明，便于理解
- 代码注释使用英文，遵循行业规范
- 遇到问题先查看 [QUICKSTART.md](QUICKSTART.md) 的"常见问题"章节
- 使用 `./scripts/setup_vm.sh --help` 查看部署脚本的所有选项

---

**最后更新**: 2024  
**项目状态**: 虚拟机环境功能完整 ✅
