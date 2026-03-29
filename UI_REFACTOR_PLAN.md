# UI重构计划 - 工具栏删除与功能迁移

## ✅ 已完成

### 1. 删除整个工具栏
- [x] 删除 `setupToolBar()` 函数实现
- [x] 删除头文件中的按钮成员变量声明
- [x] 删除构造函数初始化列表
- [x] 删除所有按钮引用（onPageChanged等）
- [x] 删除 `onRunExperiments()` 函数

## 🔄 待完成

### 2. 拓扑页面内嵌控制按钮
需要在 `TopologyEditorWidget` 内部添加：
- **开始模拟** 按钮 → 调用 `MainWindow::onStartSimulation()`
- **停止** 按钮 → 调用 `MainWindow::onStopSimulation()`
- **重置** 按钮 → 调用 `MainWindow::onResetSimulation()`

**实现方式**：
```cpp
// 在 topology_editor_widget.h 中添加
signals:
    void startSimulationRequested();
    void stopSimulationRequested();
    void resetSimulationRequested();

// 在 topology_editor_widget.cpp 中添加控制栏
QHBoxLayout* controlBar = new QHBoxLayout();
QPushButton* startBtn = new QPushButton("▶ 开始");
QPushButton* stopBtn = new QPushButton("■ 停止");
QPushButton* resetBtn = new QPushButton("↺ 重置");
// ... 连接信号到主窗口
```

### 3. 侧边栏功能按钮组
在 `SidebarWidget` 底部添加功能按钮：
- **应用拓扑** → 将当前拓扑注入实验系统
- **固定基准** → 固定当前性能数据为基准线
- **导出数据** → 导出实验结果

**实现方式**：
```cpp
// 在 sidebar_widget.h 中添加
signals:
    void applyTopologyRequested();
    void pinBaselineRequested();
    void exportDataRequested();

// 在侧边栏底部添加功能按钮区域
```

### 4. 状态显示迁移
- 状态标签已保留在 `statusLabel_`
- 通过 `statusBar()` 显示
- 无需额外修改

## 📋 编译检查清单
- [ ] 编译通过无错误
- [ ] 拓扑页面显示控制按钮
- [ ] 侧边栏显示功能按钮
- [ ] 信号槽正确连接
- [ ] 功能验证完整

## 🎯 设计原则
1. **上下文感知**：控制按钮仅在拓扑页面可见
2. **功能内聚**：相关功能放在合理位置
3. **视觉简洁**：顶部无冗余工具栏
4. **操作直观**：核心功能触手可及
