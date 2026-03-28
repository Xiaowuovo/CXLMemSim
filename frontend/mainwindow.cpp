/**
 * @file mainwindow.cpp
 * @brief CXLMemSim 主窗口实现
 */

#include "mainwindow.h"
#include "widgets/topology_editor_widget.h"
#include "widgets/config_tree_widget.h"
#include "widgets/metrics_panel.h"
#include "widgets/experiment_panel_widget.h"
#include "widgets/workload_config_widget.h"
#include "widgets/sidebar_widget.h"
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QWidget>
#include <QScrollArea>
#include <QFrame>
#include <QLabel>

#include <QFileDialog>
#include <QMessageBox>
#include <QToolBar>
#include <QMenuBar>
#include <QStatusBar>
#include <QTimer>
#include <QStringList>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStyle>
#include <QApplication>
#include <fstream>
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , sidebar_(nullptr)
    , pageStack_(nullptr)
    , topologyEditor_(nullptr)
    , configTree_(nullptr)
    , workloadWidget_(nullptr)
    , expPanel_(nullptr)
    , metricsPanel_(nullptr)
    , logView_(nullptr)
    , startButton_(nullptr)
    , stopButton_(nullptr)
    , resetButton_(nullptr)
    , statusLabel_(nullptr)
    , analyzer_(nullptr)
    , updateTimer_(nullptr)
    , simulationRunning_(false)
{
    setupStyle();
    setupUI();
    config_ = cxlsim::ConfigParser::create_default_config();
    if (configTree_) configTree_->setConfig(config_);
    statusBar()->setStyleSheet(
        "QStatusBar { background: #000000; color: #888888; border-top: 1px solid #222222; font-size: 11px; }"
    );
    updateStatus("\u5c31\u7eea");
}

MainWindow::~MainWindow() {
    if (updateTimer_) updateTimer_->stop();
    if (simulationRunning_ && analyzer_) analyzer_->stop();
}

void MainWindow::setupStyle() {
    qApp->setStyle("Fusion");
    
    // Vercel / Linear 风格的现代化暗黑主题
    QString modernTheme = R"(
        /* 全局基础设定 */
        QMainWindow { background-color: #000000; }
        QWidget { color: #EDEDED; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; font-size: 13px; }
        
        /* 菜单栏 - 修复顶部白色背景 */
        QMenuBar { background-color: #0A0A0A; color: #EDEDED; border-bottom: 1px solid #1A1A1A; padding: 4px 8px; }
        QMenuBar::item { background-color: transparent; padding: 6px 12px; border-radius: 4px; }
        QMenuBar::item:selected { background-color: #1A1A1A; color: #FFFFFF; }
        QMenuBar::item:pressed { background-color: #222222; }
        
        /* 下拉菜单 */
        QMenu { background-color: #0A0A0A; color: #EDEDED; border: 1px solid #222222; border-radius: 6px; padding: 4px; }
        QMenu::item { background-color: transparent; padding: 8px 24px 8px 12px; border-radius: 4px; }
        QMenu::item:selected { background-color: #1A1A1A; color: #FFFFFF; }
        QMenu::separator { height: 1px; background: #222222; margin: 4px 8px; }
        
        /* 工具栏 (克制的分隔线) */
        QToolBar { background-color: #000000; border-bottom: 1px solid #222222; padding: 6px; spacing: 8px; }
        QToolBar::separator { background-color: #333333; width: 1px; margin: 4px 8px; }
        
        /* 面板停靠区 (干净的标题栏) */
        QDockWidget { color: #888888; font-weight: 500; }
        QDockWidget::title { background-color: #0A0A0A; padding: 8px 12px; font-size: 12px; border-bottom: 1px solid #222222; }
        
        /* 按钮体系 (克制的交互) */
        QPushButton { background-color: #111111; color: #EDEDED; border: 1px solid #333333; border-radius: 6px; padding: 6px 16px; font-weight: 500; outline: none; }
        QPushButton:hover { background-color: #1A1A1A; border-color: #444444; color: #FFFFFF; }
        QPushButton:pressed { background-color: #222222; border-color: #555555; }
        QPushButton:disabled { background-color: #0A0A0A; color: #444444; border-color: #222222; }
        
        /* 状态按钮 (Semantic Colors) */
        QPushButton#startBtn { background-color: #000000; border: 1px solid #1A4D2E; color: #4ADE80; }
        QPushButton#startBtn:hover { background-color: #052E16; border-color: #22C55E; }
        QPushButton#stopBtn { background-color: #000000; border: 1px solid #7F1D1D; color: #F87171; }
        QPushButton#stopBtn:hover { background-color: #450A0A; border-color: #EF4444; }
        
        /* 树形列表 (优雅的层级) */
        QTreeWidget, QListWidget { background-color: transparent; color: #EDEDED; border: none; outline: none; }
        QTreeWidget::item, QListWidget::item { padding: 4px; border-radius: 4px; margin: 1px 4px; background: transparent; }
        QTreeWidget::item:selected, QListWidget::item:selected { background-color: #222222; color: #FFFFFF; }
        QTreeWidget::item:hover:!selected, QListWidget::item:hover:!selected { background-color: #111111; }
        QHeaderView::section { background-color: #0A0A0A; color: #888888; padding: 6px; border: none; border-bottom: 1px solid #222222; font-weight: 500; text-align: left; }
        
        /* 标签页 (现代化的底部下划线设计) */
        QTabWidget::pane { border: none; border-top: 1px solid #222222; background-color: #000000; }
        QTabBar::tab { background-color: transparent; color: #888888; padding: 10px 20px; border: none; border-bottom: 2px solid transparent; font-weight: 500; }
        QTabBar::tab:selected { color: #FFFFFF; border-bottom: 2px solid #EDEDED; }
        QTabBar::tab:hover:!selected { color: #EDEDED; background-color: #0A0A0A; }
        
        /* 滚动条 (纤细不可见感) */
        QScrollBar:vertical { background-color: transparent; width: 12px; margin: 0px; }
        QScrollBar::handle:vertical { background-color: #333333; border-radius: 6px; min-height: 24px; margin: 2px; }
        QScrollBar::handle:vertical:hover { background-color: #555555; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }
        QScrollBar:horizontal { background-color: transparent; height: 12px; margin: 0px; }
        QScrollBar::handle:horizontal { background-color: #333333; border-radius: 6px; min-width: 24px; margin: 2px; }
        QScrollBar::handle:horizontal:hover { background-color: #555555; }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }
        QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: none; }
        
        /* GroupBox统一样式 */
        QGroupBox { color: #888888; font-size: 11px; font-weight: bold; letter-spacing: 1px; border: none; padding-top: 16px; }
        QGroupBox::title { subcontrol-origin: margin; left: 0px; padding: 0px; }
        
        /* 文本区域与输入框 */
        QTextEdit { background-color: #050505; color: #A1A1AA; border: 1px solid #222222; border-radius: 6px; font-family: "JetBrains Mono", "Fira Code", Consolas, monospace; font-size: 12px; padding: 4px; }
        QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox { background-color: #0A0A0A; color: #EDEDED; border: 1px solid #333333; border-radius: 6px; padding: 6px 10px; }
        QLineEdit:focus, QSpinBox:focus, QComboBox:focus { border-color: #666666; background-color: #111111; }
        
        /* QComboBox 下拉箭头和列表（修复白色背景问题） */
        QComboBox::drop-down { border: none; background: transparent; width: 20px; }
        QComboBox::down-arrow { image: none; border-left: 4px solid transparent; border-right: 4px solid transparent; border-top: 6px solid #888888; width: 0; height: 0; margin-right: 6px; }
        QComboBox::down-arrow:hover { border-top-color: #EDEDED; }
        QComboBox QAbstractItemView { background-color: #0A0A0A; color: #EDEDED; border: 1px solid #333333; selection-background-color: #1A1A1A; selection-color: #FFFFFF; outline: none; }
        QComboBox QAbstractItemView::item { padding: 6px 10px; min-height: 24px; }
        QComboBox QAbstractItemView::item:hover { background-color: #111111; }
        QComboBox QAbstractItemView::item:selected { background-color: #1A1A1A; }
        
        /* QSpinBox/QDoubleSpinBox 上下箭头 */
        QSpinBox::up-button, QDoubleSpinBox::up-button { background: transparent; border: none; width: 16px; }
        QSpinBox::down-button, QDoubleSpinBox::down-button { background: transparent; border: none; width: 16px; }
        QSpinBox::up-arrow, QDoubleSpinBox::up-arrow { border-left: 4px solid transparent; border-right: 4px solid transparent; border-bottom: 5px solid #888888; width: 0; height: 0; }
        QSpinBox::down-arrow, QDoubleSpinBox::down-arrow { border-left: 4px solid transparent; border-right: 4px solid transparent; border-top: 5px solid #888888; width: 0; height: 0; }
        QSpinBox::up-arrow:hover, QDoubleSpinBox::up-arrow:hover { border-bottom-color: #EDEDED; }
        QSpinBox::down-arrow:hover, QDoubleSpinBox::down-arrow:hover { border-top-color: #EDEDED; }
        
        /* 状态栏 */
        QStatusBar { background-color: #000000; color: #888888; border-top: 1px solid #222222; }
    )";

    qApp->setStyleSheet(modernTheme);
}

void MainWindow::setupUI() {
    setWindowTitle("CXLMemSim - CXL 内存模拟系统");
    resize(1600, 900);
    setMinimumSize(1200, 700);

    setupMenuBar();
    setupToolBar();
    setupSidebar();
    setupPages();
    createConnections();

    statusBar()->showMessage("就绪 - CXLMemSim v1.0");
}

void MainWindow::setupMenuBar() {
    QMenu* fileMenu = menuBar()->addMenu("\u6587\u4ef6(&F)");
    
    QAction* newAction = fileMenu->addAction("\u65b0\u5efa\u914d\u7f6e(&N)", this, &MainWindow::onNewConfig);
    newAction->setShortcut(QKeySequence::New);
    
    QAction* openAction = fileMenu->addAction("\u6253\u5f00\u914d\u7f6e(&O)...", this, &MainWindow::onOpenConfig);
    openAction->setShortcut(QKeySequence::Open);
    
    QAction* saveAction = fileMenu->addAction("\u4fdd\u5b58\u914d\u7f6e(&S)", this, &MainWindow::onSaveConfig);
    saveAction->setShortcut(QKeySequence::Save);
    
    QAction* saveAsAction = fileMenu->addAction("\u53e6\u5b58\u4e3a(&A)...", this, &MainWindow::onSaveConfigAs);
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    
    fileMenu->addSeparator();
    fileMenu->addAction("\u5bfc\u51fa\u62d3\u6251\u56fe(&T)...", this, &MainWindow::onExportTopology);
    fileMenu->addAction("\u5bfc\u51fa\u5b9e\u9a8c\u6570\u636e(&E)...", this, &MainWindow::onExportData);
    fileMenu->addSeparator();
    
    QAction* exitAction = fileMenu->addAction("\u9000\u51fa(&X)", this, &MainWindow::onExit);
    exitAction->setShortcut(QKeySequence::Quit);

    QMenu* simMenu = menuBar()->addMenu("\u6a21\u62df(&S)");
    
    QAction* startAction = simMenu->addAction("\u5f00\u59cb\u6a21\u62df [F5]", this, &MainWindow::onStartSimulation);
    startAction->setShortcut(Qt::Key_F5);
    
    QAction* stopAction = simMenu->addAction("\u505c\u6b62\u6a21\u62df [F6]", this, &MainWindow::onStopSimulation);
    stopAction->setShortcut(Qt::Key_F6);
    
    simMenu->addAction("\u91cd\u7f6e\u6a21\u62df(&R)", this, &MainWindow::onResetSimulation);
    simMenu->addSeparator();
    simMenu->addAction("\u8fd0\u884c\u9884\u8bbe\u5b9e\u9a8c(&E)...", this, &MainWindow::onRunExperiments);

    QMenu* viewMenu = menuBar()->addMenu("\u89c6\u56fe(&V)");
    viewMenu->addAction("\u62d3\u6251\u7f16\u8f91", this, [this]() { if(sidebar_) sidebar_->setActivePage(SidebarWidget::TOPOLOGY); });
    viewMenu->addAction("\u7cfb\u7edf\u914d\u7f6e", this, [this]() { if(sidebar_) sidebar_->setActivePage(SidebarWidget::CONFIG); });
    viewMenu->addAction("\u8d1f\u8f7d\u914d\u7f6e", this, [this]() { if(sidebar_) sidebar_->setActivePage(SidebarWidget::WORKLOAD); });
    viewMenu->addAction("\u5b9e\u9a8c\u7ba1\u7406", this, [this]() { if(sidebar_) sidebar_->setActivePage(SidebarWidget::EXPERIMENT); });
    viewMenu->addAction("\u6027\u80fd\u6307\u6807", this, [this]() { if(sidebar_) sidebar_->setActivePage(SidebarWidget::METRICS); });
    viewMenu->addAction("\u8fd0\u884c\u65e5\u5fd7", this, [this]() { if(sidebar_) sidebar_->setActivePage(SidebarWidget::LOG); });

    QMenu* helpMenu = menuBar()->addMenu("\u5e2e\u52a9(&H)");
    helpMenu->addAction("\u4f7f\u7528\u6587\u6863(&D)", this, &MainWindow::onShowDocs);
    helpMenu->addAction("\u5173\u4e8e(&A)", this, &MainWindow::onAbout);
}

void MainWindow::setupToolBar() {
    QToolBar* toolbar = addToolBar("\u4e3b\u5de5\u5177\u680f");
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(16, 16));

    startButton_ = new QPushButton(" \u25b6  \u5f00\u59cb\u6a21\u62df");
    startButton_->setObjectName("startBtn");
    startButton_->setMinimumWidth(110);
    toolbar->addWidget(startButton_);

    stopButton_ = new QPushButton(" \u25a0  \u505c\u6b62");
    stopButton_->setObjectName("stopBtn");
    stopButton_->setEnabled(false);
    stopButton_->setMinimumWidth(90);
    toolbar->addWidget(stopButton_);

    resetButton_ = new QPushButton(" \u21ba  \u91cd\u7f6e");
    resetButton_->setMinimumWidth(90);
    toolbar->addWidget(resetButton_);

    toolbar->addSeparator();

    QPushButton* runExpBtn = new QPushButton(" \u2605  \u8fd0\u884c\u5b9e\u9a8c");
    runExpBtn->setMinimumWidth(100);
    runExpBtn->setStyleSheet("QPushButton{background-color:#4A148C;border-color:#CE93D8;color:#F3E5F5;}"
                             "QPushButton:hover{background-color:#6A1B9A;}");

    toolbar->addWidget(runExpBtn);

    QPushButton* injectTopoBtn = new QPushButton(" \u27a1  \u5e94\u7528\u62d3\u6251\u5230\u5b9e\u9a8c");
    injectTopoBtn->setMinimumWidth(130);
    injectTopoBtn->setToolTip("将当前拓扑图的参数注入到实验系统中");
    injectTopoBtn->setStyleSheet(
        "QPushButton{background-color:#0D3B5C;border:1px solid #29B6F6;color:#81D4FA;border-radius:4px;}"
        "QPushButton:hover{background-color:#1565C0;border-color:#4FC3F7;}"
        "QPushButton:pressed{background-color:#0288D1;}");

    toolbar->addWidget(injectTopoBtn);

    toolbar->addSeparator();

    // ── 科研多组对比按钮 ──
    QPushButton* pinBaselineBtn = new QPushButton(" \u2318  固定基准");
    pinBaselineBtn->setMinimumWidth(100);
    pinBaselineBtn->setToolTip("将当前实验数据固定为基准线，用于多组对比（控制变量法）");
    pinBaselineBtn->setStyleSheet(
        "QPushButton{background-color:#1E40AF;border:1px solid #3B82F6;color:#DBEAFE;border-radius:4px;}"
        "QPushButton:hover{background-color:#1D4ED8;border-color:#60A5FA;}"
        "QPushButton:pressed{background-color:#2563EB;}");

    toolbar->addWidget(pinBaselineBtn);
    connect(pinBaselineBtn, &QPushButton::clicked, this, [this]() {
        if (metricsPanel_) {
            metricsPanel_->pinCurrentAsBaseline();
            updateStatus("✓ 当前数据已固定为基准，可运行新实验对比");
            if (logView_) logView_->append("[INFO] 📌 基准曲线已固定，可修改拓扑/负载后重新运行对比");
        }
    });

    QPushButton* clearBaselineBtn = new QPushButton(" 🗑  清除基准");
    clearBaselineBtn->setMinimumWidth(100);
    clearBaselineBtn->setToolTip("清除基准线，恢复单组显示");
    clearBaselineBtn->setStyleSheet(
        "QPushButton{background-color:#7C2D12;border:1px solid #EA580C;color:#FED7AA;border-radius:4px;}"
        "QPushButton:hover{background-color:#9A3412;border-color:#FB923C;}"
        "QPushButton:pressed{background-color:#C2410C;}");
    toolbar->addWidget(clearBaselineBtn);
    connect(clearBaselineBtn, &QPushButton::clicked, this, [this]() {
        if (metricsPanel_) {
            metricsPanel_->clearBaseline();
            updateStatus("基准曲线已清除");
            if (logView_) logView_->append("[INFO] 🗑 基准曲线已清除");
        }
    });

    toolbar->addSeparator();

    QPushButton* exportDataBtn = new QPushButton(" 📊  导出实验数据");
    exportDataBtn->setMinimumWidth(130);
    exportDataBtn->setToolTip("导出实验结果为 CSV/JSON 用于论文绘图 (Python/MATLAB)");
    exportDataBtn->setStyleSheet(
        "QPushButton{background-color:#065F46;border:1px solid #10B981;color:#6EE7B7;border-radius:4px;font-weight:bold;}"
        "QPushButton:hover{background-color:#047857;border-color:#34D399;}"
        "QPushButton:pressed{background-color:#059669;}");
    toolbar->addWidget(exportDataBtn);
    connect(exportDataBtn, &QPushButton::clicked, this, &MainWindow::onExportData);

    toolbar->addSeparator();

    QLabel* sep = new QLabel("  \u72b6\u6001: ");
    sep->setStyleSheet("color:#9E9E9E;");
    toolbar->addWidget(sep);

    statusLabel_ = new QLabel("\u5c31\u7eea");
    statusLabel_->setStyleSheet("color:#81C784; font-weight:bold; font-size:12px;");
    toolbar->addWidget(statusLabel_);

    connect(startButton_,  &QPushButton::clicked, this, &MainWindow::onStartSimulation);
    connect(stopButton_,   &QPushButton::clicked, this, &MainWindow::onStopSimulation);
    connect(resetButton_,  &QPushButton::clicked, this, &MainWindow::onResetSimulation);
    connect(runExpBtn,     &QPushButton::clicked, this, &MainWindow::onRunExperiments);
    connect(injectTopoBtn, &QPushButton::clicked, this, [this]() {
        if (topologyEditor_ && expPanel_ && sidebar_) {
            auto cfg = topologyEditor_->getCurrentConfig();
            expPanel_->injectTopology(cfg);
            // 切换到实验页面
            sidebar_->setActivePage(SidebarWidget::EXPERIMENT);
            updateStatus("拓扑已注入实验系统");
        }
    });
}

void MainWindow::setupSidebar() {
    sidebar_ = new SidebarWidget(this);
}

void MainWindow::setupPages() {
    // 创建中央容器
    auto* centralWidget = new QWidget(this);
    auto* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 添加侧边栏
    mainLayout->addWidget(sidebar_);

    // 创建页面堆叠
    pageStack_ = new QStackedWidget(this);
    pageStack_->setStyleSheet("QStackedWidget { background: #000000; border: none; }");

    // ═══════════════════════════════════════════════════════════════
    // 页面0: 拓扑编辑（带右侧性能面板）
    // ═══════════════════════════════════════════════════════════════
    auto* topoPage = new QWidget(this);
    auto* topoLayout = new QHBoxLayout(topoPage);
    topoLayout->setContentsMargins(0, 0, 0, 0);
    topoLayout->setSpacing(0);

    topologyEditor_ = new TopologyEditorWidget(topoPage);
    topoLayout->addWidget(topologyEditor_, 1);

    // 右侧性能面板（紧凑版）
    auto* metricsContainer = new QWidget(topoPage);
    metricsContainer->setFixedWidth(320);
    metricsContainer->setStyleSheet("QWidget { background: #000000; border-left: 1px solid #1A1A1A; }");
    auto* metricsLayout = new QVBoxLayout(metricsContainer);
    metricsLayout->setContentsMargins(0, 0, 0, 0);
    
    metricsPanel_ = new MetricsPanel(metricsContainer);
    metricsLayout->addWidget(metricsPanel_);
    
    topoLayout->addWidget(metricsContainer);
    pageStack_->addWidget(topoPage);

    // ═══════════════════════════════════════════════════════════════
    // 页面1: 系统配置
    // ═══════════════════════════════════════════════════════════════
    auto* cfgScroll = new QScrollArea(this);
    cfgScroll->setWidgetResizable(true);
    cfgScroll->setFrameShape(QFrame::NoFrame);
    cfgScroll->setStyleSheet("QScrollArea { background: #000000; border: none; }");
    
    configTree_ = new ConfigTreeWidget(cfgScroll);
    cfgScroll->setWidget(configTree_);
    pageStack_->addWidget(cfgScroll);

    // ═══════════════════════════════════════════════════════════════
    // 页面2: 负载配置
    // ═══════════════════════════════════════════════════════════════
    auto* wlScroll = new QScrollArea(this);
    wlScroll->setWidgetResizable(true);
    wlScroll->setFrameShape(QFrame::NoFrame);
    wlScroll->setStyleSheet("QScrollArea { background: #000000; border: none; }");
    
    workloadWidget_ = new WorkloadConfigWidget(wlScroll);
    wlScroll->setWidget(workloadWidget_);
    pageStack_->addWidget(wlScroll);

    // ═══════════════════════════════════════════════════════════════
    // 页面3: 实验管理
    // ═══════════════════════════════════════════════════════════════
    expPanel_ = new ExperimentPanelWidget(this);
    pageStack_->addWidget(expPanel_);

    // ═══════════════════════════════════════════════════════════════
    // 页面4: 性能指标（独立全屏视图）
    // ═══════════════════════════════════════════════════════════════
    auto* metricsFullPage = new QWidget(this);
    auto* metricsFullLayout = new QVBoxLayout(metricsFullPage);
    metricsFullLayout->setContentsMargins(24, 24, 24, 24);
    
    auto* metricsTitle = new QLabel("📊 性能监控面板", metricsFullPage);
    metricsTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #EDEDED; margin-bottom: 12px;");
    metricsFullLayout->addWidget(metricsTitle);
    
    // 这里可以添加更详细的性能监控组件，暂时显示提示
    auto* hint = new QLabel("实时性能数据已集成在拓扑编辑页面右侧。\n切换到拓扑页面查看实时指标。", metricsFullPage);
    hint->setStyleSheet("color: #888888; font-size: 13px; padding: 24px;");
    hint->setAlignment(Qt::AlignCenter);
    metricsFullLayout->addWidget(hint, 1);
    pageStack_->addWidget(metricsFullPage);

    // ═══════════════════════════════════════════════════════════════
    // 页面5: 运行日志
    // ═══════════════════════════════════════════════════════════════
    auto* logPage = new QWidget(this);
    auto* logLayout = new QVBoxLayout(logPage);
    logLayout->setContentsMargins(12, 12, 12, 12);
    
    auto* logTitle = new QLabel("📝 运行日志", logPage);
    logTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #EDEDED; margin-bottom: 8px;");
    logLayout->addWidget(logTitle);
    
    logView_ = new QTextEdit(logPage);
    logView_->setReadOnly(true);
    logLayout->addWidget(logView_);
    pageStack_->addWidget(logPage);

    // 添加页面堆叠到布局
    mainLayout->addWidget(pageStack_, 1);

    setCentralWidget(centralWidget);
    
    // 默认显示拓扑编辑页面
    pageStack_->setCurrentIndex(0);
}

void MainWindow::onPageChanged(int pageIndex) {
    if (pageStack_) {
        pageStack_->setCurrentIndex(pageIndex);
        
        // 更新状态栏提示
        QStringList pageNames = {"拓扑编辑", "系统配置", "负载配置", "实验管理", "性能指标", "运行日志"};
        if (pageIndex >= 0 && pageIndex < pageNames.size()) {
            updateStatus(QString("当前页面: %1").arg(pageNames[pageIndex]));
        }
    }
}

void MainWindow::createConnections() {
    // ── 侧边栏页面切换 ──
    if (sidebar_) {
        connect(sidebar_, &SidebarWidget::pageChanged, this, &MainWindow::onPageChanged);
    }

    // ── 配置树 -> 拓扑图 ──
    if (configTree_ && topologyEditor_) {
        connect(configTree_, &ConfigTreeWidget::configChanged,
                topologyEditor_, &TopologyEditorWidget::updateTopology);
    }

    // ── 拓扑图修改 -> 更新配置树 ──
    if (topologyEditor_ && configTree_) {
        connect(topologyEditor_, &TopologyEditorWidget::topologyModified,
                this, [this]() {
                    config_ = topologyEditor_->getCurrentConfig();
                    configTree_->setConfig(config_);
                    updateStatus("\u62d3\u6251\u56fe\u5df2\u4fee\u6539");
                    if (logView_) logView_->append("[INFO] \u62d3\u6251\u56fe\u5df2\u4fee\u6539\uff0c\u914d\u7f6e\u5df2\u540c\u6b65");
                });
    }

    // ── 实验面板日志转发 ──
    if (expPanel_ && logView_) {
        connect(expPanel_, &ExperimentPanelWidget::logMessage,
                logView_, &QTextEdit::append);
    }

    // ── 实验完成时切换到实验页面 ──
    if (expPanel_ && sidebar_) {
        connect(expPanel_, &ExperimentPanelWidget::resultsReady,
                this, [this]() {
                    sidebar_->setActivePage(SidebarWidget::EXPERIMENT);
                    updateStatus("\u5b9e\u9a8c\u5b8c\u6210\uff01\u5df2\u751f\u6210\u7ed3\u679c\u56fe\u8868");
                });
    }

    // ── 定时更新指标 ──
    updateTimer_ = new QTimer(this);
    connect(updateTimer_, &QTimer::timeout, this, &MainWindow::updateMetrics);
    updateTimer_->start(1000);
}

// ...
// \u6587\u4ef6\u83dc\u5355\u69fd\u51fd\u6570
void MainWindow::onNewConfig() {
    config_ = cxlsim::ConfigParser::create_default_config();
    currentConfigFile_.clear();
    if (configTree_) configTree_->setConfig(config_);
    if (topologyEditor_) topologyEditor_->updateTopology(config_);
    updateStatus("\u5df2\u65b0\u5efa\u9ed8\u8ba4\u914d\u7f6e");
    if (logView_) logView_->append("[INFO] \u65b0\u5efa\u9ed8\u8ba4CXL\u914d\u7f6e");
}

void MainWindow::onOpenConfig() {
    QString filename = QFileDialog::getOpenFileName(
        this, "\u6253\u5f00\u914d\u7f6e\u6587\u4ef6", "",
        "JSON \u6587\u4ef6 (*.json);;All Files (*)");
    if (!filename.isEmpty()) loadConfig(filename);
}

void MainWindow::onSaveConfig() {
    if (currentConfigFile_.isEmpty()) onSaveConfigAs();
    else saveConfig(currentConfigFile_);
}

void MainWindow::onSaveConfigAs() {
    QString filename = QFileDialog::getSaveFileName(
        this, "\u4fdd\u5b58\u914d\u7f6e\u6587\u4ef6", "config.json",
        "JSON \u6587\u4ef6 (*.json);;All Files (*)");
    if (!filename.isEmpty()) {
        saveConfig(filename);
        currentConfigFile_ = filename;
    }
}

void MainWindow::onExportTopology() {
    if (!topologyEditor_) return;
    QString filename = QFileDialog::getSaveFileName(
        this, "\u5bfc\u51fa\u62d3\u6251\u56fe", "topology.png",
        "\u56fe\u7247\u6587\u4ef6 (*.png *.jpg)");
    if (!filename.isEmpty()) {
        topologyEditor_->exportToImage(filename);
        updateStatus("\u62d3\u6251\u56fe\u5df2\u5bfc\u51fa: " + filename);
        if (logView_) logView_->append("[INFO] \u5bfc\u51fa\u62d3\u6251\u56fe: " + filename);
    }
}

void MainWindow::onExit() { close(); }

// \u6a21\u62df\u63a7\u5236\u69fd\u51fd\u6570
void MainWindow::onStartSimulation() {
    // ── 状态机前置校验（科研可信度关键）──
    
    // 1. 拓扑完整性验证
    if (config_.cxl_devices.empty()) {
        QMessageBox::warning(this, "⚠️ 拓扑不完整",
            "未配置任何 CXL 设备。\n\n"
            "请在拓扑图中添加至少一个 CXL_MEM 设备后再启动模拟。");
        if (logView_) logView_->append("[WARN] 拓扑验证失败：无CXL设备");
        return;
    }
    
    if (config_.connections.empty()) {
        QMessageBox::warning(this, "⚠️ 拓扑不完整",
            "拓扑图中没有任何连接。\n\n"
            "请将 Root Complex、Switch 和 CXL 设备连接后再启动。");
        if (logView_) logView_->append("[WARN] 拓扑验证失败：无连接");
        return;
    }
    
    // 2. 负载配置验证（核心）
    if (workloadWidget_ && !workloadWidget_->isWorkloadValid()) {
        QString error = workloadWidget_->getValidationError();
        QMessageBox::critical(this, "❌ 负载配置错误",
            QString("无法启动模拟：%1\n\n"
                    "科研提示：没有负载注入，仿真引擎将没有任何内存访问事件，\n"
                    "Epoch 会保持为 0，所有指标为空。\n\n"
                    "请先在左侧 '🚀 负载配置' 面板中配置 Synthetic 或 Trace-Driven 负载。").arg(error));
        if (logView_) logView_->append(QString("[ERROR] 负载验证失败: %1").arg(error));
        return;
    }
    
    // 3. 同步负载配置到主配置
    if (workloadWidget_) {
        config_.workload = workloadWidget_->getWorkloadConfig();
        if (logView_) {
            if (config_.workload.trace_driven) {
                logView_->append(QString("[INFO] 负载模式: Trace-Driven (%1)")
                    .arg(QString::fromStdString(config_.workload.trace_file_path)));
            } else {
                logView_->append(QString("[INFO] 负载模式: Synthetic (%1, %.1f GB/s, %2 threads)")
                    .arg(config_.workload.access_pattern == cxlsim::AccessPattern::RANDOM ? "Random" : "Sequential")
                    .arg(config_.workload.injection_rate_gbps)
                    .arg(config_.workload.num_threads));
            }
        }
    }

    if (logView_) logView_->append("[INFO] ✓ 拓扑完整性验证通过");
    if (logView_) logView_->append("[INFO] ✓ 负载配置验证通过");
    if (logView_) logView_->append("[INFO] 正在初始化模拟引擎...");

    if (!analyzer_) {
        analyzer_ = std::make_unique<cxlsim::TimingAnalyzer>();
        if (!analyzer_->initialize(config_)) {
            QMessageBox::critical(this, "错误", 
                "模拟引擎初始化失败。\n\n可能原因：\n"
                "• 拓扑配置参数不合法\n"
                "• 负载配置超出硬件限制\n"
                "• 内部资源分配失败\n\n"
                "请检查配置参数后重试。");
            if (logView_) logView_->append("[ERROR] 引擎初始化失败");
            analyzer_.reset();
            return;
        }
    }

    simulationRunning_ = true;
    if (startButton_) startButton_->setEnabled(false);
    if (stopButton_)  stopButton_->setEnabled(true);
    if (statusLabel_) {
        statusLabel_->setText("运行中");
        statusLabel_->setStyleSheet("color:#FF8A65; font-weight:bold; font-size:12px;");
    }
    updateStatus("模拟运行中...");
    if (logView_) logView_->append("[INFO] ✅ CXL 内存模拟已启动 (Epoch 即将开始跳动)");
}

void MainWindow::onStopSimulation() {
    if (analyzer_) analyzer_->stop();
    simulationRunning_ = false;
    if (startButton_) startButton_->setEnabled(true);
    if (stopButton_)  stopButton_->setEnabled(false);
    if (statusLabel_) {
        statusLabel_->setText("\u5df2\u505c\u6b62");
        statusLabel_->setStyleSheet("color:#EF5350; font-weight:bold; font-size:12px;");
    }
    if (topologyEditor_) topologyEditor_->clearAllMetrics();
    updateStatus("\u6a21\u62df\u5df2\u505c\u6b62");
    if (logView_) logView_->append("[INFO] \u6a21\u62df\u5df2\u505c\u6b62");
}

void MainWindow::onResetSimulation() {
    onStopSimulation();
    analyzer_.reset();
    if (metricsPanel_) metricsPanel_->reset();
    if (statusLabel_) {
        statusLabel_->setText("\u5c31\u7eea");
        statusLabel_->setStyleSheet("color:#81C784; font-weight:bold; font-size:12px;");
    }
    updateStatus("\u6a21\u62df\u5df2\u91cd\u7f6e");
    if (logView_) logView_->append("[INFO] \u6a21\u62df\u5df2\u91cd\u7f6e\uff0c\u53ef\u91cd\u65b0\u8fd0\u884c");
}

void MainWindow::onRunExperiments() {
    if (sidebar_) {
        sidebar_->setActivePage(SidebarWidget::EXPERIMENT);
    }
    updateStatus("已切换到实验管理页面");
    if (logView_) logView_->append("[INFO] 请在实验管理页面选择并运行实验");
}

void MainWindow::onAbout() {
    QMessageBox::about(this, "\u5173\u4e8e CXLMemSim",
        "<h2 style='color:#4FC3F7;'>CXLMemSim v1.0.0</h2>"
        "<p><b>CXL \u5185\u5b58\u6a21\u62df\u7cfb\u7edf</b></p>"
        "<p>\u9ad8\u4ef7\u5024\u6267\u884c\u9a71\u52a8\u7684 CXL \u5185\u5b58\u7cfb\u7edf\u6a21\u62df\u5668\uff0c\u652f\u6301\u5b9e\u9a8c\u7ea7\u5ef6\u8fdf\u5efa\u6a21\u3002</p>"
        "<hr/>"
        "<p><b>\u6838\u5fc3\u529f\u80fd\uff1a</b></p>"
        "<ul>"
        "<li>\u7cbe\u786e\u5ef6\u8fdf\u5efa\u6a21\uff08CXL 1.1/2.0/3.0\uff09</li>"
        "<li>CXL \u62d3\u6251\u53ef\u89c6\u5316\u7f16\u8f91\u5668</li>"
        "<li>\u5b9e\u65f6\u6027\u80fd\u6307\u6807\u76d1\u63a7</li>"
        "<li>\u62e5\u585e\u6a21\u578b\u4e0e MLP \u4f18\u5316</li>"
        "<li>13 \u7ec4\u9884\u8bbe\u79d1\u7814\u5b9e\u9a8c\u96c6 (MLC/LLM/HPC)</li>"
        "<li>\u5185\u7f6e\u5b9e\u9a8c\u7ed3\u679c\u53ef\u89c6\u5316</li>"
        "</ul>"
        "<hr/>"
        "<p style='color:#9E9E9E;'>\u57fa\u4e8e Qt6 + C++17 \u5f00\u53d1</p>");
}

void MainWindow::onShowDocs() {
    QMessageBox::information(this, "\u4f7f\u7528\u6587\u6863",
        "<h3 style='color:#4FC3F7;'>CXLMemSim \u6587\u6863\u7d22\u5f15</h3>"
        "<p><b>\u6838\u5fc3\u6587\u6863\uff1a</b></p>"
        "<ul>"
        "<li><b>README.md</b> \u2014 \u9879\u76ee\u6982\u89c8\u4e0e\u5feb\u901f\u5f00\u59cb</li>"
        "<li><b>QUICKSTART.md</b> \u2014 \u8be6\u7ec6\u5feb\u901f\u5165\u95e8\u6307\u5357</li>"
        "<li><b>\u6bd5\u8bbe\u64b0\u5199\u6307\u5bfc\u8bf4\u660e\u4e66.md</b> \u2014 \u8bba\u6587\u64b0\u5199\u6307\u5357</li>"
        "</ul>"
        "<p><b>\u5b9e\u9a8c\u914d\u7f6e\uff1a</b></p>"
        "<ul>"
        "<li><b>configs/examples/simple_cxl.json</b> \u2014 \u7b80\u5355\u793a\u4f8b</li>"
        "<li><b>configs/standard_experiments.json</b> \u2014 11 \u7ec4\u6807\u51c6\u5b9e\u9a8c</li>"
        "</ul>"
        "<p><b>\u5982\u4f55\u8fd0\u884c\u5b9e\u9a8c\uff1a</b></p>"
        "<p>\u5728\u5e95\u90e8\u300a\u9884\u8bbe\u5b9e\u9a8c\u7ba1\u7406\u300b\u9762\u677f\u4e2d\u9009\u62e9\u5b9e\u9a8c\uff0c\u70b9\u51fb\u300a\u8fd0\u884c\u9009\u4e2d\u5b9e\u9a8c\u300b\u5373\u53ef\u76f4\u63a5\u8fd0\u884c\u5e76\u67e5\u770b\u7ed3\u679c\u56fe\u8868\u3002</p>"
    );
    if (logView_) logView_->append("[INFO] \u5f00\u542f\u6587\u6863\u5e2e\u52a9");
}

// \u8f85\u52a9\u65b9\u6cd5
void MainWindow::updateStatus(const QString& message) {
    if (statusLabel_) statusLabel_->setText(message);
    statusBar()->showMessage(message, 4000);
}

void MainWindow::updateMetrics() {
    if (!simulationRunning_ || !analyzer_ || !metricsPanel_) return;
    const auto& stats = analyzer_->get_current_stats();
    metricsPanel_->updateStats(stats);

    // 实时推送指标到拓扑节点覆盖层
    if (topologyEditor_) {
        auto cfg = topologyEditor_->getCurrentConfig();

        // Root Complex 节点显示整体访问延迟/带宽
        if (!cfg.root_complex.id.empty()) {
            DeviceMetrics m;
            m.active        = true;
            m.latency_ns    = stats.avg_latency_ns;
            // 带宽估算：CXL访问次数 × 64B cache line / 1秒
            double cxl_bw = cfg.cxl_devices.empty() ? 64.0
                            : cfg.cxl_devices[0].bandwidth_gbps;
            m.bandwidth_gbps = (stats.cxl_accesses > 0)
                ? std::min(cxl_bw, 64.0 * stats.cxl_accesses / 1e9)
                : 0.0;
            m.load_pct = (stats.total_accesses > 0)
                ? std::min(100.0, 100.0 * stats.cxl_accesses / stats.total_accesses * 3.0)
                : 0.0;
            topologyEditor_->updateDeviceMetrics(
                QString::fromStdString(cfg.root_complex.id), m);
        }

        // CXL 设备节点显示 CXL 专项指标
        for (size_t i = 0; i < cfg.cxl_devices.size(); ++i) {
            DeviceMetrics dm;
            dm.active         = true;
            double idx = static_cast<double>(i);
            dm.latency_ns     = stats.avg_latency_ns * (1.0 + 0.1 * idx);
            dm.bandwidth_gbps = cfg.cxl_devices[i].bandwidth_gbps
                                * (stats.cxl_accesses > 0 ? 0.7 : 0.0);
            dm.load_pct       = (stats.cxl_accesses > 0)
                ? std::min(100.0, 60.0 + 10.0 * idx)
                : 0.0;
            topologyEditor_->updateDeviceMetrics(
                QString::fromStdString(cfg.cxl_devices[i].id), dm);
            
            // 推送链路实时利用率（RC -> CXL设备 或 SW -> CXL设备）
            // 利用率 = (实际流量 / 物理带宽) * 100%
            // 实际流量估算：cxl_accesses * 64B cacheline / 1秒，分摊到各设备
            double actualTrafficGbps = (stats.cxl_accesses > 0)
                ? (64.0 * stats.cxl_accesses / 1e9) / cfg.cxl_devices.size()
                : 0.0;
            double linkUtilPct = std::min(100.0, 
                (actualTrafficGbps / cfg.cxl_devices[i].bandwidth_gbps) * 100.0);
            
            // 链路可能是 RC -> CXL 或 SW -> CXL，尝试两者
            if (!cfg.root_complex.id.empty()) {
                topologyEditor_->updateLinkUtilization(
                    QString::fromStdString(cfg.root_complex.id),
                    QString::fromStdString(cfg.cxl_devices[i].id),
                    linkUtilPct);
            }
            // 如果有交换机，也更新 SW -> CXL 链路
            if (!cfg.switches.empty()) {
                topologyEditor_->updateLinkUtilization(
                    QString::fromStdString(cfg.switches[0].id),
                    QString::fromStdString(cfg.cxl_devices[i].id),
                    linkUtilPct);
            }
        }
    }
}

void MainWindow::loadConfig(const QString& filename) {
    cxlsim::ConfigParser parser;
    if (!parser.load_from_file(filename.toStdString())) {
        QMessageBox::critical(this, "\u9519\u8bef", "\u914d\u7f6e\u6587\u4ef6\u52a0\u8f7d\u5931\u8d25\uff0c\u8bf7\u68c0\u67e5\u6587\u4ef6\u683c\u5f0f");
        if (logView_) logView_->append("[ERROR] \u52a0\u8f7d\u5931\u8d25: " + filename);
        return;
    }
    config_ = parser.get_config();
    currentConfigFile_ = filename;
    if (configTree_)    configTree_->setConfig(config_);
    if (topologyEditor_) topologyEditor_->updateTopology(config_);
    updateStatus("\u914d\u7f6e\u5df2\u52a0\u8f7d: " + filename);
    if (logView_) logView_->append("[INFO] \u52a0\u8f7d\u914d\u7f6e: " + filename);
}

void MainWindow::saveConfig(const QString& filename) {
    if (configTree_) config_ = configTree_->getConfig();
    cxlsim::ConfigParser parser;
    parser.get_config() = config_;
    if (!parser.save_to_file(filename.toStdString())) {
        QMessageBox::critical(this, "\u9519\u8bef", "\u914d\u7f6e\u6587\u4ef6\u4fdd\u5b58\u5931\u8d25");
        if (logView_) logView_->append("[ERROR] \u4fdd\u5b58\u5931\u8d25: " + filename);
        return;
    }
    updateStatus("\u914d\u7f6e\u5df2\u4fdd\u5b58: " + filename);
    if (logView_) logView_->append("[INFO] \u4fdd\u5b58\u914d\u7f6e: " + filename);
}

void MainWindow::onExportData() {
    QString filename = QFileDialog::getSaveFileName(
        this, "\u5bfc\u51fa\u5b9e\u9a8c\u6570\u636e", "experiment_data.csv",
        "CSV \u6587\u4ef6 (*.csv);;JSON \u6587\u4ef6 (*.json);;All Files (*)");
    if (filename.isEmpty()) return;

    if (expPanel_) {
        expPanel_->exportResults(filename);
        updateStatus("\u5b9e\u9a8c\u6570\u636e\u5df2\u5bfc\u51fa: " + filename);
        if (logView_) logView_->append("[INFO] \u5b9e\u9a8c\u6570\u636e\u5df2\u5bfc\u51fa: " + filename);
        return;
    }

    if (!analyzer_) {
        QMessageBox::information(this, "\u63d0\u793a", "\u6682\u65e0\u5b9e\u9a8c\u6570\u636e\uff0c\u8bf7\u5148\u8fd0\u884c\u5b9e\u9a8c\u6216\u6a21\u62df");
        return;
    }
    const auto& stats = analyzer_->get_current_stats();
    try {
        if (filename.endsWith(".csv")) {
            std::ofstream file(filename.toStdString());
            file << "\u6307\u6807,\u6570\u503c\n"
                 << "Epoch\u7f16\u53f7," << stats.epoch_number << "\n"
                 << "\u603b\u8bbf\u95ee\u6b21\u6570," << stats.total_accesses << "\n"
                 << "L3\u7f3a\u5931\u6b21\u6570," << stats.l3_misses << "\n"
                 << "CXL\u8bbf\u95ee\u6b21\u6570," << stats.cxl_accesses << "\n"
                 << "\u5e73\u5747\u5ef6\u8fdf(ns)," << stats.avg_latency_ns << "\n"
                 << "\u603b\u6ce8\u5165\u5ef6\u8fdf(ns)," << stats.total_injected_delay_ns << "\n";
        } else {
            QJsonObject json;
            json["epoch_number"]            = static_cast<qint64>(stats.epoch_number);
            json["total_accesses"]          = static_cast<qint64>(stats.total_accesses);
            json["l3_misses"]               = static_cast<qint64>(stats.l3_misses);
            json["cxl_accesses"]            = static_cast<qint64>(stats.cxl_accesses);
            json["avg_latency_ns"]          = stats.avg_latency_ns;
            json["total_injected_delay_ns"] = stats.total_injected_delay_ns;
            QFile f(filename);
            if (f.open(QIODevice::WriteOnly))
                f.write(QJsonDocument(json).toJson());
        }
        updateStatus("\u6570\u636e\u5df2\u5bfc\u51fa: " + filename);
        if (logView_) logView_->append("[INFO] \u5bfc\u51fa\u5b8c\u6210: " + filename);
    } catch (...) {
        QMessageBox::critical(this, "\u9519\u8bef", "\u5bfc\u51fa\u5931\u8d25");
    }
}
