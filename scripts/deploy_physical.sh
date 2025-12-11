#!/bin/bash

################################################################################
# CXLMemSim 物理服务器自动部署脚本
#
# 功能:
#   - 检测硬件支持 (Intel CPU, PEBS)
#   - 安装所有依赖项
#   - 配置 perf 权限
#   - 编译项目
#   - 运行验证测试
#   - 生成部署报告
#
# 用法:
#   sudo ./scripts/deploy_physical.sh [选项]
#
# 选项:
#   --auto          自动模式 (跳过确认提示)
#   --skip-deps     跳过依赖安装
#   --skip-build    跳过编译
#   --skip-tests    跳过测试
#   --install-dir   安装目录 (默认: 当前目录)
#   --help          显示帮助
#
# 示例:
#   sudo ./scripts/deploy_physical.sh --auto
#   ./scripts/deploy_physical.sh --skip-deps --skip-tests
#
################################################################################

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color
BOLD='\033[1m'

# 默认选项
AUTO_MODE=false
SKIP_DEPS=false
SKIP_BUILD=false
SKIP_TESTS=false
INSTALL_DIR=$(pwd)
BUILD_TYPE="Release"
BUILD_GUI=true
NUM_JOBS=$(nproc)

# 部署状态
HAS_PEBS=false
HAS_QT=false
IS_VM=false
DEPLOYMENT_SUCCESS=true

# 日志文件
LOG_DIR="${INSTALL_DIR}/logs"
DEPLOY_LOG="${LOG_DIR}/deployment_$(date +%Y%m%d_%H%M%S).log"
TEST_LOG="${LOG_DIR}/tests_$(date +%Y%m%d_%H%M%S).log"

################################################################################
# 辅助函数
################################################################################

print_header() {
    echo ""
    echo -e "${CYAN}${BOLD}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${CYAN}${BOLD}  $1${NC}"
    echo -e "${CYAN}${BOLD}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo ""
}

print_step() {
    echo -e "${BLUE}${BOLD}[$(date +%H:%M:%S)]${NC} ${BOLD}$1${NC}"
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

print_info() {
    echo -e "${CYAN}ℹ${NC} $1"
}

ask_confirmation() {
    if [ "$AUTO_MODE" = true ]; then
        return 0
    fi

    local prompt="$1"
    local default="${2:-Y}"

    if [ "$default" = "Y" ]; then
        read -p "$prompt [Y/n]: " response
        response=${response:-Y}
    else
        read -p "$prompt [y/N]: " response
        response=${response:-N}
    fi

    case "$response" in
        [yY][eE][sS]|[yY]) return 0 ;;
        *) return 1 ;;
    esac
}

log_message() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" >> "$DEPLOY_LOG"
}

check_root() {
    if [ "$SKIP_DEPS" = false ] && [ "$EUID" -ne 0 ]; then
        print_warning "某些操作需要 root 权限"
        print_info "建议使用: sudo $0 $@"
        if ! ask_confirmation "是否继续? (依赖安装将被跳过)"; then
            exit 1
        fi
        SKIP_DEPS=true
    fi
}

################################################################################
# 解析命令行参数
################################################################################

parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --auto)
                AUTO_MODE=true
                shift
                ;;
            --skip-deps)
                SKIP_DEPS=true
                shift
                ;;
            --skip-build)
                SKIP_BUILD=true
                shift
                ;;
            --skip-tests)
                SKIP_TESTS=true
                shift
                ;;
            --install-dir)
                INSTALL_DIR="$2"
                shift 2
                ;;
            --no-gui)
                BUILD_GUI=false
                shift
                ;;
            --debug)
                BUILD_TYPE="Debug"
                shift
                ;;
            --help)
                cat << EOF
CXLMemSim 物理服务器部署脚本

用法: $0 [选项]

选项:
  --auto          自动模式 (跳过所有确认提示)
  --skip-deps     跳过依赖安装
  --skip-build    跳过编译步骤
  --skip-tests    跳过测试步骤
  --install-dir   指定安装目录 (默认: 当前目录)
  --no-gui        不编译 Qt GUI
  --debug         使用 Debug 模式编译
  --help          显示此帮助信息

示例:
  sudo $0 --auto                    # 全自动部署
  $0 --skip-deps --skip-tests       # 仅编译，跳过依赖和测试
  sudo $0 --install-dir /opt/CXLMemSim  # 安装到指定目录

EOF
                exit 0
                ;;
            *)
                print_error "未知选项: $1"
                echo "使用 --help 查看帮助"
                exit 1
                ;;
        esac
    done
}

################################################################################
# 系统检测
################################################################################

detect_environment() {
    print_header "系统环境检测"

    # 操作系统
    print_step "检测操作系统..."
    OS_NAME=$(lsb_release -si 2>/dev/null || echo "Unknown")
    OS_VERSION=$(lsb_release -sr 2>/dev/null || echo "Unknown")
    KERNEL_VERSION=$(uname -r)

    print_info "操作系统: $OS_NAME $OS_VERSION"
    print_info "内核版本: $KERNEL_VERSION"
    log_message "OS: $OS_NAME $OS_VERSION, Kernel: $KERNEL_VERSION"

    # 检查虚拟化
    print_step "检测虚拟化环境..."
    VIRT_TYPE=$(systemd-detect-virt 2>/dev/null || echo "none")

    if [ "$VIRT_TYPE" != "none" ]; then
        IS_VM=true
        print_warning "检测到虚拟化环境: $VIRT_TYPE"
        print_warning "PEBS 在虚拟机中不可用，将使用 Mock Tracer"
        log_message "WARN: Running in virtual environment: $VIRT_TYPE"
    else
        print_success "运行在物理硬件上"
        log_message "Running on physical hardware"
    fi

    # CPU 信息
    print_step "检测 CPU 型号..."
    CPU_MODEL=$(lscpu | grep "Model name" | cut -d':' -f2 | xargs)
    CPU_CORES=$(nproc)

    print_info "CPU: $CPU_MODEL"
    print_info "CPU 核心数: $CPU_CORES"
    log_message "CPU: $CPU_MODEL, Cores: $CPU_CORES"

    # 检查是否为 Intel CPU
    if [[ "$CPU_MODEL" == *"Intel"* ]]; then
        print_success "检测到 Intel CPU"
    else
        print_warning "非 Intel CPU - PEBS 可能不可用"
        print_info "AMD CPU 请考虑使用 IBS (Instruction-Based Sampling)"
    fi

    # 内存
    print_step "检测内存配置..."
    TOTAL_MEM_GB=$(free -g | awk '/^Mem:/{print $2}')
    print_info "总内存: ${TOTAL_MEM_GB}GB"
    log_message "Memory: ${TOTAL_MEM_GB}GB"

    if [ "$TOTAL_MEM_GB" -lt 8 ]; then
        print_warning "内存少于 8GB，可能影响性能"
    fi

    echo ""
}

detect_pebs_support() {
    print_header "PEBS 硬件支持检测"

    if [ "$IS_VM" = true ]; then
        print_warning "跳过 PEBS 检测 (虚拟机环境)"
        HAS_PEBS=false
        return
    fi

    # 检查 perf 工具
    print_step "检查 perf 工具..."
    if command -v perf &> /dev/null; then
        print_success "perf 工具已安装"
        PERF_VERSION=$(perf --version 2>&1 | head -1)
        print_info "版本: $PERF_VERSION"
    else
        print_warning "perf 工具未安装"
        log_message "WARN: perf not installed"
    fi

    # 检查 PMU 事件
    print_step "检查内存采样事件..."
    if perf list 2>/dev/null | grep -q "mem_load_retired.l3_miss"; then
        print_success "找到 mem_load_retired.l3_miss 事件"
        HAS_PEBS=true
        log_message "PEBS events found"
    else
        print_warning "未找到 PEBS 内存采样事件"
        HAS_PEBS=false
        log_message "WARN: PEBS events not found"
    fi

    # 尝试测试性打开 perf 事件
    print_step "测试 perf_event_open 系统调用..."

    cat > /tmp/test_pebs.c << 'EOF'
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

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
        printf("SUCCESS\n");
        return 0;
    }
    printf("FAILED: errno=%d\n", errno);
    return 1;
}
EOF

    gcc -o /tmp/test_pebs /tmp/test_pebs.c 2>/dev/null
    PEBS_TEST_RESULT=$(/tmp/test_pebs 2>&1)

    if [[ "$PEBS_TEST_RESULT" == "SUCCESS" ]]; then
        print_success "PEBS 系统调用测试成功"
        HAS_PEBS=true
    else
        print_warning "PEBS 测试失败: $PEBS_TEST_RESULT"
        HAS_PEBS=false

        # 检查常见原因
        PARANOID=$(cat /proc/sys/kernel/perf_event_paranoid 2>/dev/null || echo "unknown")
        print_info "当前 perf_event_paranoid: $PARANOID"

        if [ "$PARANOID" -gt 1 ]; then
            print_info "需要设置 perf_event_paranoid <= 1"
        fi
    fi

    rm -f /tmp/test_pebs.c /tmp/test_pebs
    echo ""
}

detect_dependencies() {
    print_header "依赖检测"

    # CMake
    print_step "检查 CMake..."
    if command -v cmake &> /dev/null; then
        CMAKE_VERSION=$(cmake --version | head -1 | awk '{print $3}')
        print_success "CMake $CMAKE_VERSION"

        if [ "$(printf '%s\n' "3.20" "$CMAKE_VERSION" | sort -V | head -1)" != "3.20" ]; then
            print_warning "CMake 版本低于 3.20，可能无法编译"
        fi
    else
        print_error "CMake 未安装"
    fi

    # GCC
    print_step "检查 GCC..."
    if command -v g++ &> /dev/null; then
        GCC_VERSION=$(g++ --version | head -1 | awk '{print $NF}')
        print_success "GCC $GCC_VERSION"
    else
        print_error "GCC 未安装"
    fi

    # Qt6
    print_step "检查 Qt6..."
    if pkg-config --exists Qt6Core 2>/dev/null; then
        QT_VERSION=$(pkg-config --modversion Qt6Core)
        print_success "Qt6 $QT_VERSION"
        HAS_QT=true
    else
        print_warning "Qt6 未安装 (GUI 将不可用)"
        HAS_QT=false
    fi

    # nlohmann-json
    print_step "检查 nlohmann-json..."
    if [ -f /usr/include/nlohmann/json.hpp ]; then
        print_success "nlohmann-json (header-only)"
    elif pkg-config --exists nlohmann_json 2>/dev/null; then
        print_success "nlohmann-json (package)"
    else
        print_warning "nlohmann-json 未安装"
    fi

    # Google Test
    print_step "检查 Google Test..."
    if pkg-config --exists gtest 2>/dev/null || [ -f /usr/include/gtest/gtest.h ]; then
        print_success "Google Test"
    else
        print_warning "Google Test 未安装 (测试将不可用)"
    fi

    echo ""
}

################################################################################
# 依赖安装
################################################################################

install_dependencies() {
    if [ "$SKIP_DEPS" = true ]; then
        print_info "跳过依赖安装"
        return
    fi

    print_header "安装依赖包"

    if ! ask_confirmation "是否安装/更新依赖包?"; then
        print_info "跳过依赖安装"
        return
    fi

    print_step "更新软件包列表..."
    apt update >> "$DEPLOY_LOG" 2>&1 || {
        print_error "apt update 失败"
        DEPLOYMENT_SUCCESS=false
        return
    }

    print_step "安装编译工具链..."
    apt install -y \
        build-essential \
        cmake \
        git \
        pkg-config \
        >> "$DEPLOY_LOG" 2>&1
    print_success "编译工具链安装完成"

    print_step "安装性能监控工具..."
    apt install -y \
        linux-tools-common \
        linux-tools-generic \
        linux-tools-$(uname -r) \
        >> "$DEPLOY_LOG" 2>&1
    print_success "性能工具安装完成"

    print_step "安装开发库..."
    apt install -y \
        nlohmann-json3-dev \
        libgtest-dev \
        libgmock-dev \
        >> "$DEPLOY_LOG" 2>&1
    print_success "开发库安装完成"

    if [ "$BUILD_GUI" = true ]; then
        print_step "安装 Qt6 开发环境..."
        apt install -y \
            qt6-base-dev \
            qt6-tools-dev \
            qt6-tools-dev-tools \
            libqt6charts6-dev \
            libqt6svg6-dev \
            >> "$DEPLOY_LOG" 2>&1
        print_success "Qt6 安装完成"
        HAS_QT=true
    fi

    print_success "所有依赖安装完成"
    echo ""
}

################################################################################
# 权限配置
################################################################################

configure_perf_permissions() {
    print_header "配置 perf 权限"

    if [ "$IS_VM" = true ]; then
        print_info "虚拟机环境，跳过权限配置"
        return
    fi

    if [ "$EUID" -ne 0 ]; then
        print_warning "需要 root 权限配置 perf"
        return
    fi

    CURRENT_PARANOID=$(cat /proc/sys/kernel/perf_event_paranoid)
    print_info "当前 perf_event_paranoid: $CURRENT_PARANOID"

    if [ "$CURRENT_PARANOID" -le 1 ]; then
        print_success "权限已正确配置"
        return
    fi

    if ! ask_confirmation "是否设置 perf_event_paranoid=1? (推荐)"; then
        print_info "跳过权限配置"
        return
    fi

    print_step "设置 perf_event_paranoid=1..."
    sysctl -w kernel.perf_event_paranoid=1 >> "$DEPLOY_LOG" 2>&1
    print_success "临时设置成功"

    print_step "设置 kernel.kptr_restrict=0..."
    sysctl -w kernel.kptr_restrict=0 >> "$DEPLOY_LOG" 2>&1

    if ask_confirmation "是否永久保存设置? (写入 /etc/sysctl.conf)"; then
        echo "# CXLMemSim PEBS configuration" >> /etc/sysctl.conf
        echo "kernel.perf_event_paranoid=1" >> /etc/sysctl.conf
        echo "kernel.kptr_restrict=0" >> /etc/sysctl.conf
        print_success "永久配置已保存"
    fi

    echo ""
}

################################################################################
# 编译项目
################################################################################

build_project() {
    if [ "$SKIP_BUILD" = true ]; then
        print_info "跳过编译"
        return
    fi

    print_header "编译 CXLMemSim"

    cd "$INSTALL_DIR"

    # 创建 build 目录
    print_step "创建构建目录..."
    rm -rf build
    mkdir -p build
    cd build

    # 配置 CMake
    print_step "配置 CMake ($BUILD_TYPE 模式)..."

    CMAKE_ARGS="-DCMAKE_BUILD_TYPE=$BUILD_TYPE"

    if [ "$HAS_QT" = true ] && [ "$BUILD_GUI" = true ]; then
        CMAKE_ARGS="$CMAKE_ARGS -DBUILD_GUI=ON"
        print_info "启用 Qt GUI 编译"
    else
        CMAKE_ARGS="$CMAKE_ARGS -DBUILD_GUI=OFF"
        print_info "禁用 Qt GUI 编译"
    fi

    CMAKE_ARGS="$CMAKE_ARGS -DBUILD_TESTS=ON"

    cmake $CMAKE_ARGS .. >> "$DEPLOY_LOG" 2>&1 || {
        print_error "CMake 配置失败，请查看日志: $DEPLOY_LOG"
        DEPLOYMENT_SUCCESS=false
        return
    }
    print_success "CMake 配置成功"

    # 编译
    print_step "编译项目 (使用 $NUM_JOBS 个并发任务)..."

    make -j"$NUM_JOBS" >> "$DEPLOY_LOG" 2>&1 || {
        print_error "编译失败，请查看日志: $DEPLOY_LOG"
        DEPLOYMENT_SUCCESS=false
        return
    }

    print_success "编译成功"

    # 列出生成的可执行文件
    print_info "生成的可执行文件:"
    ls -lh cxlmemsim simple_simulation 2>/dev/null | awk '{print "  " $9 " (" $5 ")"}'

    echo ""
}

################################################################################
# 运行测试
################################################################################

run_tests() {
    if [ "$SKIP_TESTS" = true ]; then
        print_info "跳过测试"
        return
    fi

    print_header "运行验证测试"

    cd "$INSTALL_DIR/build"

    # 测试 1: Tracer 检测
    print_step "测试 1: Tracer 类型检测..."
    if ./cxlmemsim --check-tracer >> "$TEST_LOG" 2>&1; then
        TRACER_OUTPUT=$(./cxlmemsim --check-tracer 2>&1)

        if echo "$TRACER_OUTPUT" | grep -q "PEBSTracer"; then
            print_success "使用 PEBSTracer (硬件采样)"
        elif echo "$TRACER_OUTPUT" | grep -q "MockTracer"; then
            print_success "使用 MockTracer (模拟数据)"
        else
            print_warning "Tracer 类型未知"
        fi

        echo "$TRACER_OUTPUT" | sed 's/^/    /'
    else
        print_error "Tracer 检测失败"
        DEPLOYMENT_SUCCESS=false
    fi

    # 测试 2: 单元测试
    print_step "测试 2: 运行单元测试..."
    if [ -x "./tests/test_tracer" ]; then
        if ctest --output-on-failure >> "$TEST_LOG" 2>&1; then
            PASSED=$(grep "tests passed" "$TEST_LOG" | tail -1 | awk '{print $1}')
            print_success "单元测试通过 ($PASSED 个测试)"
        else
            print_warning "部分单元测试失败，查看详情: $TEST_LOG"
        fi
    else
        print_info "单元测试未编译"
    fi

    # 测试 3: Mock Tracer 功能
    print_step "测试 3: Mock Tracer 功能测试..."
    if ./cxlmemsim --test-mock >> "$TEST_LOG" 2>&1; then
        print_success "Mock Tracer 功能正常"
    else
        print_warning "Mock Tracer 测试失败"
    fi

    # 测试 4: PEBS 真实采样 (仅在支持时)
    if [ "$HAS_PEBS" = true ] && [ "$IS_VM" = false ]; then
        print_step "测试 4: PEBS 真实采样测试..."

        if ./cxlmemsim --mode=trace \
            --target=/bin/ls \
            --target-args="-lah /usr" \
            --output=/tmp/test_trace.json \
            --duration=5 \
            >> "$TEST_LOG" 2>&1; then

            if [ -f /tmp/test_trace.json ]; then
                EVENTS=$(jq '.events | length' /tmp/test_trace.json 2>/dev/null || echo 0)
                if [ "$EVENTS" -gt 0 ]; then
                    print_success "采集到 $EVENTS 个内存访问事件"
                else
                    print_warning "未采集到事件，可能需要调整采样参数"
                fi
                rm -f /tmp/test_trace.json
            fi
        else
            print_warning "PEBS 采样测试失败"
        fi
    fi

    echo ""
}

################################################################################
# 生成部署报告
################################################################################

generate_report() {
    print_header "生成部署报告"

    REPORT_FILE="${INSTALL_DIR}/deployment_report_$(date +%Y%m%d_%H%M%S).txt"

    cat > "$REPORT_FILE" << EOF
================================================================================
CXLMemSim 物理服务器部署报告
================================================================================

部署时间: $(date '+%Y-%m-%d %H:%M:%S')
安装目录: $INSTALL_DIR
部署状态: $([ "$DEPLOYMENT_SUCCESS" = true ] && echo "成功" || echo "失败")

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
系统环境
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

操作系统:       $OS_NAME $OS_VERSION
内核版本:       $KERNEL_VERSION
虚拟化:         $VIRT_TYPE
CPU 型号:       $CPU_MODEL
CPU 核心数:     $CPU_CORES
总内存:         ${TOTAL_MEM_GB}GB

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
硬件支持
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

PEBS 支持:      $([ "$HAS_PEBS" = true ] && echo "✓ 是" || echo "✗ 否")
推荐 Tracer:    $([ "$HAS_PEBS" = true ] && echo "PEBSTracer" || echo "MockTracer")

EOF

    if [ "$HAS_PEBS" = true ]; then
        cat >> "$REPORT_FILE" << EOF
说明: 检测到 PEBS 支持，可使用硬件精确采样
      - 可以采集真实内存访问 trace
      - 支持精确的虚拟地址和延迟测量
      - 适合生产环境部署
EOF
    else
        cat >> "$REPORT_FILE" << EOF
说明: PEBS 不可用 (虚拟机或不支持的 CPU)
      - 使用 MockTracer 进行开发和测试
      - 可以加载预录制的 trace 文件
      - 不影响其他功能 (GUI, Analyzer, Injector)
EOF
    fi

    cat >> "$REPORT_FILE" << EOF

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
软件版本
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

CMake:          $(cmake --version 2>/dev/null | head -1 | awk '{print $3}' || echo "未安装")
GCC:            $(g++ --version 2>/dev/null | head -1 | awk '{print $NF}' || echo "未安装")
Qt6:            $([ "$HAS_QT" = true ] && pkg-config --modversion Qt6Core || echo "未安装")
perf:           $(perf --version 2>/dev/null | head -1 || echo "未安装")

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
编译配置
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

构建类型:       $BUILD_TYPE
Qt GUI:         $([ "$BUILD_GUI" = true ] && [ "$HAS_QT" = true ] && echo "启用" || echo "禁用")
单元测试:       $([ "$SKIP_TESTS" = false ] && echo "启用" || echo "禁用")
并发任务数:     $NUM_JOBS

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
生成的文件
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

可执行文件:
EOF

    if [ -d "$INSTALL_DIR/build" ]; then
        ls -lh "$INSTALL_DIR/build/cxlmemsim" 2>/dev/null | awk '{print "  " $9 " (" $5 ")"}' >> "$REPORT_FILE" || echo "  未生成" >> "$REPORT_FILE"
    fi

    cat >> "$REPORT_FILE" << EOF

日志文件:
  部署日志: $DEPLOY_LOG
  测试日志: $TEST_LOG

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
下一步操作
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

1. 验证安装:
   cd $INSTALL_DIR/build
   ./cxlmemsim --check-tracer

2. 运行测试:
   ./cxlmemsim --test-mock
   ctest --output-on-failure

3. 启动 GUI (如果已编译):
   ./cxlmemsim

4. 采集真实 trace (如果 PEBS 可用):
   ./cxlmemsim --mode=trace --target=/bin/ls --duration=5

5. 查看文档:
   less $INSTALL_DIR/docs/PHYSICAL_DEPLOYMENT.md

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
支持信息
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

问题反馈: 请提交 issue 到项目仓库
文档:     $INSTALL_DIR/docs/
示例:     $INSTALL_DIR/configs/examples/

================================================================================
报告生成时间: $(date '+%Y-%m-%d %H:%M:%S')
================================================================================
EOF

    print_success "部署报告已保存到: $REPORT_FILE"

    # 显示摘要
    echo ""
    cat "$REPORT_FILE"
}

################################################################################
# 主流程
################################################################################

main() {
    # 解析参数
    parse_args "$@"

    # 创建日志目录
    mkdir -p "$LOG_DIR"

    # 打印欢迎信息
    clear
    cat << "EOF"
   ____  ____  ____     __  __                 ____  _
  / ___||  _ \|  _ \   |  \/  | ___ _ __ ___ / ___|(_)_ __ ___
 | |    | |_) | |_) |  | |\/| |/ _ \ '_ ` _ \\___ \| | '_ ` _ \
 | |___ |  __/|  __/   | |  | |  __/ | | | | |___) | | | | | | |
  \____|_|   |_|      |_|  |_|\___|_| |_| |_|____/|_|_| |_| |_|

           物理服务器自动部署脚本 v1.0
EOF

    echo ""
    print_info "部署日志: $DEPLOY_LOG"
    echo ""

    log_message "Deployment started"
    log_message "Arguments: $@"

    # 检查 root 权限
    check_root "$@"

    # 执行部署步骤
    detect_environment
    detect_pebs_support
    detect_dependencies
    install_dependencies
    configure_perf_permissions
    build_project
    run_tests
    generate_report

    # 最终状态
    echo ""
    print_header "部署完成"

    if [ "$DEPLOYMENT_SUCCESS" = true ]; then
        print_success "CXLMemSim 已成功部署到 $INSTALL_DIR"

        if [ "$HAS_PEBS" = true ]; then
            print_info "使用 PEBSTracer (硬件精确采样)"
        else
            print_info "使用 MockTracer (虚拟机模式)"
        fi

        echo ""
        print_info "快速开始:"
        echo "  cd $INSTALL_DIR/build"
        echo "  ./cxlmemsim --check-tracer"

        if [ "$HAS_QT" = true ] && [ "$BUILD_GUI" = true ]; then
            echo "  ./cxlmemsim  # 启动 GUI"
        fi

        log_message "Deployment completed successfully"
        exit 0
    else
        print_error "部署过程中遇到错误"
        print_info "请查看日志: $DEPLOY_LOG"
        log_message "Deployment failed"
        exit 1
    fi
}

# 执行主流程
main "$@"
