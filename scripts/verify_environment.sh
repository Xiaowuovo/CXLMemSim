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

if pkg-config --exists libbpf 2>/dev/null; then
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
