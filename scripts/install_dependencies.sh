#!/bin/bash

# CXLMemSim 依赖安装脚本
# 需要sudo权限执行

echo "正在安装CXLMemSim依赖..."

# 更新包管理器
sudo apt update

# 安装编译工具链
echo "安装编译工具链..."
sudo apt install -y \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    git

# 安装Clang和LLVM
echo "安装Clang和LLVM..."
sudo apt install -y \
    clang \
    llvm \
    llvm-dev

# 安装eBPF相关工具
echo "安装eBPF工具..."
sudo apt install -y \
    libbpf-dev \
    libelf-dev \
    zlib1g-dev \
    bpftool

# 安装性能分析工具
echo "安装Perf工具..."
sudo apt install -y \
    linux-tools-common \
    linux-tools-generic \
    linux-tools-$(uname -r)

# 安装开发库
echo "安装开发库..."
sudo apt install -y \
    nlohmann-json3-dev \
    libboost-all-dev

# 可选：安装测试框架
echo "安装测试框架（可选）..."
sudo apt install -y \
    libgtest-dev \
    libbenchmark-dev

echo ""
echo "依赖安装完成！"
echo ""
echo "接下来需要配置perf权限："
echo "sudo sysctl -w kernel.perf_event_paranoid=-1"
echo ""
echo "验证安装："
echo "./scripts/verify_environment.sh"
