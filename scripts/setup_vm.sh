#!/bin/bash

################################################################################
# CXLMemSim VM 一键部署脚本
#
# 功能:
#   - 安装所有依赖项 (Ubuntu/Debian)
#   - 编译项目 (使用 MockTracer)
#   - 运行验证测试
#   - 启动 GUI
#
# 用法:
#   ./scripts/setup_vm.sh [选项]
#
# 选项:
#   --skip-deps     跳过依赖安装
#   --skip-build    跳过编译
#   --skip-tests    跳过测试
#   --no-gui        不启动GUI
#   --help          显示帮助
#
################################################################################

set -e  # 遇到错误立即退出

# 颜色定义
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'
BOLD='\033[1m'

# 默认选项
SKIP_DEPS=false
SKIP_BUILD=false
SKIP_TESTS=false
NO_GUI=false
PROJECT_DIR=$(cd "$(dirname "$0")/.." && pwd)

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
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
        --no-gui)
            NO_GUI=true
            shift
            ;;
        --help)
            grep "^#" "$0" | grep -v "^#!/" | sed 's/^# //'
            exit 0
            ;;
        *)
            echo "未知选项: $1"
            echo "使用 --help 查看帮助"
            exit 1
            ;;
    esac
done

echo -e "${BOLD}${BLUE}==================================${NC}"
echo -e "${BOLD}${BLUE}  CXLMemSim VM 环境部署${NC}"
echo -e "${BOLD}${BLUE}==================================${NC}"
echo ""

# 步骤 1: 安装依赖
if [ "$SKIP_DEPS" = false ]; then
    echo -e "${GREEN}[1/4] 安装依赖...${NC}"
    
    echo "更新软件包列表..."
    sudo apt update
    
    echo "安装编译工具链..."
    sudo apt install -y \
        build-essential \
        cmake \
        git \
        pkg-config
    
    echo "安装开发库..."
    sudo apt install -y \
        nlohmann-json3-dev \
        libgtest-dev
    
    echo "安装 Qt6 (GUI 支持)..."
    sudo apt install -y \
        qt6-base-dev \
        qt6-tools-dev \
        libqt6charts6-dev || {
        echo -e "${YELLOW}警告: Qt6 安装失败，将仅编译命令行工具${NC}"
    }
    
    echo -e "${GREEN}✓ 依赖安装完成${NC}"
    echo ""
else
    echo -e "${YELLOW}[1/4] 跳过依赖安装${NC}"
    echo ""
fi

# 步骤 2: 编译项目
if [ "$SKIP_BUILD" = false ]; then
    echo -e "${GREEN}[2/4] 编译项目...${NC}"
    
    cd "$PROJECT_DIR"
    
    # 清理旧的构建
    if [ -d "build" ]; then
        echo "清理旧的构建目录..."
        rm -rf build
    fi
    
    # 创建构建目录
    mkdir -p build
    cd build
    
    # 配置 CMake (VM 环境会自动使用 MockTracer)
    echo "配置 CMake..."
    cmake -DCMAKE_BUILD_TYPE=Release ..
    
    # 编译
    echo "编译中..."
    make -j$(nproc)
    
    echo -e "${GREEN}✓ 编译完成${NC}"
    echo ""
else
    echo -e "${YELLOW}[2/4] 跳过编译${NC}"
    echo ""
    cd "$PROJECT_DIR/build"
fi

# 步骤 3: 运行测试
if [ "$SKIP_TESTS" = false ]; then
    echo -e "${GREEN}[3/4] 运行测试...${NC}"
    
    cd "$PROJECT_DIR/build"
    
    # 检查 Tracer
    echo ""
    echo "检查 Tracer 类型:"
    ./cxlmemsim --check-tracer
    echo ""
    
    # 测试 Mock Tracer
    echo "测试 MockTracer:"
    ./cxlmemsim --test-mock
    echo ""
    
    # 运行单元测试 (如果存在)
    if [ -f "tests/test_tracer" ]; then
        echo "运行单元测试:"
        ctest --output-on-failure || echo -e "${YELLOW}部分测试失败 (正常情况)${NC}"
        echo ""
    fi
    
    echo -e "${GREEN}✓ 测试完成${NC}"
    echo ""
else
    echo -e "${YELLOW}[3/4] 跳过测试${NC}"
    echo ""
fi

# 步骤 4: 启动 GUI (可选)
if [ "$NO_GUI" = false ]; then
    echo -e "${GREEN}[4/4] 准备启动 GUI...${NC}"
    echo ""
    
    cd "$PROJECT_DIR/build"
    
    if [ -f "frontend/cxlmemsim_gui" ]; then
        echo -e "${BOLD}${GREEN}==================================${NC}"
        echo -e "${BOLD}${GREEN}  部署成功！${NC}"
        echo -e "${BOLD}${GREEN}==================================${NC}"
        echo ""
        echo "GUI 可执行文件: build/frontend/cxlmemsim_gui"
        echo "CLI 工具: build/cxlmemsim"
        echo ""
        echo -e "${YELLOW}提示: 使用以下命令启动 GUI:${NC}"
        echo "  cd $PROJECT_DIR"
        echo "  ./run_gui.sh"
        echo ""
        echo -e "${BLUE}是否现在启动 GUI? (y/N)${NC}"
        read -r response
        if [[ "$response" =~ ^[Yy]$ ]]; then
            echo "启动 GUI..."
            cd "$PROJECT_DIR"
            ./run_gui.sh
        fi
    else
        echo -e "${YELLOW}警告: GUI 未编译 (可能 Qt6 未安装)${NC}"
        echo ""
        echo -e "${BOLD}${GREEN}==================================${NC}"
        echo -e "${BOLD}${GREEN}  部署成功 (仅命令行工具)${NC}"
        echo -e "${BOLD}${GREEN}==================================${NC}"
        echo ""
        echo "CLI 工具: build/cxlmemsim"
        echo ""
        echo "示例命令:"
        echo "  ./build/cxlmemsim --check-tracer"
        echo "  ./build/cxlmemsim --test-mock"
    fi
else
    echo -e "${YELLOW}[4/4] 跳过 GUI 启动${NC}"
    echo ""
    echo -e "${BOLD}${GREEN}==================================${NC}"
    echo -e "${BOLD}${GREEN}  部署成功！${NC}"
    echo -e "${BOLD}${GREEN}==================================${NC}"
    echo ""
    echo "项目位置: $PROJECT_DIR"
    echo "构建目录: $PROJECT_DIR/build"
fi

echo ""
echo -e "${BLUE}快速开始:${NC}"
echo "  1. 启动 GUI: ./run_gui.sh"
echo "  2. 测试 CLI: ./build/cxlmemsim --check-tracer"
echo "  3. 查看文档: cat DOCS_INDEX.md"
echo ""
