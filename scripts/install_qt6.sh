#!/bin/bash
# Script to install Qt6 development environment

set -e

echo "========================================"
echo "Installing Qt6 Development Environment"
echo "========================================"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "This script must be run with sudo"
    echo "Usage: sudo ./install_qt6.sh"
    exit 1
fi

echo "[1/3] Updating package lists..."
apt update

echo ""
echo "[2/3] Installing Qt6 packages..."
apt install -y \
    qt6-base-dev \
    qt6-tools-dev \
    qt6-tools-dev-tools \
    libqt6opengl6-dev \
    qmake6

echo ""
echo "[3/3] Verifying installation..."
if command -v qmake6 &> /dev/null; then
    echo "✓ Qt6 installed successfully!"
    echo ""
    qmake6 --version
else
    echo "✗ Qt6 installation may have failed"
    exit 1
fi

echo ""
echo "========================================"
echo "Qt6 Installation Complete!"
echo "========================================"
echo ""
echo "You can now build the GUI:"
echo "  cd build"
echo "  cmake .. -DBUILD_GUI=ON"
echo "  make cxlmemsim_gui"
echo ""
