#!/bin/bash

################################################################################
# CXLMemSim 诊断信息收集脚本
#
# 用途: 收集系统信息用于故障排查
# 用法: ./scripts/collect_diagnostic_info.sh > diagnostic_report.txt
#
################################################################################

set -e

cat << "EOF"
================================================================================
CXLMemSim 诊断报告
================================================================================

生成时间: $(date '+%Y-%m-%d %H:%M:%S')

EOF

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "系统信息"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo "[操作系统]"
if command -v lsb_release &> /dev/null; then
    lsb_release -a 2>/dev/null
else
    cat /etc/os-release 2>/dev/null || echo "无法检测"
fi
echo ""

echo "[内核版本]"
uname -a
echo ""

echo "[虚拟化检测]"
systemd-detect-virt 2>/dev/null || echo "未安装 systemd-detect-virt"
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "硬件信息"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo "[CPU 信息]"
lscpu | grep -E "Model name|CPU\(s\)|Thread|Core|Socket|Vendor|Architecture"
echo ""

echo "[内存信息]"
free -h
echo ""

echo "[NUMA 拓扑]"
numactl --hardware 2>/dev/null || echo "numactl 未安装"
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "性能监控 (perf)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo "[perf 版本]"
perf --version 2>&1 || echo "perf 未安装"
echo ""

echo "[perf 权限]"
echo "perf_event_paranoid: $(cat /proc/sys/kernel/perf_event_paranoid 2>/dev/null || echo '无法读取')"
echo "kptr_restrict: $(cat /proc/sys/kernel/kptr_restrict 2>/dev/null || echo '无法读取')"
echo ""

echo "[可用的内存事件]"
perf list 2>/dev/null | grep -i "mem.*retired\|mem.*load\|mem.*inst" || echo "无内存采样事件"
echo ""

echo "[PMU 硬件信息]"
dmesg | grep -i "Performance monitoring" | head -5 || echo "无 PMU 信息"
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "软件版本"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo "[编译工具链]"
echo "GCC: $(gcc --version 2>/dev/null | head -1 || echo '未安装')"
echo "G++: $(g++ --version 2>/dev/null | head -1 || echo '未安装')"
echo "CMake: $(cmake --version 2>/dev/null | head -1 || echo '未安装')"
echo "Make: $(make --version 2>/dev/null | head -1 || echo '未安装')"
echo ""

echo "[Qt 框架]"
if pkg-config --exists Qt6Core 2>/dev/null; then
    echo "Qt6Core: $(pkg-config --modversion Qt6Core)"
    echo "Qt6Widgets: $(pkg-config --modversion Qt6Widgets 2>/dev/null || echo '未安装')"
    echo "Qt6Charts: $(pkg-config --modversion Qt6Charts 2>/dev/null || echo '未安装')"
else
    echo "Qt6 未安装"
fi
echo ""

echo "[开发库]"
if [ -f /usr/include/nlohmann/json.hpp ]; then
    echo "nlohmann-json: 已安装 (header-only)"
elif pkg-config --exists nlohmann_json 2>/dev/null; then
    echo "nlohmann-json: $(pkg-config --modversion nlohmann_json)"
else
    echo "nlohmann-json: 未安装"
fi

if [ -f /usr/include/gtest/gtest.h ] || pkg-config --exists gtest 2>/dev/null; then
    echo "Google Test: 已安装"
else
    echo "Google Test: 未安装"
fi
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "CXLMemSim 状态"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo "[项目目录]"
if [ -f "CMakeLists.txt" ]; then
    echo "当前目录: $(pwd)"
    echo "Git 分支: $(git branch --show-current 2>/dev/null || echo '非 Git 仓库')"
    echo "Git 提交: $(git rev-parse --short HEAD 2>/dev/null || echo '无')"
else
    echo "不在 CXLMemSim 项目目录中"
fi
echo ""

echo "[构建状态]"
if [ -d "build" ]; then
    echo "构建目录: 存在"

    if [ -f "build/CMakeCache.txt" ]; then
        echo "CMake 配置: 已完成"
        echo "构建类型: $(grep CMAKE_BUILD_TYPE build/CMakeCache.txt | cut -d'=' -f2 || echo '未知')"
        echo "Qt GUI: $(grep BUILD_GUI build/CMakeCache.txt | cut -d'=' -f2 || echo '未知')"
    else
        echo "CMake 配置: 未完成"
    fi

    if [ -f "build/cxlmemsim" ]; then
        echo "主程序: 已编译 ($(stat -c%s build/cxlmemsim 2>/dev/null | numfmt --to=iec-i --suffix=B || echo '未知大小'))"
    else
        echo "主程序: 未编译"
    fi
else
    echo "构建目录: 不存在"
fi
echo ""

echo "[运行时测试]"
if [ -x "build/cxlmemsim" ]; then
    echo "执行 --check-tracer:"
    ./build/cxlmemsim --check-tracer 2>&1 | sed 's/^/  /'
else
    echo "可执行文件不存在，跳过运行时测试"
fi
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "内核配置"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

KERNEL_CONFIG="/boot/config-$(uname -r)"
if [ -f "$KERNEL_CONFIG" ]; then
    echo "[性能事件配置]"
    grep -E "CONFIG_PERF_EVENTS|CONFIG_HAVE_PERF_EVENTS|CONFIG_HW_PERF_EVENTS" "$KERNEL_CONFIG" || echo "无相关配置"
    echo ""

    echo "[追踪配置]"
    grep -E "CONFIG_FTRACE|CONFIG_KPROBES|CONFIG_UPROBES" "$KERNEL_CONFIG" | head -5 || echo "无相关配置"
else
    echo "内核配置文件不存在: $KERNEL_CONFIG"
fi
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "环境变量"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo "[相关环境变量]"
echo "PATH: $PATH"
echo "LD_LIBRARY_PATH: ${LD_LIBRARY_PATH:-未设置}"
echo "PKG_CONFIG_PATH: ${PKG_CONFIG_PATH:-未设置}"
echo "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH:-未设置}"
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "日志文件"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

if [ -d "logs" ]; then
    echo "[最近的日志文件]"
    ls -lht logs/*.log 2>/dev/null | head -5 | awk '{print $9 " (" $5 ", " $6 " " $7 " " $8 ")"}' || echo "无日志文件"
else
    echo "日志目录不存在"
fi
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "PEBS 功能测试"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo "[编译并测试 perf_event_open]"
cat > /tmp/test_pebs_diag.c << 'EOFCODE'
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

int main() {
    struct perf_event_attr pe;
    memset(&pe, 0, sizeof(pe));
    pe.type = PERF_TYPE_RAW;
    pe.size = sizeof(pe);
    pe.config = 0x1cd;  // MEM_LOAD_RETIRED.L3_MISS
    pe.sample_period = 10000;
    pe.sample_type = PERF_SAMPLE_ADDR | PERF_SAMPLE_TIME;
    pe.precise_ip = 2;
    pe.disabled = 1;

    int fd = syscall(__NR_perf_event_open, &pe, -1, 0, -1, 0);
    if (fd >= 0) {
        close(fd);
        printf("PEBS 测试: 成功 ✓\n");
        printf("说明: 系统支持精确的内存采样事件\n");
        return 0;
    }

    printf("PEBS 测试: 失败 ✗\n");
    printf("错误码: %d (%s)\n", errno, strerror(errno));

    if (errno == 1) {
        printf("原因: 权限不足\n");
        printf("解决: sudo sysctl -w kernel.perf_event_paranoid=1\n");
    } else if (errno == 95) {
        printf("原因: 硬件不支持 PEBS (虚拟机或旧 CPU)\n");
        printf("解决: 使用 MockTracer 开发，物理机部署时会自动切换到 PEBS\n");
    } else if (errno == 22) {
        printf("原因: 无效的事件配置\n");
        printf("解决: 检查 CPU 是否支持 MEM_LOAD_RETIRED.L3_MISS 事件\n");
    }

    return 1;
}
EOFCODE

gcc -o /tmp/test_pebs_diag /tmp/test_pebs_diag.c 2>/dev/null && /tmp/test_pebs_diag || echo "编译测试程序失败"
rm -f /tmp/test_pebs_diag.c /tmp/test_pebs_diag
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "建议操作"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# 根据检测结果给出建议
IS_VM=$(systemd-detect-virt 2>/dev/null)
PARANOID=$(cat /proc/sys/kernel/perf_event_paranoid 2>/dev/null || echo "unknown")

if [ "$IS_VM" != "none" ]; then
    echo "✓ 虚拟机环境 - 使用 MockTracer 开发"
    echo "  - 当前环境不支持 PEBS，这是正常的"
    echo "  - 可以完成所有开发工作 (GUI, Analyzer, Injector)"
    echo "  - 部署到物理机时会自动切换到 PEBS"
elif [ "$PARANOID" -gt 1 ]; then
    echo "⚠ 物理机环境 - 需要配置权限"
    echo "  - 执行: sudo sysctl -w kernel.perf_event_paranoid=1"
    echo "  - 或使用: sudo ./build/cxlmemsim"
else
    echo "✓ 物理机环境 - 权限已配置"
    echo "  - 可以使用 PEBS 精确采样"
    echo "  - 建议运行: ./build/cxlmemsim --check-tracer 验证"
fi

echo ""
echo "================================================================================
报告结束
================================================================================
"

echo "如需保存报告，请重新运行:"
echo "  ./scripts/collect_diagnostic_info.sh > diagnostic_report.txt"
echo ""
