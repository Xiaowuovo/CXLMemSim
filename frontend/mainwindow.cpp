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
#include "widgets/export_dialog.h"
#include "tracer/mock_tracer.h"
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
#include <QJsonArray>
#include <QDateTime>
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
    QString modernTheme = R"(
        /* ═══════════════════════════════════════════════════════════════
           CXLMEMSIM - PROFESSIONAL DARK THEME v3.0
           企业级深色主题，极致细节打磨
        ═══════════════════════════════════════════════════════════════ */
        
        * { outline: none; }
        
        /* 主窗口与基础组件 - 微妙渐变背景 */
        QMainWindow { 
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 #000000, stop:0.5 #0A0A0A, stop:1 #000000); 
            color: #E8E8E8; 
            font-family: "Segoe UI", "SF Pro Display", "Microsoft YaHei UI", sans-serif; 
            font-size: 13px; 
        }
        QWidget { background-color: transparent; color: #E8E8E8; }
        
        /* 菜单栏 - 增强层次感 */
        QMenuBar { 
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 #0D0D0D, stop:1 #050505); 
            color: #E8E8E8; 
            border-bottom: 1px solid #1A1A1A; 
            padding: 4px; 
            font-weight: 500;
        }
        QMenuBar::item { 
            background-color: transparent; 
            padding: 7px 16px; 
            border-radius: 6px; 
            margin: 2px 4px;
        }
        QMenuBar::item:selected { 
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 #1A1A1A, stop:1 #0F0F0F); 
        }
        QMenuBar::item:pressed { background-color: #222222; }
        
        QMenu { 
            background-color: #0F0F0F; 
            color: #E8E8E8; 
            border: 1px solid #2A2A2A; 
            border-radius: 10px; 
            padding: 8px; 
        }
        QMenu::item { 
            background-color: transparent; 
            padding: 10px 32px 10px 16px; 
            border-radius: 6px; 
            min-width: 160px; 
            font-size: 13px;
        }
        QMenu::item:selected { 
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 #1E1E1E, stop:1 #141414); 
            color: #FFFFFF; 
        }
        QMenu::separator { 
            height: 1px; 
            background-color: #1A1A1A; 
            margin: 8px 12px; 
        }
        
        /* 工具栏 - 专业级工具条 */
        QToolBar { 
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 #0D0D0D, stop:1 #050505); 
            border: none; 
            border-bottom: 1px solid #1A1A1A; 
            spacing: 6px; 
            padding: 8px 12px; 
        }
        QToolBar::separator { 
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 #1A1A1A, stop:1 #333333); 
            width: 1px; 
            margin: 6px 12px; 
        }
        
        /* 面板停靠区 (干净的标题栏) */
        QDockWidget { color: #888888; font-weight: 500; }
        QDockWidget::title { background-color: #0A0A0A; padding: 8px 12px; font-size: 12px; border-bottom: 1px solid #222222; }
        
        /* 按钮 - 专业级交互反馈 */
        QPushButton { 
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 #1A1A1A, stop:1 #0F0F0F); 
            color: #E8E8E8; 
            border: 1px solid #2A2A2A; 
            border-radius: 7px; 
            padding: 9px 18px; 
            font-weight: 500; 
            font-size: 13px;
            min-height: 26px; 
        }
        QPushButton:hover { 
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 #252525, stop:1 #1A1A1A); 
            border-color: #444444; 
            color: #FFFFFF;
        }
        QPushButton:pressed { 
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 #0F0F0F, stop:1 #1A1A1A); 
            border-color: #555555;
        }
        QPushButton:disabled { 
            background-color: #0A0A0A; 
            color: #444444; 
            border-color: #1A1A1A; 
        }
        
        /* 状态按钮 (Semantic Colors) */
        QPushButton#startBtn { background-color: #000000; border: 1px solid #1A4D2E; color: #4ADE80; }
        QPushButton#startBtn:hover { background-color: #052E16; border-color: #22C55E; }
        QPushButton#stopBtn { background-color: #000000; border: 1px solid #7F1D1D; color: #F87171; }
        QPushButton#stopBtn:hover { background-color: #450A0A; border-color: #EF4444; }
        
        /* 树形列表 - 清晰的信息层级 */
        QTreeWidget, QListWidget { 
            background-color: transparent; 
            color: #E8E8E8; 
            border: none; 
            outline: none; 
            font-size: 13px;
        }
        QTreeWidget::item, QListWidget::item { 
            padding: 8px 12px; 
            border-radius: 6px; 
            margin: 2px 6px; 
            background: transparent; 
            min-height: 24px;
        }
        QTreeWidget::item:selected, QListWidget::item:selected { 
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 #1E1E1E, stop:1 #141414); 
            color: #FFFFFF; 
            border-left: 2px solid #4FC3F7;
        }
        QTreeWidget::item:hover:!selected, QListWidget::item:hover:!selected { 
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 #141414, stop:1 #0A0A0A); 
        }
        QHeaderView::section { 
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 #0D0D0D, stop:1 #050505); 
            color: #888888; 
            padding: 10px 12px; 
            border: none; 
            border-bottom: 1px solid #1A1A1A; 
            font-weight: 600; 
            font-size: 12px;
            text-align: left; 
            letter-spacing: 0.5px;
        }
        
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
        
        /* 文本区域 - 专业编辑器风格 */
        QTextEdit { 
            background-color: #050505; 
            color: #B8B8B8; 
            border: 1px solid #1A1A1A; 
            border-radius: 8px; 
            font-family: "JetBrains Mono", "Fira Code", "Cascadia Code", Consolas, monospace; 
            font-size: 12px; 
            padding: 12px; 
            selection-background-color: #264F78;
            selection-color: #FFFFFF;
        }
        
        /* 输入框 - 增强聚焦状态 */
        QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox { 
            background-color: #0D0D0D; 
            color: #E8E8E8; 
            border: 1px solid #2A2A2A; 
            border-radius: 7px; 
            padding: 8px 12px; 
            font-size: 13px;
            min-height: 20px;
        }
        QLineEdit:focus, QSpinBox:focus, QComboBox:focus { 
            border-color: #4FC3F7; 
            background-color: #0F0F0F; 
            color: #FFFFFF;
        }
        QLineEdit:hover, QSpinBox:hover, QComboBox:hover { 
            border-color: #3A3A3A; 
        }
        
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

// 工具栏已删除，功能迁移到侧边栏和拓扑页面

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
        
        // 工具栏已删除，功能已迁移到拓扑页面和侧边栏
    }
}

void MainWindow::createConnections() {
    // ── 侧边栏页面切换 ──
    if (sidebar_) {
        connect(sidebar_, &SidebarWidget::pageChanged, this, &MainWindow::onPageChanged);
        
        // 侧边栏功能按钮
        connect(sidebar_, &SidebarWidget::pinBaselineRequested, this, [this]() {
            if (metricsPanel_) {
                metricsPanel_->pinCurrentAsBaseline();
                updateStatus("✓ 当前数据已固定为基准，可运行新实验对比");
                if (logView_) logView_->append("[INFO] 📌 基准曲线已固定，可修改拓扑/负载后重新运行对比");
            }
        });
        
        connect(sidebar_, &SidebarWidget::exportDataRequested, this, &MainWindow::onExportData);
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
    
    // ── 拓扑编辑器模拟控制 -> MainWindow ──
    if (topologyEditor_) {
        connect(topologyEditor_, &TopologyEditorWidget::startSimulationRequested,
                this, &MainWindow::onStartSimulation);
        connect(topologyEditor_, &TopologyEditorWidget::stopSimulationRequested,
                this, &MainWindow::onStopSimulation);
        connect(topologyEditor_, &TopologyEditorWidget::resetSimulationRequested,
                this, &MainWindow::onResetSimulation);
    }

    // ── 负载配置应用/取消 ──
    if (workloadWidget_) {
        connect(workloadWidget_, &WorkloadConfigWidget::applyWorkloadRequested, this, [this]() {
            if (!workloadWidget_->isWorkloadValid()) {
                QMessageBox::warning(this, "负载配置无效", 
                    workloadWidget_->getValidationError());
                if (logView_) logView_->append("[WARN] 负载应用失败：配置无效");
                return;
            }
            
            // 同步负载配置到主配置
            config_.workload = workloadWidget_->getWorkloadConfig();
            
            // 更新配置树显示
            if (configTree_) {
                configTree_->setConfig(config_);
            }
            
            updateStatus("✓ 负载配置已应用");
            if (logView_) {
                if (config_.workload.trace_driven) {
                    logView_->append(QString("[INFO] ✓ 负载已应用: Trace-Driven (%1)")
                        .arg(QString::fromStdString(config_.workload.trace_file_path)));
                } else {
                    logView_->append(QString("[INFO] ✓ 负载已应用: Synthetic (%.1f GB/s, %2 模式)")
                        .arg(config_.workload.injection_rate_gbps)
                        .arg(config_.workload.access_pattern == cxlsim::AccessPattern::RANDOM ? "Random" : "Sequential"));
                }
            }
        });
        
        connect(workloadWidget_, &WorkloadConfigWidget::cancelWorkloadRequested, this, [this]() {
            // 清空负载配置
            config_.workload = cxlsim::WorkloadConfig();
            
            // 重置UI
            if (workloadWidget_) {
                workloadWidget_->setWorkloadConfig(config_.workload);
            }
            
            updateStatus("负载配置已清除");
            if (logView_) logView_->append("[INFO] 负载配置已重置");
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
        
        // 创建并配置MockTracer（用于模拟内存访问事件）
        auto tracer = std::make_shared<cxlsim::MockTracer>();
        tracer->set_simulation_params(0.20, 150); // 20% L3 miss rate, 150ns latency
        tracer->set_address_range(0x100000000, 0x200000000); // 4GB-8GB range
        tracer->initialize(0);
        analyzer_->set_tracer(tracer);
        
        // 配置地址映射：将MockTracer生成的地址映射到CXL设备
        // 这是关键：没有地址映射，所有内存访问都不会被识别为CXL访问
        uint64_t base_addr = 0x100000000; // 4GB起始（每次重新初始化）
        for (const auto& device : config_.cxl_devices) {
            cxlsim::AddressMapping mapping;
            mapping.device_id = device.id;
            // 为每个CXL设备分配地址空间
            // 简化：每个设备分配1GB空间
            mapping.start_addr = base_addr;
            mapping.end_addr = base_addr + (1ULL << 30); // +1GB
            base_addr = mapping.end_addr;
            
            analyzer_->add_address_mapping(mapping);
            
            if (logView_) {
                logView_->append(QString("[INFO] 地址映射: %1 -> 0x%2-0x%3")
                    .arg(QString::fromStdString(device.id))
                    .arg(mapping.start_addr, 0, 16)
                    .arg(mapping.end_addr, 0, 16));
            }
        }
        
        if (logView_) logView_->append("[INFO] ✓ MockTracer 已配置 (20% L3 miss, 150ns)");
    }

    // 启动分析器后台线程
    if (!analyzer_->start()) {
        QMessageBox::critical(this, "错误", "无法启动模拟引擎后台线程");
        if (logView_) logView_->append("[ERROR] 引擎启动失败");
        analyzer_.reset();
        return;
    }
    
    simulationRunning_ = true;
    if (statusLabel_) {
        statusLabel_->setText("运行中");
        statusLabel_->setStyleSheet("color:#FF8A65; font-weight:bold; font-size:12px;");
    }
    updateStatus("模拟运行中...");
    if (logView_) logView_->append("[INFO] ✅ CXL 内存模拟已启动");
    if (logView_) logView_->append("[INFO] 📊 Epoch 计数器即将开始跳动...");
}

void MainWindow::onStopSimulation() {
    if (analyzer_) analyzer_->stop();
    simulationRunning_ = false;
    if (statusLabel_) {
        statusLabel_->setText("已停止");
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

// onRunExperiments已删除，实验功能通过侧边栏和实验页面直接访问

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
    
    // 收集历史数据用于导出
    epochHistory_.push_back(stats);
    if (epochHistory_.size() > MAX_HISTORY_SIZE) {
        // 保持最近的MAX_HISTORY_SIZE个数据
        epochHistory_.erase(epochHistory_.begin(), 
                           epochHistory_.begin() + (epochHistory_.size() - MAX_HISTORY_SIZE));
    }

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
    if (epochHistory_.empty()) {
        QMessageBox::information(this, "提示", 
            "暂无历史数据。请先运行模拟并等待数据积累。\n\n"
            "提示：模拟运行时会自动收集性能指标，最多保存10万个Epoch的数据。");
        return;
    }

    // 创建导出对话框
    ExportDialog dialog(this);
    
    // 设置数据
    dialog.setEpochData(epochHistory_);
    
    // 生成配置快照JSON
    QJsonObject configSnapshot;
    configSnapshot["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    configSnapshot["total_epochs"] = static_cast<qint64>(epochHistory_.size());
    
    // 拓扑配置
    if (topologyEditor_) {
        auto cfg = topologyEditor_->getCurrentConfig();
        QJsonObject topo;
        topo["root_complex_id"] = QString::fromStdString(cfg.root_complex.id);
        topo["cxl_device_count"] = static_cast<int>(cfg.cxl_devices.size());
        
        QJsonArray devices;
        for (const auto& dev : cfg.cxl_devices) {
            QJsonObject d;
            d["id"] = QString::fromStdString(dev.id);
            d["capacity_gb"] = static_cast<int>(dev.capacity_gb);
            d["bandwidth_gbps"] = dev.bandwidth_gbps;
            d["base_latency_ns"] = dev.base_latency_ns;
            devices.append(d);
        }
        topo["cxl_devices"] = devices;
        configSnapshot["topology"] = topo;
    }
    
    // 负载配置
    if (workloadWidget_) {
        QJsonObject workload;
        auto wlCfg = workloadWidget_->getWorkloadConfig();
        workload["mode"] = wlCfg.mode == cxlsim::WorkloadConfig::SYNTHETIC ? "synthetic" : "trace";
        workload["read_ratio"] = wlCfg.read_ratio;
        workload["write_ratio"] = wlCfg.write_ratio;
        workload["access_size_bytes"] = static_cast<int>(wlCfg.access_size_bytes);
        configSnapshot["workload"] = workload;
    }
    
    // 统计摘要
    if (!epochHistory_.empty()) {
        const auto& latest = epochHistory_.back();
        QJsonObject summary;
        summary["latest_epoch"] = static_cast<qint64>(latest.epoch_number);
        summary["avg_latency_ns"] = latest.avg_latency_ns;
        summary["p95_latency_ns"] = latest.p95_latency_ns;
        summary["p99_latency_ns"] = latest.p99_latency_ns;
        summary["total_accesses"] = static_cast<qint64>(latest.total_accesses);
        summary["cxl_accesses"] = static_cast<qint64>(latest.cxl_accesses);
        summary["local_dram_accesses"] = static_cast<qint64>(latest.local_dram_accesses);
        configSnapshot["summary"] = summary;
    }
    
    QJsonDocument doc(configSnapshot);
    dialog.setConfigData(doc.toJson(QJsonDocument::Indented));
    
    // 显示对话框
    if (dialog.exec() == QDialog::Accepted) {
        updateStatus("✓ 数据导出完成");
        if (logView_) {
            logView_->append(QString("[INFO] 📦 已导出 %1 个Epoch的性能数据")
                .arg(epochHistory_.size()));
        }
    }
}
