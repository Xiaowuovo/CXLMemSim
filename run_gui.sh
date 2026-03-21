#!/bin/bash

# CXLMemSim GUI 启动脚本 (VM 优化版)

PROJECT_DIR=$(cd "$(dirname "$0")" && pwd)
GUI_EXECUTABLE="$PROJECT_DIR/build/frontend/cxlmemsim_gui"

# 检查是否已编译
if [ ! -f "$GUI_EXECUTABLE" ]; then
    echo "错误: GUI 未找到，请先编译项目"
    echo "运行: ./scripts/setup_vm.sh"
    exit 1
fi

# 设置 Qt 环境变量 (VM 优化)
export QT_QPA_PLATFORM=xcb
export QT_X11_NO_MITSHM=1
export QT_XCB_GL_INTEGRATION=xcb_egl
export LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH

# 检查 X11 显示
if [ -z "$DISPLAY" ]; then
    echo "警告: DISPLAY 环境变量未设置"
    echo "尝试使用默认值 :0"
    export DISPLAY=:0
fi

echo "启动 CXLMemSim GUI..."
echo "项目目录: $PROJECT_DIR"
echo "显示服务器: $DISPLAY"
echo ""

# 启动 GUI
cd "$PROJECT_DIR"
"$GUI_EXECUTABLE" "$@"
