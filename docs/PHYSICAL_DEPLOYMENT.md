# CXLMemSim 物理服务器部署指南

## 📋 目录
- [系统要求](#系统要求)
- [硬件要求](#硬件要求)
- [快速部署](#快速部署)
- [手动部署](#手动部署)
- [权限配置](#权限配置)
- [验证测试](#验证测试)
- [性能调优](#性能调优)
- [故障排查](#故障排查)

---

## 🖥️ 系统要求

### 操作系统
- **推荐**: Ubuntu 22.04 LTS 或更高版本
- **支持**: 其他 Linux 发行版 (需要 Linux Kernel 5.10+)
- **架构**: x86_64

### 软件依赖
| 组件 | 最低版本 | 推荐版本 | 说明 |
|------|---------|---------|------|
| Linux Kernel | 5.10 | 6.0+ | 需要 PEBS 支持 |
| GCC | 9.0 | 11.0+ | 支持 C++17 |
| CMake | 3.20 | 3.25+ | 构建系统 |
| Qt6 | 6.2 | 6.5+ | GUI 框架 |
| perf | 5.10 | 最新 | 性能监控工具 |
| nlohmann-json | 3.0 | 3.10+ | JSON 解析库 |
| Google Test | 1.10 | 1.12+ | 单元测试框架 |

---

## 🔧 硬件要求

### 必需硬件特性
✅ **Intel CPU** (支持 PEBS - Precise Event-Based Sampling)
  - Intel Xeon (Skylake 或更新)
  - Intel Core (第 6 代或更新)
  - 支持的事件: `MEM_LOAD_RETIRED.L3_MISS`, `MEM_INST_RETIRED.ALL_LOADS`

✅ **内存配置**
  - 最低: 8GB RAM
  - 推荐: 16GB+ RAM
  - 支持 CXL 设备 (可选)

### 硬件检测
运行以下命令检查 CPU 是否支持 PEBS:

```bash
# 检查 CPU 型号
cat /proc/cpuinfo | grep "model name" | head -1

# 检查 PMU 功能
dmesg | grep -i "Performance monitoring"

# 测试 perf 事件
perf list | grep -E "mem.*retired|mem.*load"
```

**预期输出示例**:
```
  mem_load_retired.l3_miss                           [Hardware event]
  mem_inst_retired.all_loads                         [Hardware event]
```

---

## 🚀 快速部署

### 一键部署脚本
在物理服务器上运行:

```bash
# 方法1: 从 Git 仓库克隆 (推荐)
git clone <your-repo-url> /opt/CXLMemSim
cd /opt/CXLMemSim
sudo ./scripts/deploy_physical.sh --auto

# 方法2: 从虚拟机同步代码
rsync -avz --exclude 'build' \
    user@vm-host:/home/xiaowu/work/CXLMemSim/ \
    /opt/CXLMemSim/
cd /opt/CXLMemSim
sudo ./scripts/deploy_physical.sh --auto
```

### 部署脚本功能
- ✅ 自动检测硬件支持
- ✅ 安装所有依赖项
- ✅ 配置 perf 权限
- ✅ 编译项目 (Release 模式)
- ✅ 运行验证测试
- ✅ 生成部署报告

---

## 📦 手动部署步骤

### 步骤 1: 安装依赖

#### Ubuntu/Debian
```bash
# 更新软件源
sudo apt update

# 安装编译工具链
sudo apt install -y \
    build-essential \
    cmake \
    git \
    pkg-config

# 安装性能监控工具
sudo apt install -y \
    linux-tools-common \
    linux-tools-generic \
    linux-tools-$(uname -r)

# 安装开发库
sudo apt install -y \
    nlohmann-json3-dev \
    libgtest-dev \
    libgmock-dev

# 安装 Qt6 (GUI 支持)
sudo apt install -y \
    qt6-base-dev \
    qt6-tools-dev \
    qt6-tools-dev-tools \
    libqt6charts6-dev \
    libqt6svg6-dev
```

#### RHEL/CentOS/Fedora
```bash
# 安装编译工具链
sudo dnf groupinstall -y "Development Tools"
sudo dnf install -y cmake git

# 安装性能监控工具
sudo dnf install -y perf

# 安装开发库
sudo dnf install -y \
    json-devel \
    gtest-devel \
    gmock-devel

# 安装 Qt6
sudo dnf install -y \
    qt6-qtbase-devel \
    qt6-qttools-devel \
    qt6-qtcharts-devel
```

### 步骤 2: 克隆代码

```bash
# 创建工作目录
sudo mkdir -p /opt/CXLMemSim
sudo chown $USER:$USER /opt/CXLMemSim

# 克隆代码
cd /opt
git clone <your-repo-url> CXLMemSim
cd CXLMemSim

# 或从虚拟机同步
rsync -avz --exclude 'build' --exclude '.git' \
    user@vm-host:/home/xiaowu/work/CXLMemSim/ \
    /opt/CXLMemSim/
```

### 步骤 3: 编译项目

```bash
# 创建构建目录
mkdir -p build && cd build

# 配置 CMake (Release 模式)
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_GUI=ON \
      -DBUILD_TESTS=ON \
      ..

# 编译 (使用所有 CPU 核心)
make -j$(nproc)

# 可选: 安装到系统路径
sudo make install
```

### 步骤 4: 配置权限

#### 配置 perf 权限 (推荐方法)

```bash
# 方法1: 允许普通用户使用 perf (推荐)
sudo sysctl -w kernel.perf_event_paranoid=1
sudo sysctl -w kernel.kptr_restrict=0

# 永久生效
echo "kernel.perf_event_paranoid=1" | sudo tee -a /etc/sysctl.conf
echo "kernel.kptr_restrict=0" | sudo tee -a /etc/sysctl.conf

# 方法2: 使用 CAP_PERFMON 能力 (Kernel 5.8+)
sudo setcap cap_perfmon=ep ./build/cxlmemsim
```

#### 权限级别说明

| paranoid 值 | 权限 | 说明 |
|------------|------|------|
| -1 | 全部允许 | **不推荐**: 安全风险 |
| 0 | CPU 事件 + 内核跟踪 | 需要 PEBS 采样 |
| 1 | CPU 事件 (排除内核) | **推荐**: 安全且够用 |
| 2 | 仅用户空间 | 默认值，PEBS 不可用 |
| 3 | 完全禁止 | 无法使用 perf |

### 步骤 5: 验证部署

```bash
# 检查 Tracer 类型
./build/cxlmemsim --check-tracer

# 预期输出 (物理机):
# PEBS Support: ✓ Available
# Best Tracer: PEBS
# Tracer Info:
#   Name: PEBSTracer
#   Capabilities: Precise Event-Based Sampling with data address
#   Precise Address: Yes

# 运行单元测试
cd build
ctest --output-on-failure

# 运行集成测试
./build/simple_simulation
```

---

## ✅ 验证测试

### 测试 1: PEBS 可用性检测

```bash
./build/cxlmemsim --check-tracer
```

**成功标志**:
- ✅ "PEBS Support: ✓ Available"
- ✅ "Best Tracer: PEBS"

**失败处理**:
- ❌ "PEBS not available" → 检查 CPU 型号和内核版本
- ❌ "Permission denied" → 检查 perf_event_paranoid 设置

### 测试 2: 采集真实 Trace

```bash
# 采集系统命令的内存访问
./build/cxlmemsim --mode=trace \
    --target=/bin/ls \
    --target-args="-lah /usr" \
    --output=test_trace.json \
    --duration=5

# 检查输出文件
ls -lh test_trace.json
jq '.events | length' test_trace.json
```

**预期结果**:
- 文件大小 > 10KB
- 包含数百到数千个内存访问事件

### 测试 3: GUI 功能测试

```bash
# 启动 GUI (如果编译了 Qt 前端)
./build/cxlmemsim

# 测试功能:
# 1. 加载配置文件 (configs/examples/simple_cxl.json)
# 2. 编辑拓扑图
# 3. 启动模拟
# 4. 查看实时指标
```

### 测试 4: 性能基准测试

```bash
# 运行 SPEC CPU2017 或自定义基准程序
./build/cxlmemsim --mode=simulate \
    --config=configs/benchmarks/spec_gcc.json \
    --target=/path/to/spec/gcc \
    --output=results/spec_gcc_cxl.json

# 对比原生性能
time /path/to/spec/gcc <args>
# vs
time ./build/cxlmemsim --simulate /path/to/spec/gcc <args>

# 预期开销: 3-6x slowdown
```

---

## ⚡ 性能调优

### CPU 频率缩放 (禁用节能模式)

```bash
# 设置为 performance 模式
sudo cpupower frequency-set -g performance

# 验证
cpupower frequency-info
```

### NUMA 配置 (多路服务器)

```bash
# 查看 NUMA 拓扑
numactl --hardware

# 绑定到特定 NUMA 节点运行
numactl --cpunodebind=0 --membind=0 ./build/cxlmemsim <args>
```

### PEBS 采样频率调整

编辑配置文件 `configs/pebs_config.json`:

```json
{
  "pebs": {
    "sample_period": 10000,  // 每 10000 次 L3 Miss 采样一次
    "event": "MEM_LOAD_RETIRED.L3_MISS",
    "precise_ip": 2,  // 最高精度
    "buffer_size_mb": 64  // 增加缓冲区减少丢失
  }
}
```

**调优建议**:
- `sample_period`: 越小越精确，但开销越大 (推荐: 1000-100000)
- `buffer_size_mb`: 高负载时增大 (推荐: 32-128 MB)

### 系统调优参数

```bash
# /etc/sysctl.conf 添加:
kernel.perf_event_max_sample_rate=100000  # 最大采样率
kernel.perf_event_mlock_kb=524288         # 512MB 锁定内存
```

---

## 🔍 故障排查

### 问题 1: PEBS 检测失败

**症状**:
```
PEBS Support: ✗ Not available
errno: 95 (Operation not supported)
```

**解决方法**:

1. **检查 CPU 支持**:
```bash
# Intel CPU 型号检查
lscpu | grep "Model name"

# 必须是 Intel Skylake 或更新架构
# AMD CPU 不支持 PEBS (使用 IBS - Instruction-Based Sampling)
```

2. **检查内核版本**:
```bash
uname -r
# 需要 >= 5.10
```

3. **检查内核配置**:
```bash
grep CONFIG_PERF_EVENTS /boot/config-$(uname -r)
# 应输出: CONFIG_PERF_EVENTS=y
```

4. **检查虚拟化**:
```bash
systemd-detect-virt
# 如果输出非 "none", 则在虚拟机中 (PEBS 不可用)
```

### 问题 2: 权限错误

**症状**:
```
perf_event_open: Permission denied (errno=1)
```

**解决方法**:
```bash
# 1. 检查当前设置
cat /proc/sys/kernel/perf_event_paranoid

# 2. 临时修改
sudo sysctl -w kernel.perf_event_paranoid=1

# 3. 永久修改
echo "kernel.perf_event_paranoid=1" | sudo tee -a /etc/sysctl.conf
sudo sysctl -p

# 4. 或使用 sudo 运行
sudo ./build/cxlmemsim <args>
```

### 问题 3: Qt 库找不到

**症状**:
```
CMake Error: Could not find Qt6Core
```

**解决方法**:
```bash
# Ubuntu/Debian
sudo apt install qt6-base-dev qt6-tools-dev

# 或禁用 GUI 编译
cmake -DBUILD_GUI=OFF ..
make
```

### 问题 4: 采样数据为空

**症状**:
```json
{
  "events": [],
  "total_samples": 0
}
```

**可能原因**:

1. **目标程序运行时间太短**:
```bash
# 增加运行时间或使用长时间运行的程序
./build/cxlmemsim --target=/usr/bin/stress --duration=30
```

2. **L3 Miss 率太低**:
```bash
# 使用内存密集型程序
./build/cxlmemsim --target=/path/to/memory-intensive-app
```

3. **采样周期太大**:
```bash
# 编辑 configs/pebs_config.json
"sample_period": 1000  # 降低到 1000
```

### 问题 5: 编译错误

**症状**:
```
fatal error: nlohmann/json.hpp: No such file or directory
```

**解决方法**:
```bash
# 安装缺失的开发库
sudo apt install nlohmann-json3-dev

# 或使用系统包含路径
cmake -DCMAKE_CXX_FLAGS="-I/usr/include/nlohmann" ..
```

---

## 📊 部署验证清单

完成以下清单确保部署成功:

### 硬件检查
- [ ] CPU 型号确认为 Intel (Skylake 或更新)
- [ ] `perf list` 显示 `mem_load_retired.l3_miss` 事件
- [ ] 内存 >= 8GB

### 软件环境
- [ ] Linux Kernel >= 5.10
- [ ] GCC >= 9.0
- [ ] CMake >= 3.20
- [ ] Qt6 已安装 (如需 GUI)

### 编译验证
- [ ] `cmake ..` 成功且显示 "PEBS support detected"
- [ ] `make -j$(nproc)` 无错误
- [ ] 生成可执行文件 `build/cxlmemsim`

### 权限配置
- [ ] `kernel.perf_event_paranoid` 设置为 1 或 0
- [ ] `kernel.kptr_restrict` 设置为 0

### 功能测试
- [ ] `./build/cxlmemsim --check-tracer` 输出 "PEBS"
- [ ] 单元测试 `ctest` 全部通过
- [ ] 成功采集到真实 trace 数据
- [ ] GUI 启动并正常显示 (如适用)

### 性能测试
- [ ] 运行基准程序并记录开销 (应 < 10x)
- [ ] 长时间运行无崩溃 (> 1 小时)

---

## 📞 支持与反馈

### 文档资源
- 架构设计: [docs/design/architecture.md](../design/architecture.md)
- API 文档: [docs/api/README.md](../api/README.md)
- 用户手册: [docs/user_guide/README.md](../user_guide/README.md)

### 日志收集
遇到问题时，请提供以下信息:

```bash
# 生成诊断报告
./scripts/collect_diagnostic_info.sh > diagnostic_report.txt

# 报告包含:
# - 硬件信息 (CPU, 内存)
# - 软件版本 (内核, GCC, CMake, Qt)
# - perf 配置
# - 编译日志
# - 测试结果
```

### 常见问题
参考 [FAQ.md](FAQ.md) 获取更多问题解答。

---

## 📝 部署示例

### 示例 1: 标准部署 (单节点服务器)

```bash
# 1. 克隆代码
git clone https://github.com/yourname/CXLMemSim.git /opt/CXLMemSim
cd /opt/CXLMemSim

# 2. 自动部署
sudo ./scripts/deploy_physical.sh --auto

# 3. 验证
./build/cxlmemsim --check-tracer
./build/cxlmemsim --test-mock
./build/cxlmemsim --mode=trace --target=/bin/ls --duration=5

# 4. 运行 GUI
./build/cxlmemsim
```

### 示例 2: 多节点集群部署

```bash
# 在管理节点:
for node in node01 node02 node03; do
    ssh $node "mkdir -p /opt/CXLMemSim"
    rsync -avz /opt/CXLMemSim/ $node:/opt/CXLMemSim/
    ssh $node "cd /opt/CXLMemSim && sudo ./scripts/deploy_physical.sh --auto"
done

# 验证所有节点
for node in node01 node02 node03; do
    echo "=== $node ==="
    ssh $node "/opt/CXLMemSim/build/cxlmemsim --check-tracer"
done
```

### 示例 3: 容器化部署 (Docker)

```dockerfile
# Dockerfile
FROM ubuntu:22.04

RUN apt update && apt install -y \
    build-essential cmake git \
    linux-tools-generic \
    nlohmann-json3-dev libgtest-dev \
    qt6-base-dev qt6-tools-dev

COPY . /app/CXLMemSim
WORKDIR /app/CXLMemSim

RUN mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc)

# 注意: Docker 容器内 PEBS 可能不可用
# 需要 --privileged 模式和 host 网络
CMD ["/app/CXLMemSim/build/cxlmemsim", "--check-tracer"]
```

```bash
# 构建和运行
docker build -t cxlmemsim:latest .
docker run --privileged --network=host cxlmemsim:latest
```

---

## 🔄 更新和维护

### 从虚拟机拉取最新代码

```bash
# 在物理服务器上:
cd /opt/CXLMemSim

# Git 同步
git pull origin main

# 重新编译
cd build
cmake ..
make -j$(nproc)

# 或使用 rsync 增量同步
rsync -avz --delete \
    --exclude 'build' --exclude '.git' \
    user@vm-host:/home/xiaowu/work/CXLMemSim/ \
    /opt/CXLMemSim/
```

### 定期维护任务

```bash
# 每周:
# 1. 更新系统软件包
sudo apt update && sudo apt upgrade

# 2. 清理旧的 trace 文件
find /opt/CXLMemSim/results -name "*.json" -mtime +30 -delete

# 3. 检查日志大小
du -sh /opt/CXLMemSim/logs

# 每月:
# 4. 运行完整测试套件
cd /opt/CXLMemSim/build
ctest --verbose
```

---

## 🎯 下一步

部署完成后，您可以:

1. **运行实验**: 参考 [docs/user_guide/experiments.md](../user_guide/experiments.md)
2. **配置拓扑**: 参考 [docs/user_guide/topology_config.md](../user_guide/topology_config.md)
3. **分析结果**: 参考 [docs/user_guide/result_analysis.md](../user_guide/result_analysis.md)
4. **开发扩展**: 参考 [docs/api/plugin_development.md](../api/plugin_development.md)

---

**文档版本**: 1.0.0
**最后更新**: 2025-12-11
**维护者**: CXLMemSim Team
