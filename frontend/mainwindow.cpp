/**
 * @file mainwindow.cpp
 * @brief Main window implementation
 */

#include "mainwindow.h"
#include "widgets/topology_editor_widget.h"
#include "widgets/config_tree_widget.h"
#include "widgets/metrics_panel.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QToolBar>
#include <QMenuBar>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <fstream>
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

void MainWindow::onExportData() {
    if (!analyzer_ || !simulationRunning_) {
        QMessageBox::information(this, "提示", 
            "请先运行模拟，然后再导出实验数据。");
        return;
    }
    
    QString filename = QFileDialog::getSaveFileName(
        this,
        "导出实验数据",
        "experiment_data.csv",
        "CSV Files (*.csv);;JSON Files (*.json);;All Files (*)"
    );
    
    if (filename.isEmpty()) {
        return;
    }
    
    // 获取当前统计数据
    const auto& stats = analyzer_->get_current_stats();
    
    try {
        if (filename.endsWith(".csv")) {
            // 导出为CSV格式
            std::ofstream file(filename.toStdString());
            file << "指标,数值\n";
            file << "Epoch编号," << stats.epoch_number << "\n";
            file << "总访问次数," << stats.total_accesses << "\n";
            file << "CXL访问次数," << stats.cxl_accesses << "\n";
            file << "平均延迟(ns)," << stats.avg_latency_ns << "\n";
            file << "最大延迟(ns)," << stats.max_latency_ns << "\n";
            file << "最小延迟(ns)," << stats.min_latency_ns << "\n";
            file << "总注入延迟(ns)," << stats.total_injected_delay_ns << "\n";
            file.close();
            
            updateStatus("数据已导出到: " + filename);
            logView_->append("[INFO] 实验数据已导出: " + filename);
        } else {
            // 导出为JSON格式
            QJsonObject json;
            json["epoch_number"] = static_cast<qint64>(stats.epoch_number);
            json["total_accesses"] = static_cast<qint64>(stats.total_accesses);
            json["cxl_accesses"] = static_cast<qint64>(stats.cxl_accesses);
            json["avg_latency_ns"] = stats.avg_latency_ns;
            json["max_latency_ns"] = stats.max_latency_ns;
            json["min_latency_ns"] = stats.min_latency_ns;
            json["total_injected_delay_ns"] = stats.total_injected_delay_ns;
            
            QJsonDocument doc(json);
            QFile file(filename);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(doc.toJson());
                file.close();
                updateStatus("数据已导出到: " + filename);
                logView_->append("[INFO] 实验数据已导出: " + filename);
            }
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "错误", 
            QString("导出失败: %1").arg(e.what()));
        logView_->append("[ERROR] 导出数据失败");
    }
}

void MainWindow::onRunExperiments() {
    QMessageBox::information(this, "运行标准实验",
        "<h3>标准实验集</h3>"
        "<p>系统提供了11组预设的标准实验配置：</p>"
        "<ul>"
        "<li><b>延迟敏感性分析</b>（4组）：评估不同CXL延迟的影响</li>"
        "<li><b>带宽瓶颈分析</b>（3组）：评估带宽对性能的影响</li>"
        "<li><b>拥塞模型对比</b>（2组）：验证拥塞模型的效果</li>"
        "<li><b>MLP优化效果</b>（2组）：评估内存级并行优化</li>"
        "</ul>"
        "<p><b>运行方式：</b></p>"
        "<p>请在终端中执行以下命令：</p>"
        "<pre>./scripts/run_experiments.sh</pre>"
        "<p>实验结果将保存在 <code>results/</code> 目录</p>"
        "<p>然后使用Python脚本生成图表：</p>"
        "<pre>python3 scripts/plot_results.py results/experiment_results.csv</pre>"
    );
    
    logView_->append("[INFO] 请参考标准实验配置: configs/standard_experiments.json");
}

void MainWindow::onShowDocs() {
    QMessageBox::information(this, "使用文档",
        "<h3>CXLMemSim 文档索引</h3>"
        "<p><b>核心文档：</b></p>"
        "<ul>"
        "<li><b>README.md</b> - 项目概览和快速开始</li>"
        "<li><b>QUICKSTART.md</b> - 详细的快速开始指南</li>"
        "<li><b>DOCS_INDEX.md</b> - 完整文档索引</li>"
        "<li><b>毕设撰写指导说明书.md</b> - 论文撰写指南（2万字）</li>"
        "<li><b>项目优化总结.md</b> - 项目优化内容总结</li>"
        "</ul>"
        "<p><b>配置文件：</b></p>"
        "<ul>"
        "<li><b>configs/examples/simple_cxl.json</b> - 简单示例配置</li>"
        "<li><b>configs/standard_experiments.json</b> - 标准实验配置（11组）</li>"
        "</ul>"
        "<p><b>在线帮助：</b></p>"
        "<p>项目目录中的所有文档均可在文本编辑器中打开查看。</p>"
    );
    
    logView_->append("[INFO] 请参考项目根目录下的文档文件");
}
