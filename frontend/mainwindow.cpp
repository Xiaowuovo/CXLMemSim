/**
 * @file mainwindow.cpp
 * @brief Main window implementation
 */

#include "mainwindow.h"
#include "widgets/topology_editor_widget.h"
#include "widgets/config_tree_widget.h"
#include "widgets/metrics_panel.h"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSettings>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(nullptr)
    , topologyEditor_(nullptr)
    , configDock_(nullptr)
    , metricsDock_(nullptr)
    , logDock_(nullptr)
    , configTree_(nullptr)
    , metricsPanel_(nullptr)
    , logView_(nullptr)
    , startButton_(nullptr)
    , stopButton_(nullptr)
    , statusLabel_(nullptr)
    , simulationRunning_(false)
{
    setupUI();

    // Load default config
    config_ = cxlsim::ConfigParser::create_default_config();

    updateStatus("Ready");
}

MainWindow::~MainWindow() {
    if (simulationRunning_ && analyzer_) {
        analyzer_->stop();
    }
}

void MainWindow::setupUI() {
    setWindowTitle("CXLMemSim - CXL 内存模拟器");
    resize(1280, 800);

    setupMenuBar();
    setupToolBar();
    setupCentralWidget();
    setupDockWidgets();
    createConnections();

    // Status bar
    statusBar()->showMessage("Ready");
}

void MainWindow::setupMenuBar() {
    // File menu
    QMenu* fileMenu = menuBar()->addMenu("文件(&F)");

    QAction* newAction = fileMenu->addAction("新建配置(&N)");
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::onNewConfig);

    QAction* openAction = fileMenu->addAction("打开配置(&O)...");
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenConfig);

    QAction* saveAction = fileMenu->addAction("保存配置(&S)");
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveConfig);

    QAction* saveAsAction = fileMenu->addAction("另存为(&A)...");
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::onSaveConfigAs);

    fileMenu->addSeparator();

    QAction* exportAction = fileMenu->addAction("导出拓扑(&T)...");
    connect(exportAction, &QAction::triggered, this, &MainWindow::onExportTopology);

    QAction* exportDataAction = fileMenu->addAction("导出实验数据(&E)...");
    connect(exportDataAction, &QAction::triggered, this, &MainWindow::onExportData);

    fileMenu->addSeparator();

    QAction* exitAction = fileMenu->addAction("退出(&X)");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &MainWindow::onExit);

    // Simulation menu
    QMenu* simMenu = menuBar()->addMenu("模拟(&S)");

    QAction* startAction = simMenu->addAction("开始模拟(&S)");
    startAction->setShortcut(Qt::Key_F5);
    connect(startAction, &QAction::triggered, this, &MainWindow::onStartSimulation);

    QAction* stopAction = simMenu->addAction("停止模拟(&T)");
    stopAction->setShortcut(Qt::Key_F6);
    connect(stopAction, &QAction::triggered, this, &MainWindow::onStopSimulation);

    QAction* resetAction = simMenu->addAction("重置(&R)");
    connect(resetAction, &QAction::triggered, this, &MainWindow::onResetSimulation);

    simMenu->addSeparator();

    QAction* runExperimentsAction = simMenu->addAction("运行标准实验(&E)...");
    connect(runExperimentsAction, &QAction::triggered, this, &MainWindow::onRunExperiments);

    // View menu
    QMenu* viewMenu = menuBar()->addMenu("视图(&V)");
    // Will add dock widget toggles later

    // Help menu
    QMenu* helpMenu = menuBar()->addMenu("帮助(&H)");

    QAction* aboutAction = helpMenu->addAction("关于(&A)");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);

    QAction* docsAction = helpMenu->addAction("使用文档(&D)");
    connect(docsAction, &QAction::triggered, this, &MainWindow::onShowDocs);
}

void MainWindow::setupToolBar() {
    QToolBar* toolbar = addToolBar("Main Toolbar");
    toolbar->setMovable(false);

    // Start/Stop buttons
    startButton_ = new QPushButton("开始模拟");
    startButton_->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    toolbar->addWidget(startButton_);
    connect(startButton_, &QPushButton::clicked, this, &MainWindow::onStartSimulation);

    stopButton_ = new QPushButton("停止");
    stopButton_->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    stopButton_->setEnabled(false);
    toolbar->addWidget(stopButton_);
    connect(stopButton_, &QPushButton::clicked, this, &MainWindow::onStopSimulation);

    toolbar->addSeparator();

    // Status label
    statusLabel_ = new QLabel("状态: 就绪");
    toolbar->addWidget(statusLabel_);
}

void MainWindow::setupCentralWidget() {
    // Central widget is the topology editor
    topologyEditor_ = new TopologyEditorWidget(this);
    setCentralWidget(topologyEditor_);
}

void MainWindow::setupDockWidgets() {
    // Configuration tree dock (left)
    configDock_ = new QDockWidget("Configuration", this);
    configDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    configTree_ = new ConfigTreeWidget(configDock_);
    configDock_->setWidget(configTree_);
    addDockWidget(Qt::LeftDockWidgetArea, configDock_);

    // Metrics panel dock (right)
    metricsDock_ = new QDockWidget("Performance Metrics", this);
    metricsDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    metricsPanel_ = new MetricsPanel(metricsDock_);
    metricsDock_->setWidget(metricsPanel_);
    addDockWidget(Qt::RightDockWidgetArea, metricsDock_);

    // Log view dock (bottom)
    logDock_ = new QDockWidget("Log", this);
    logDock_->setAllowedAreas(Qt::BottomDockWidgetArea);

    logView_ = new QTextEdit(logDock_);
    logView_->setReadOnly(true);
    logView_->setMaximumHeight(150);
    logDock_->setWidget(logView_);
    addDockWidget(Qt::BottomDockWidgetArea, logDock_);
}

void MainWindow::createConnections() {
    // Connect config tree to topology editor
    connect(configTree_, &ConfigTreeWidget::configChanged,
            topologyEditor_, &TopologyEditorWidget::updateTopology);

    // Connect topology editor context menu actions to config tree slots
    connect(topologyEditor_, &TopologyEditorWidget::addDeviceRequested,
            configTree_, &ConfigTreeWidget::onAddDevice);
    connect(topologyEditor_, &TopologyEditorWidget::addSwitchRequested,
            configTree_, &ConfigTreeWidget::onAddSwitch);
    connect(topologyEditor_, &TopologyEditorWidget::removeSelectedRequested,
            configTree_, &ConfigTreeWidget::onRemoveSelected);

    // Auto-update metrics when simulation is running
    QTimer* updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::updateMetrics);
    updateTimer->start(1000);  // Update every second
}

// File menu slots
void MainWindow::onNewConfig() {
    config_ = cxlsim::ConfigParser::create_default_config();
    currentConfigFile_.clear();

    configTree_->setConfig(config_);
    topologyEditor_->updateTopology(config_);

    updateStatus("New configuration created");
    logView_->append("[INFO] Created new default configuration");
}

void MainWindow::onOpenConfig() {
    QString filename = QFileDialog::getOpenFileName(
        this,
        "Open Configuration",
        "",
        "JSON Files (*.json);;All Files (*)"
    );

    if (!filename.isEmpty()) {
        loadConfig(filename);
    }
}

void MainWindow::onSaveConfig() {
    if (currentConfigFile_.isEmpty()) {
        onSaveConfigAs();
    } else {
        saveConfig(currentConfigFile_);
    }
}

void MainWindow::onSaveConfigAs() {
    QString filename = QFileDialog::getSaveFileName(
        this,
        "Save Configuration",
        "",
        "JSON Files (*.json);;All Files (*)"
    );

    if (!filename.isEmpty()) {
        saveConfig(filename);
        currentConfigFile_ = filename;
    }
}

void MainWindow::onExportTopology() {
    QString filename = QFileDialog::getSaveFileName(
        this,
        "Export Topology Image",
        "topology.png",
        "Images (*.png *.jpg)"
    );

    if (!filename.isEmpty()) {
        topologyEditor_->exportToImage(filename);
        updateStatus("Topology exported to " + filename);
        logView_->append("[INFO] Exported topology image: " + filename);
    }
}

void MainWindow::onExit() {
    close();
}

// Simulation menu slots
void MainWindow::onStartSimulation() {
    logView_->append("[INFO] Starting simulation...");

    // Create analyzer if needed
    if (!analyzer_) {
        analyzer_ = std::make_unique<cxlsim::TimingAnalyzer>();
        if (!analyzer_->initialize(config_)) {
            QMessageBox::critical(this, "Error", "Failed to initialize analyzer");
            logView_->append("[ERROR] Failed to initialize analyzer");
            return;
        }
    }

    // TODO: Set up tracer and start simulation

    simulationRunning_ = true;
    startButton_->setEnabled(false);
    stopButton_->setEnabled(true);
    statusLabel_->setText("Status: Running");

    updateStatus("Simulation started");
    logView_->append("[INFO] Simulation started");
}

void MainWindow::onStopSimulation() {
    if (analyzer_) {
        analyzer_->stop();
    }

    simulationRunning_ = false;
    startButton_->setEnabled(true);
    stopButton_->setEnabled(false);
    statusLabel_->setText("Status: Stopped");

    updateStatus("Simulation stopped");
    logView_->append("[INFO] Simulation stopped");
}

void MainWindow::onResetSimulation() {
    onStopSimulation();
    analyzer_.reset();

    metricsPanel_->reset();

    updateStatus("Simulation reset");
    logView_->append("[INFO] Simulation reset");
}

// Help menu slots
void MainWindow::onAbout() {
    QMessageBox::about(this, "About CXLMemSim",
        "<h2>CXLMemSim 1.0.0</h2>"
        "<p>CXL Memory Simulator</p>"
        "<p>A high-fidelity execution-driven simulator for CXL memory systems.</p>"
        "<p><b>Features:</b></p>"
        "<ul>"
        "<li>Precise latency modeling</li>"
        "<li>Topology visualization</li>"
        "<li>Performance analysis</li>"
        "<li>Congestion simulation</li>"
        "</ul>"
        "<p>Developed with Qt6 and C++17</p>"
    );
}

// Helper methods
void MainWindow::updateStatus(const QString& message) {
    statusBar()->showMessage(message, 3000);
}

void MainWindow::updateMetrics() {
    if (!simulationRunning_ || !analyzer_) {
        return;
    }

    // Get current stats from analyzer
    const auto& stats = analyzer_->get_current_stats();
    metricsPanel_->updateStats(stats);
}

void MainWindow::loadConfig(const QString& filename) {
    cxlsim::ConfigParser parser;
    if (!parser.load_from_file(filename.toStdString())) {
        QMessageBox::critical(this, "Error", "Failed to load configuration file");
        logView_->append("[ERROR] Failed to load: " + filename);
        return;
    }

    config_ = parser.get_config();
    currentConfigFile_ = filename;

    configTree_->setConfig(config_);
    topologyEditor_->updateTopology(config_);

    updateStatus("Configuration loaded: " + filename);
    logView_->append("[INFO] Loaded configuration: " + filename);
}

void MainWindow::saveConfig(const QString& filename) {
    // Get current config from UI
    config_ = configTree_->getConfig();

    cxlsim::ConfigParser parser;
    parser.get_config() = config_;

    if (!parser.save_to_file(filename.toStdString())) {
        QMessageBox::critical(this, "Error", "Failed to save configuration file");
        logView_->append("[ERROR] Failed to save: " + filename);
        return;
    }

    updateStatus("Configuration saved: " + filename);
    logView_->append("[INFO] Saved configuration: " + filename);
}
