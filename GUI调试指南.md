# CXLMemSim GUI 段错误调试指南

## 当前状况

GUI启动时发生段错误（Segmentation Fault / Core Dumped），需要定位具体原因。

---

## 快速调试步骤

### 步骤1：使用 GDB 调试器定位问题

```bash
cd ~/work/CXLMemSim

# 1. 使用gdb运行GUI
gdb ./build/frontend/cxlmemsim_gui

# 2. 在gdb中运行程序
(gdb) run

# 3. 程序崩溃后，查看调用栈
(gdb) bt

# 4. 查看详细调用栈（包含所有参数）
(gdb) bt full

# 5. 退出gdb
(gdb) quit
```

**请将 `bt` 和 `bt full` 的输出发给我分析！**

---

### 步骤2：查看核心转储文件

```bash
# 1. 启用核心转储
ulimit -c unlimited

# 2. 重新运行GUI
./run_gui.sh

# 3. 如果生成了core文件，使用gdb分析
gdb ./build/frontend/cxlmemsim_gui core

# 4. 在gdb中查看调用栈
(gdb) bt
(gdb) bt full
```

---

### 步骤3：逐步禁用组件测试

我已经在代码中添加了调试注释，现在测试最简化版本：

#### 测试1：编译并运行当前版本

```bash
cd ~/work/CXLMemSim/build
make clean
make -j$(nproc)
./run_gui.sh
```

如果仍然段错误，继续下一步。

---

#### 测试2：检查Qt版本兼容性

```bash
# 检查Qt版本
qmake6 --version
# 或
qmake --version

# 检查已安装的Qt6组件
dpkg -l | grep qt6
```

**要求的Qt版本**: Qt 6.2.0 或更高

---

#### 测试3：简化子widget

临时修改 `frontend/widgets/config_tree_widget.cpp`：

在 `ConfigTreeWidget` 构造函数中注释掉配置加载：

```cpp
ConfigTreeWidget::ConfigTreeWidget(QWidget *parent)
    : QWidget(parent)
    , tree_(nullptr)
    , addDeviceButton_(nullptr)
    , addSwitchButton_(nullptr)
    , removeButton_(nullptr)
{
    setupUI();
    // 临时注释掉
    // config_ = cxlsim::ConfigParser::create_default_config();
    // populateTree();
}
```

重新编译测试：
```bash
cd ~/work/CXLMemSim/build
make -j$(nproc)
../run_gui.sh
```

---

## 可能的原因分析

### 原因1：ConfigParser 崩溃

**症状**：调用 `ConfigParser::create_default_config()` 时崩溃

**解决**：
- 检查 `common/src/config_parser.cpp` 是否正常编译
- 检查 JSON 库是否正确链接

**验证**：
```bash
cd ~/work/CXLMemSim
./build/cxlmemsim --check
```

---

### 原因2：QGraphicsScene 初始化问题

**症状**：创建 `TopologyEditorWidget` 时崩溃

**解决**：临时禁用拓扑编辑器

修改 `mainwindow.cpp` 的 `setupCentralWidget()`：

```cpp
void MainWindow::setupCentralWidget() {
    // 临时使用简单的QWidget替代
    QWidget* placeholder = new QWidget(this);
    placeholder->setStyleSheet("background-color: #f0f0f0;");
    setCentralWidget(placeholder);
    
    // 注释掉原来的代码
    // topologyEditor_ = new TopologyEditorWidget(this);
    // setCentralWidget(topologyEditor_);
}
```

---

### 原因3：信号/槽连接问题

**症状**：调用 `connect()` 时崩溃

**解决**：我已经在代码中注释掉所有信号连接（见 `createConnections()`）

---

### 原因4：子widget构造函数崩溃

**可能崩溃的widget**：
1. `ConfigTreeWidget` - 调用 `create_default_config()`
2. `MetricsPanel` - 创建图表widget
3. `TopologyEditorWidget` - 创建QGraphicsScene
4. `RealTimeChartWidget` - 绘图相关

**调试方法**：逐个禁用

---

## 完整的最小化测试版本

### 创建简化的 main.cpp

```cpp
#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <iostream>

int main(int argc, char *argv[]) {
    std::cout << "Starting minimal Qt test..." << std::endl;
    
    QApplication app(argc, argv);
    
    QMainWindow window;
    window.setWindowTitle("CXLMemSim - Minimal Test");
    window.resize(800, 600);
    
    QLabel* label = new QLabel("If you see this, Qt works!", &window);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("font-size: 24pt;");
    window.setCentralWidget(label);
    
    window.show();
    
    std::cout << "Window shown successfully!" << std::endl;
    
    return app.exec();
}
```

保存为 `frontend/main_minimal.cpp`，然后修改 `frontend/CMakeLists.txt`：

```cmake
# 临时使用最小化版本
add_executable(cxlmemsim_gui_minimal
    main_minimal.cpp
)

target_link_libraries(cxlmemsim_gui_minimal
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
)
```

编译并测试：
```bash
cd ~/work/CXLMemSim/build
make cxlmemsim_gui_minimal
./frontend/cxlmemsim_gui_minimal
```

**如果这个最小版本能运行，说明Qt本身没问题，是我们的代码有问题。**

---

## 常见错误排查

### 错误1：找不到Qt库

```
error while loading shared libraries: libQt6Core.so.6
```

**解决**：
```bash
export LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH
```

---

### 错误2：显示服务器问题

```
qt.qpa.xcb: could not connect to display
```

**解决**：
```bash
export DISPLAY=:0
xhost +local:
```

---

### 错误3：权限问题

```
Authorization required, but no authorization protocol specified
```

**解决**：
```bash
xhost +SI:localuser:$(whoami)
```

---

## 收集诊断信息

请运行以下命令并将输出发给我：

```bash
# 1. 系统信息
uname -a
lsb_release -a

# 2. Qt版本
qmake6 --version || qmake --version

# 3. 编译器版本
g++ --version

# 4. 已安装的Qt包
dpkg -l | grep -E "qt6|qt5"

# 5. 检查可执行文件依赖
ldd ./build/frontend/cxlmemsim_gui | grep -i qt

# 6. 检查是否有未满足的依赖
ldd ./build/frontend/cxlmemsim_gui | grep "not found"

# 7. 运行后端CLI（验证backend是否正常）
./build/cxlmemsim --check
```

---

## 我已经做的修改

### 修改1：调整初始化顺序

**位置**：`frontend/mainwindow.cpp:setupUI()`

**修改**：先创建dock widgets，再创建菜单（因为菜单中访问了dock widgets）

```cpp
setupCentralWidget();  // 第1步
setupDockWidgets();    // 第2步
setupMenuBar();        // 第3步（访问dock widgets）
setupToolBar();        // 第4步
```

---

### 修改2：添加空指针检查

**位置**：多处

**修改**：在访问指针前检查是否为空

```cpp
if (configTree_) {
    configTree_->setConfig(config_);
}
if (logView_) {
    logView_->append("[INFO] ...");
}
```

---

### 修改3：禁用信号连接

**位置**：`frontend/mainwindow.cpp:createConnections()`

**修改**：注释掉所有 `connect()` 调用，避免信号槽问题

---

### 修改4：简化构造函数

**位置**：`frontend/mainwindow.cpp:MainWindow()`

**修改**：
- 添加 try-catch 保护
- 禁用自动加载配置
- 减少初始化操作

---

### 修改5：修复EpochStats字段

**位置**：`frontend/mainwindow.cpp:onExportData()`

**修改**：删除不存在的 `max_latency_ns` 和 `min_latency_ns` 字段

---

## 下一步建议

### 方案A：使用GDB定位（推荐）

1. 运行 `gdb ./build/frontend/cxlmemsim_gui`
2. 输入 `run`
3. 崩溃后输入 `bt full`
4. 将输出发给我

### 方案B：逐步禁用组件

1. 先测试最小化Qt程序（见上面）
2. 逐步恢复我们的组件
3. 找出导致崩溃的具体组件

### 方案C：使用valgrind检测内存错误

```bash
valgrind --leak-check=full --show-leak-kinds=all \
         --track-origins=yes --verbose \
         ./build/frontend/cxlmemsim_gui
```

---

## 临时解决方案

如果实在无法修复GUI，可以：

1. **使用命令行工具**：
   ```bash
   ./build/cxlmemsim --config configs/examples/simple_cxl.json
   ```

2. **运行标准实验**：
   ```bash
   ./scripts/run_experiments.sh
   ```

3. **生成图表**：
   ```bash
   python3 scripts/plot_results.py results/experiment_results.csv
   ```

功能完全可用，只是没有图形界面。

---

## 联系方式

如果按照上述步骤仍无法解决，请提供：

1. GDB调用栈（`bt full` 的输出）
2. 系统信息（上面"收集诊断信息"部分的输出）
3. 最后一次成功运行的版本（如果有）

**我会根据这些信息进一步诊断！**
