# 第一步：环境准备与配置

## 📋 任务概览
完成开发环境的配置，验证硬件支持，安装必要的工具链和依赖。

---

## ✅ 检查清单

### 1. 硬件能力验证

#### 1.1 检查CPU性能监控单元(PMU)支持
```bash
# 检查LLC Miss事件支持
perf list | grep -i "mem_load_retired.l3_miss"

# 检查LBR(Last Branch Record)支持
perf list | grep -i "lbr"

# 检查PEBS支持
perf list | grep -i "pebs"

# 查看CPU详细信息
lscpu | grep -E "Model name|Architecture"
cat /proc/cpuinfo | grep -E "model name|flags" | head -20
```

**预期输出**:
- 应该能看到 `mem_load_retired.l3_miss` 或类似事件
- Intel CPU应该在flags中包含 `pebs`, `pdcm` 等标志

#### 1.2 检查当前内核配置
```bash
# 内核版本(建议5.10+)
uname -r

# 检查eBPF支持
cat /boot/config-$(uname -r) | grep -E "CONFIG_BPF|CONFIG_KPROBES"

# 检查大页支持
grep Huge /proc/meminfo
cat /proc/sys/vm/nr_hugepages
```

---

### 2. 权限配置

#### 2.1 配置perf_event权限
```bash
# 查看当前设置
cat /proc/sys/kernel/perf_event_paranoid

# 临时修改(需要root权限)
sudo sysctl -w kernel.perf_event_paranoid=-1

# 永久修改
echo "kernel.perf_event_paranoid = -1" | sudo tee -a /etc/sysctl.conf
sudo sysctl -p
```

**参数说明**:
- `2`: 仅允许用户态计数
- `1`: 允许内核分析，但需要CAP_PERFMON
- `0`: 允许原始跟踪点访问
- `-1`: 允许所有性能事件(推荐开发时使用)

#### 2.2 配置eBPF权限(如果非root用户)
```bash
# 检查当前用户组
groups

# 添加到必要的组(可能需要)
sudo usermod -a -G sudo $USER
sudo usermod -a -G adm $USER

# 检查unprivileged_bpf_disabled
cat /proc/sys/kernel/unprivileged_bpf_disabled

# 如果为1, 可以临时允许(仅用于开发)
sudo sysctl -w kernel.unprivileged_bpf_disabled=0
```

---

### 3. 安装依赖工具

#### 3.1 基础开发工具
```bash
# 更新包管理器
sudo apt update

# 安装C++编译器和CMake
sudo apt install -y \
  build-essential \
  cmake \
  ninja-build \
  pkg-config \
  git

# 验证安装
gcc --version    # 建议GCC 9+
g++ --version
cmake --version  # 建议CMake 3.20+
```

#### 3.2 性能分析工具
```bash
# 安装Linux perf工具
sudo apt install -y \
  linux-tools-common \
  linux-tools-generic \
  linux-tools-$(uname -r)

# 验证perf安装
perf --version

# 测试perf基本功能
perf stat ls
```

#### 3.3 eBPF工具链
```bash
# 安装libbpf和相关开发包
sudo apt install -y \
  libbpf-dev \
  libelf-dev \
  zlib1g-dev \
  clang \
  llvm \
  bpftool

# 验证安装
bpftool --version
clang --version  # 建议Clang 10+

# 可选: 安装BCC工具集(用于快速原型)
sudo apt install -y \
  bpfcc-tools \
  python3-bpfcc
```

#### 3.4 其他依赖库
```bash
# JSON解析库
sudo apt install -y nlohmann-json3-dev

# Boost库(用于图算法, 可选)
sudo apt install -y libboost-all-dev

# Google Test(用于单元测试, 可选)
sudo apt install -y libgtest-dev

# Google Benchmark(用于性能测试, 可选)
sudo apt install -y libbenchmark-dev
```

---

### 4. 创建项目目录结构

```bash
cd /home/xiaowu/work/CXLMemSim

# 创建目录结构
mkdir -p backend/ebpf
mkdir -p backend/include/{tracer,analyzer,injector,common}
mkdir -p backend/src/{tracer,analyzer,injector}
mkdir -p frontend/components
mkdir -p common/{include,src}
mkdir -p tests/{unit,integration,benchmarks}
mkdir -p docs/{design,api,user_guide}
mkdir -p configs
mkdir -p scripts
mkdir -p third_party

# 创建基础文件
touch README.md
touch .gitignore
touch CMakeLists.txt
```

#### 4.1 创建.gitignore文件
```bash
cat > /home/xiaowu/work/CXLMemSim/.gitignore << 'EOF'
# Build artifacts
build/
build-*/
cmake-build-*/
*.o
*.a
*.so
*.dylib

# IDE
.vscode/
.idea/
*.swp
*.swo
*~

# BPF compiled objects
*.bpf.o
vmlinux.h

# Test outputs
test_results/
*.log

# Perf data
perf.data
perf.data.old

# User config
local_config.json
EOF
```

#### 4.2 创建初始README
```bash
cat > /home/xiaowu/work/CXLMemSim/README.md << 'EOF'
# CXLMemSim - CXL Memory Simulator

A high-fidelity, execution-driven CXL memory simulator using eBPF and hardware performance counters.

## Features
- PEBS/LBR-based precise memory access tracking
- eBPF kernel-level memory allocation tracing
- Multi-tier topology modeling (switches, fabrics)
- Bandwidth and latency simulation
- CXL.mem, CXL.cache, CXL.io protocol modeling

## Status
🚧 Under active development - Phase 1: Infrastructure Setup

## Build Requirements
- Linux Kernel 5.10+
- GCC 9+ or Clang 10+
- CMake 3.20+
- libbpf-dev
- Intel CPU with PEBS/LBR support

## Quick Start
Coming soon...

## Documentation
See `docs/` directory and `planning/execution_plan.md`

## License
TBD
EOF
```

---

### 5. 验证环境完整性

#### 5.1 创建验证脚本
```bash
cat > /home/xiaowu/work/CXLMemSim/scripts/verify_environment.sh << 'EOF'
#!/bin/bash

echo "================================"
echo "CXLMemSim Environment Validator"
echo "================================"
echo

# Color codes
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

check_command() {
    if command -v $1 &> /dev/null; then
        echo -e "${GREEN}✓${NC} $1 is installed"
        return 0
    else
        echo -e "${RED}✗${NC} $1 is NOT installed"
        return 1
    fi
}

check_kernel_config() {
    local config=$1
    if grep -q "^${config}=y" /boot/config-$(uname -r) 2>/dev/null; then
        echo -e "${GREEN}✓${NC} Kernel config: $config"
        return 0
    else
        echo -e "${YELLOW}⚠${NC} Kernel config: $config (not found or not enabled)"
        return 1
    fi
}

# 1. Check basic tools
echo "1. Checking basic development tools..."
check_command gcc
check_command g++
check_command cmake
check_command make
echo

# 2. Check perf tools
echo "2. Checking perf tools..."
check_command perf
perf_paranoid=$(cat /proc/sys/kernel/perf_event_paranoid)
if [ "$perf_paranoid" -le 1 ]; then
    echo -e "${GREEN}✓${NC} perf_event_paranoid = $perf_paranoid (OK)"
else
    echo -e "${RED}✗${NC} perf_event_paranoid = $perf_paranoid (should be <=1)"
fi
echo

# 3. Check eBPF tools
echo "3. Checking eBPF toolchain..."
check_command clang
check_command llvm-strip
check_command bpftool
check_kernel_config CONFIG_BPF
check_kernel_config CONFIG_BPF_SYSCALL
echo

# 4. Check PMU events
echo "4. Checking CPU PMU support..."
if perf list | grep -q "mem_load.*l3_miss"; then
    echo -e "${GREEN}✓${NC} LLC Miss events available"
else
    echo -e "${RED}✗${NC} LLC Miss events NOT found"
fi

if perf list | grep -q -i "lbr"; then
    echo -e "${GREEN}✓${NC} LBR events available"
else
    echo -e "${YELLOW}⚠${NC} LBR events NOT found (may not be critical)"
fi
echo

# 5. Check libraries
echo "5. Checking development libraries..."
check_command pkg-config

if pkg-config --exists libbpf; then
    echo -e "${GREEN}✓${NC} libbpf development package"
else
    echo -e "${RED}✗${NC} libbpf development package NOT found"
fi

if [ -f /usr/include/nlohmann/json.hpp ]; then
    echo -e "${GREEN}✓${NC} nlohmann-json header found"
else
    echo -e "${YELLOW}⚠${NC} nlohmann-json header NOT found (can be installed later)"
fi
echo

# 6. Summary
echo "================================"
echo "Verification Complete"
echo "================================"
echo
echo "If you see any RED ✗ items above, please install missing dependencies."
echo "Refer to planning/step1_environment_setup.md for detailed instructions."
EOF

chmod +x /home/xiaowu/work/CXLMemSim/scripts/verify_environment.sh
```

#### 5.2 运行验证
```bash
cd /home/xiaowu/work/CXLMemSim
./scripts/verify_environment.sh
```

---

### 6. 测试基础功能

#### 6.1 测试perf基本采样
```bash
# 创建简单的测试程序
cat > /tmp/test_mem.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE (100 * 1024 * 1024) // 100MB

int main() {
    char *buffer = malloc(SIZE);
    if (!buffer) return 1;

    // Generate many cache misses
    for (int i = 0; i < 10; i++) {
        for (size_t j = 0; j < SIZE; j += 4096) {
            buffer[j] = (char)j;
        }
    }

    free(buffer);
    printf("Memory test completed\n");
    return 0;
}
EOF

gcc -O2 /tmp/test_mem.c -o /tmp/test_mem

# 使用perf采样LLC Miss
perf stat -e LLC-load-misses,LLC-loads /tmp/test_mem

# 使用perf record记录详细信息
perf record -e mem_load_retired.l3_miss:pp /tmp/test_mem
perf report
```

#### 6.2 测试简单的eBPF程序
```bash
# 如果安装了bcc-tools, 测试基础tracepoint
sudo trace-bpfcc 't:syscalls:sys_enter_openat "%s", args->filename' &
TRACE_PID=$!
sleep 1
ls /tmp
sudo kill $TRACE_PID
```

---

## 📝 完成标准

完成本阶段后，你应该能够:
- [ ] `perf list` 能列出内存相关事件
- [ ] `perf stat` 能统计LLC Miss
- [ ] `bpftool` 命令可用
- [ ] `/proc/sys/kernel/perf_event_paranoid` ≤ 1
- [ ] 项目目录结构已创建
- [ ] 验证脚本全部通过(无红色✗)

---

## 🐛 常见问题

### Q1: perf命令找不到?
```bash
# 确保安装了对应内核版本的工具
sudo apt install linux-tools-$(uname -r)
```

### Q2: perf list显示事件很少?
```bash
# 检查是否有权限限制
cat /proc/sys/kernel/perf_event_paranoid
# 如果>1, 参考权限配置部分
```

### Q3: bpftool命令找不到?
```bash
# Ubuntu 20.04+
sudo apt install linux-tools-common linux-tools-generic

# 或从源码编译
cd /tmp
git clone https://github.com/libbpf/bpftool.git
cd bpftool/src
make
sudo make install
```

### Q4: CPU不支持PEBS?
如果CPU不支持PEBS(通常是AMD或较老的Intel CPU):
- 可以使用软件模拟: `perf record -e cache-misses`
- 但精度会降低, 无法获取精确的数据地址
- 建议在Intel Skylake或更新的CPU上开发

---

## 📚 参考资料

- [Linux Perf Wiki](https://perf.wiki.kernel.org/)
- [BPF和XDP参考指南](https://cilium.readthedocs.io/en/stable/bpf/)
- [libbpf Documentation](https://libbpf.readthedocs.io/)
- [Intel PEBS Guide](https://www.intel.com/content/www/us/en/docs/vtune-profiler/cookbook/current/precise-event-based-sampling.html)

---

## ✅ 下一步

完成环境配置后, 进入 **任务1.2: Tracer模块开发**
- 开始编写perf_event_open封装代码
- 实现PEBS采样数据读取
