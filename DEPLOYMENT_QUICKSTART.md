# CXLMemSim 物理机快速部署指南 🚀

> 从虚拟机开发环境迁移到物理服务器的快速入门指南

---

## 📋 前置检查清单

在物理服务器上部署前，请确认:

- ✅ **Intel CPU** (Skylake 或更新架构)
- ✅ **Linux Kernel** >= 5.10
- ✅ **Root 权限** (用于安装依赖和配置)
- ✅ **网络连接** (下载软件包)
- ✅ **磁盘空间** >= 5GB

---

## ⚡ 三步快速部署

### 方法 1: Git Clone (推荐)

```bash
# 1️⃣ 克隆代码到物理机
git clone <your-repo-url> /opt/CXLMemSim
cd /opt/CXLMemSim

# 2️⃣ 一键自动部署
sudo ./scripts/deploy_physical.sh --auto

# 3️⃣ 验证安装
./build/cxlmemsim --check-tracer
```

**预期输出**:
```
PEBS Support: ✓ Available
Best Tracer: PEBS
Tracer Info:
  Name: PEBSTracer
  Capabilities: Precise Event-Based Sampling with data address
  Precise Address: Yes
```

### 方法 2: rsync 同步

如果无法使用 Git，可以从虚拟机直接同步:

```bash
# 在虚拟机上执行
rsync -avz --exclude 'build' --exclude '.git' \
    /home/xiaowu/work/CXLMemSim/ \
    user@physical-server:/opt/CXLMemSim/

# 在物理服务器上执行
cd /opt/CXLMemSim
sudo ./scripts/deploy_physical.sh --auto
```

---

## 🛠️ 手动部署 (逐步执行)

如果需要更多控制，可以手动执行各个步骤:

### 步骤 1: 安装依赖

```bash
cd /opt/CXLMemSim
sudo ./scripts/install_dependencies.sh
```

或手动安装:

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y \
    build-essential cmake git \
    linux-tools-generic \
    nlohmann-json3-dev libgtest-dev \
    qt6-base-dev qt6-tools-dev libqt6charts6-dev
```

### 步骤 2: 配置权限

```bash
# 允许普通用户使用 perf
sudo sysctl -w kernel.perf_event_paranoid=1
sudo sysctl -w kernel.kptr_restrict=0

# 永久保存
echo "kernel.perf_event_paranoid=1" | sudo tee -a /etc/sysctl.conf
echo "kernel.kptr_restrict=0" | sudo tee -a /etc/sysctl.conf
```

### 步骤 3: 编译项目

```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### 步骤 4: 验证

```bash
# 检查 Tracer 类型
./cxlmemsim --check-tracer

# 运行单元测试
ctest --output-on-failure

# 采集测试 trace
./cxlmemsim --mode=trace --target=/bin/ls --duration=5
```

---

## 🔍 故障排查

### 问题 1: "PEBS not available"

**原因**: 虚拟机环境或不支持的 CPU

**解决**:
- 确认运行在物理机: `systemd-detect-virt` (应输出 `none`)
- 确认 CPU 型号: `lscpu | grep "Model name"`
- 查看 perf 事件: `perf list | grep mem_load_retired`

### 问题 2: "Permission denied"

**原因**: perf 权限不足

**解决**:
```bash
# 检查当前值
cat /proc/sys/kernel/perf_event_paranoid

# 应该 <= 1，如果不是则设置:
sudo sysctl -w kernel.perf_event_paranoid=1
```

### 问题 3: Qt6 找不到

**原因**: Qt6 未安装

**解决**:
```bash
# 安装 Qt6
sudo apt install qt6-base-dev qt6-tools-dev

# 或禁用 GUI 编译
cmake -DBUILD_GUI=OFF ..
make
```

### 问题 4: 编译错误

**诊断信息收集**:
```bash
# 生成完整诊断报告
./scripts/collect_diagnostic_info.sh > diagnostic.txt

# 查看报告
less diagnostic.txt
```

---

## 📊 验证测试

部署成功后，运行以下测试确认功能正常:

```bash
cd /opt/CXLMemSim/build

# 1. Tracer 检测 (应显示 PEBS)
./cxlmemsim --check-tracer

# 2. Mock Tracer 测试
./cxlmemsim --test-mock

# 3. 单元测试
ctest --verbose

# 4. 真实 trace 采集
./cxlmemsim --mode=trace \
    --target=/usr/bin/stress-ng \
    --target-args="--vm 1 --vm-bytes 256M --timeout 10s" \
    --output=test_trace.json \
    --duration=10

# 5. 检查采集结果
ls -lh test_trace.json
jq '.events | length' test_trace.json  # 应显示事件数量
```

---

## 🎯 性能对比测试

对比原生运行和模拟运行的性能:

```bash
# 原生运行
time /path/to/benchmark

# CXL 模拟运行
time ./cxlmemsim --simulate \
    --config=configs/examples/simple_cxl.json \
    --target=/path/to/benchmark

# 预期开销: 3-6x slowdown
```

---

## 📁 部署后的目录结构

```
/opt/CXLMemSim/
├── build/
│   ├── cxlmemsim          # 主程序
│   ├── simple_simulation   # 演示程序
│   └── tests/              # 测试程序
├── configs/
│   └── examples/
│       └── simple_cxl.json # 配置示例
├── logs/
│   ├── deployment_*.log    # 部署日志
│   └── tests_*.log         # 测试日志
└── docs/
    └── PHYSICAL_DEPLOYMENT.md  # 完整部署文档
```

---

## 🔄 持续开发流程

### 在虚拟机开发

```bash
# 虚拟机中
cd /home/xiaowu/work/CXLMemSim

# 1. 开发新功能
vim backend/src/...

# 2. 测试 (使用 MockTracer)
mkdir build && cd build
cmake .. && make
./cxlmemsim --test-mock

# 3. 提交代码
git add .
git commit -m "Add new feature"
git push origin main
```

### 在物理机测试

```bash
# 物理机上
cd /opt/CXLMemSim

# 1. 拉取最新代码
git pull origin main

# 2. 重新编译
cd build
make -j$(nproc)

# 3. 运行真实测试 (使用 PEBSTracer)
./cxlmemsim --mode=trace --target=/path/to/app --duration=30

# 4. 收集真实 trace 数据
cp results/*.json /tmp/real_traces/

# 5. 传回虚拟机用于开发
scp /tmp/real_traces/*.json user@vm-host:/home/xiaowu/work/CXLMemSim/tests/data/
```

---

## 📝 常用命令速查

```bash
# 检测 PEBS 支持
./build/cxlmemsim --check-tracer

# 采集 trace
./build/cxlmemsim --mode=trace --target=<program> --duration=<seconds>

# 启动 GUI
./build/cxlmemsim

# 运行模拟
./build/cxlmemsim --simulate --config=<config.json> --target=<program>

# 查看 perf 权限
cat /proc/sys/kernel/perf_event_paranoid

# 设置 perf 权限
sudo sysctl -w kernel.perf_event_paranoid=1

# 生成诊断报告
./scripts/collect_diagnostic_info.sh > diagnostic.txt

# 重新部署
sudo ./scripts/deploy_physical.sh --auto
```

---

## 📚 更多文档

- **完整部署文档**: [docs/PHYSICAL_DEPLOYMENT.md](docs/PHYSICAL_DEPLOYMENT.md)
- **用户手册**: [docs/user_guide/README.md](docs/user_guide/README.md)
- **API 文档**: [docs/api/README.md](docs/api/README.md)
- **架构设计**: [planning/architecture_updates.md](planning/architecture_updates.md)

---

## 💬 获取帮助

### 自助诊断

```bash
# 生成诊断报告并查看
./scripts/collect_diagnostic_info.sh | less

# 查看部署日志
tail -f logs/deployment_*.log
```

### 社区支持

- 提交 Issue: 附上诊断报告
- 邮件联系: 包含系统信息和错误日志

---

## ✅ 成功标志

部署成功的标志:

- ✅ `./cxlmemsim --check-tracer` 显示 "PEBS"
- ✅ 单元测试 `ctest` 全部通过
- ✅ 成功采集到真实 trace 数据
- ✅ GUI 正常启动并加载配置
- ✅ 模拟运行开销 < 10x

---

## 🎉 下一步

部署成功后，您可以:

1. **运行基准测试**: 使用 SPEC CPU2017 或自定义程序
2. **配置 CXL 拓扑**: 编辑 `configs/examples/simple_cxl.json`
3. **采集真实 trace**: 为您的应用生成内存访问模式
4. **分析性能**: 使用 GUI 查看延迟和带宽指标

---

**快速入门完成！🚀**

更多详情请参考 [完整部署文档](docs/PHYSICAL_DEPLOYMENT.md)。
