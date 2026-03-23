/**
 * @file mainwindow.cpp
 * @brief CXLMemSim 主窗口实现
 */

#include "mainwindow.h"
#include "widgets/topology_editor_widget.h"
#include "widgets/config_tree_widget.h"
#include "widgets/metrics_panel.h"
#include "widgets/experiment_panel_widget.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QToolBar>
#include <QMenuBar>
#include <QStatusBar>
#include <QTimer>
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
    , centralTabs_(nullptr)
    , topologyEditor_(nullptr)
    , configDock_(nullptr)
    , metricsDock_(nullptr)
    , logDock_(nullptr)
    , expDock_(nullptr)
    , configTree_(nullptr)
    , metricsPanel_(nullptr)
    , logView_(nullptr)
    , expPanel_(nullptr)
    , startButton_(nullptr)
    , stopButton_(nullptr)
    , resetButton_(nullptr)
    , statusLabel_(nullptr)
    , updateTimer_(nullptr)
    , simulationRunning_(false)
{
    setupStyle();
    setupUI();
    config_ = cxlsim::ConfigParser::create_default_config();
    if (configTree_) configTree_->setConfig(config_);
    updateStatus("\u5c31\u7eea");
}

MainWindow::~MainWindow() {
    if (updateTimer_) updateTimer_->stop();
    if (simulationRunning_ && analyzer_) analyzer_->stop();
}

void MainWindow::setupStyle() {
    qApp->setStyle("Fusion");
    qApp->setStyleSheet(
        "QMainWindow { background-color: #1A1A2E; }"
        "QMenuBar { background-color: #16213E; color: #E0E0E0; border-bottom: 1px solid #0F3460; font-size: 12px; }"
        "QMenuBar::item:selected { background-color: #0F3460; }"
        "QMenuBar::item:pressed { background-color: #4FC3F7; color: #1A1A2E; }"
        "QMenu { background-color: #16213E; color: #E0E0E0; border: 1px solid #0F3460; }"
        "QMenu::item:selected { background-color: #0F3460; }"
        "QToolBar { background-color: #16213E; border-bottom: 2px solid #0F3460; padding: 4px 8px; spacing: 6px; }"
        "QToolBar::separator { background-color: #0F3460; width: 1px; margin: 4px 6px; }"
        "QDockWidget { color: #E0E0E0; font-weight: bold; }"
        "QDockWidget::title { background-color: #0F3460; padding: 6px 10px; font-size: 12px; font-weight: bold; color: #4FC3F7; }"
        "QDockWidget::close-button, QDockWidget::float-button { background-color: transparent; }"
        "QGroupBox { color: #4FC3F7; border: 1px solid #0F3460; border-radius: 6px; margin-top: 10px; padding-top: 10px; font-weight: bold; font-size: 12px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 6px; }"
        "QPushButton { background-color: #0F3460; color: #E0E0E0; border: 1px solid #4FC3F7; border-radius: 4px; padding: 6px 14px; font-size: 12px; min-width: 70px; }"
        "QPushButton:hover { background-color: #1A4A7A; border-color: #81D4FA; }"
        "QPushButton:pressed { background-color: #4FC3F7; color: #1A1A2E; }"
        "QPushButton:disabled { background-color: #252535; color: #5A5A6E; border-color: #3A3A5C; }"
        "QPushButton#startBtn { background-color: #1B5E20; border-color: #81C784; color: #E8F5E9; }"
        "QPushButton#startBtn:hover { background-color: #2E7D32; }"
        "QPushButton#startBtn:pressed { background-color: #81C784; color: #1B5E20; }"
        "QPushButton#stopBtn { background-color: #B71C1C; border-color: #EF9A9A; color: #FFEBEE; }"
        "QPushButton#stopBtn:hover { background-color: #C62828; }"
        "QPushButton#stopBtn:pressed { background-color: #EF5350; color: #1A1A2E; }"
        "QTreeWidget { background-color: #1E1E2E; color: #E0E0E0; border: 1px solid #0F3460; alternate-background-color: #252535; font-size: 12px; }"
        "QTreeWidget::item:selected { background-color: #0F3460; color: #4FC3F7; }"
        "QTreeWidget::item:hover { background-color: #252545; }"
        "QHeaderView::section { background-color: #0F3460; color: #4FC3F7; padding: 6px; border: none; font-weight: bold; }"
        "QTextEdit { background-color: #0D0D1A; color: #4FC3F7; border: 1px solid #0F3460; font-family: 'Consolas','Monospace'; font-size: 11px; }"
        "QLabel { color: #E0E0E0; font-size: 12px; }"
        "QProgressBar { background-color: #1E1E2E; border: 1px solid #0F3460; border-radius: 4px; text-align: center; color: #E0E0E0; height: 18px; }"
        "QProgressBar::chunk { background-color: #4FC3F7; border-radius: 3px; }"
        "QTabWidget::pane { border: 1px solid #0F3460; background-color: #1E1E2E; }"
        "QTabBar::tab { background-color: #16213E; color: #9E9E9E; padding: 8px 18px; border: 1px solid #0F3460; border-bottom: none; font-size: 12px; }"
        "QTabBar::tab:selected { background-color: #0F3460; color: #4FC3F7; font-weight: bold; }"
        "QTabBar::tab:hover { background-color: #252545; color: #E0E0E0; }"
        "QListWidget { background-color: #1E1E2E; color: #E0E0E0; border: 1px solid #0F3460; alternate-background-color: #252535; font-size: 12px; }"
        "QListWidget::item:selected { background-color: #0F3460; color: #4FC3F7; }"
        "QListWidget::item:hover { background-color: #252545; }"
        "QLCDNumber { background-color: #0D0D1A; color: #4FC3F7; border: 1px solid #0F3460; border-radius: 4px; }"
        "QStatusBar { background-color: #16213E; color: #81D4FA; border-top: 1px solid #0F3460; font-size: 12px; }"
        "QScrollBar:vertical { background-color: #1E1E2E; width: 10px; }"
        "QScrollBar::handle:vertical { background-color: #0F3460; border-radius: 5px; min-height: 20px; }"
        "QScrollBar::handle:vertical:hover { background-color: #4FC3F7; }"
        "QScrollBar:horizontal { background-color: #1E1E2E; height: 10px; }"
        "QScrollBar::handle:horizontal { background-color: #0F3460; border-radius: 5px; }"
        "QSplitter::handle { background-color: #0F3460; }"
        "QComboBox { background-color: #1E1E2E; color: #E0E0E0; border: 1px solid #0F3460; border-radius: 4px; padding: 4px 8px; }"
        "QComboBox::drop-down { border-left: 1px solid #0F3460; }"
        "QSpinBox, QDoubleSpinBox { background-color: #1E1E2E; color: #E0E0E0; border: 1px solid #0F3460; border-radius: 4px; padding: 4px; }"
        "QLineEdit { background-color: #1E1E2E; color: #E0E0E0; border: 1px solid #0F3460; border-radius: 4px; padding: 4px 8px; }"
        "QLineEdit:focus { border-color: #4FC3F7; }"
        "QInputDialog QLabel { color: #E0E0E0; }"
        "QMessageBox { background-color: #1E1E2E; color: #E0E0E0; }"
        "QMessageBox QLabel { color: #E0E0E0; }"
    );
}

void MainWindow::setupUI() {
    setWindowTitle("CXLMemSim - CXL \u5185\u5b58\u6a21\u62df\u7cfb\u7edf");
    resize(1400, 900);
    setMinimumSize(1100, 700);

    // \u987a\u5e8f\u5fc5\u987b\u6b63\u786e\uff1a\u5148\u521b\u5efawidget\uff0c\u518d\u8fde\u63a5\u4fe1\u53f7\u69fd
    setupCentralWidget();
    setupDockWidgets();
    setupMenuBar();
    setupToolBar();
    createConnections();

    statusBar()->showMessage("\u5c31\u7eea - CXLMemSim v1.0");
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
    if (configDock_)  viewMenu->addAction(configDock_->toggleViewAction());
    if (metricsDock_) viewMenu->addAction(metricsDock_->toggleViewAction());
    if (expDock_)     viewMenu->addAction(expDock_->toggleViewAction());
    if (logDock_)     viewMenu->addAction(logDock_->toggleViewAction());

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
}

void MainWindow::setupCentralWidget() {
    centralTabs_ = new QTabWidget(this);
    centralTabs_->setDocumentMode(true);

    topologyEditor_ = new TopologyEditorWidget(centralTabs_);
    centralTabs_->addTab(topologyEditor_, "\u2316  CXL \u62d3\u6251\u56fe");

    setCentralWidget(centralTabs_);
}

void MainWindow::setupDockWidgets() {
    // \u5de6\u4fa7\uff1a\u914d\u7f6e\u6811
    configDock_ = new QDockWidget("\u2630  \u7cfb\u7edf\u914d\u7f6e", this);
    configDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    configDock_->setMinimumWidth(240);
    configTree_ = new ConfigTreeWidget(this);
    configDock_->setWidget(configTree_);
    addDockWidget(Qt::LeftDockWidgetArea, configDock_);

    // \u53f3\u4fa7\uff1a\u6027\u80fd\u6307\u6807
    metricsDock_ = new QDockWidget("\u2630  \u6027\u80fd\u6307\u6807", this);
    metricsDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    metricsDock_->setMinimumWidth(260);
    metricsPanel_ = new MetricsPanel(this);
    metricsDock_->setWidget(metricsPanel_);
    addDockWidget(Qt::RightDockWidgetArea, metricsDock_);

    // \u5e95\u90e8\u5de6\uff1a\u5b9e\u9a8c\u7ba1\u7406\u9762\u677f
    expDock_ = new QDockWidget("\u2630  \u9884\u8bbe\u5b9e\u9a8c\u7ba1\u7406", this);
    expDock_->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::LeftDockWidgetArea);
    expPanel_ = new ExperimentPanelWidget(this);
    expDock_->setWidget(expPanel_);
    addDockWidget(Qt::BottomDockWidgetArea, expDock_);

    // \u5e95\u90e8\u53f3\uff1a\u65e5\u5fd7
    logDock_ = new QDockWidget("\u2630  \u8fd0\u884c\u65e5\u5fd7", this);
    logDock_->setAllowedAreas(Qt::BottomDockWidgetArea);
    logView_ = new QTextEdit(this);
    logView_->setReadOnly(true);
    logView_->setMinimumHeight(100);
    logView_->setMaximumHeight(180);
    logDock_->setWidget(logView_);
    addDockWidget(Qt::BottomDockWidgetArea, logDock_);

    tabifyDockWidget(expDock_, logDock_);
    expDock_->raise();

    resizeDocks({configDock_}, {260}, Qt::Horizontal);
    resizeDocks({metricsDock_}, {280}, Qt::Horizontal);
}

void MainWindow::createConnections() {
    // \u914d\u7f6e\u6811 -> \u62d3\u6251\u56fe
    if (configTree_ && topologyEditor_) {
        connect(configTree_, &ConfigTreeWidget::configChanged,
                topologyEditor_, &TopologyEditorWidget::updateTopology);
    }

    // \u5b9e\u9a8c\u9762\u677f\u65e5\u5fd7\u8f6c\u53d1
    if (expPanel_ && logView_) {
        connect(expPanel_, &ExperimentPanelWidget::logMessage,
                logView_, &QTextEdit::append);
    }

    // \u5b9e\u9a8c\u5b8c\u6210\u65f6\u663e\u793a\u5b9e\u9a8c\u9762\u677f
    if (expPanel_ && expDock_) {
        connect(expPanel_, &ExperimentPanelWidget::resultsReady,
                this, [this]() {
                    expDock_->show();
                    expDock_->raise();
                    updateStatus("\u5b9e\u9a8c\u5b8c\u6210\uff01\u5df2\u751f\u6210\u7ed3\u679c\u56fe\u8868");
                });
    }

    // \u5b9a\u65f6\u66f4\u65b0\u6307\u6807
    updateTimer_ = new QTimer(this);
    connect(updateTimer_, &QTimer::timeout, this, &MainWindow::updateMetrics);
    updateTimer_->start(1000);
}

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
    if (logView_) logView_->append("[INFO] \u6b63\u5728\u521d\u59cb\u5316\u6a21\u62df\u5f15\u64ce...");

    if (!analyzer_) {
        analyzer_ = std::make_unique<cxlsim::TimingAnalyzer>();
        if (!analyzer_->initialize(config_)) {
            QMessageBox::critical(this, "\u9519\u8bef", "\u6a21\u62df\u5f15\u64ce\u521d\u59cb\u5316\u5931\u8d25\uff0c\u8bf7\u68c0\u67e5\u914d\u7f6e\u6587\u4ef6");
            if (logView_) logView_->append("[ERROR] \u5f15\u64ce\u521d\u59cb\u5316\u5931\u8d25");
            analyzer_.reset();
            return;
        }
    }

    simulationRunning_ = true;
    if (startButton_) startButton_->setEnabled(false);
    if (stopButton_)  stopButton_->setEnabled(true);
    if (statusLabel_) {
        statusLabel_->setText("\u8fd0\u884c\u4e2d");
        statusLabel_->setStyleSheet("color:#FF8A65; font-weight:bold; font-size:12px;");
    }
    updateStatus("\u6a21\u62df\u8fd0\u884c\u4e2d...");
    if (logView_) logView_->append("[INFO] CXL \u5185\u5b58\u6a21\u62df\u5df2\u542f\u52a8");
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
    if (expDock_) {
        expDock_->show();
        expDock_->raise();
    }
    updateStatus("\u8bf7\u5728\u300a\u9884\u8bbe\u5b9e\u9a8c\u7ba1\u7406\u300b\u9762\u677f\u9009\u62e9\u5b9e\u9a8c");
    if (logView_) logView_->append("[INFO] \u8bf7\u5728\u5e95\u90e8\u300a\u9884\u8bbe\u5b9e\u9a8c\u7ba1\u7406\u300b\u9762\u677f\u8fd0\u884c\u5b9e\u9a8c");
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
        "<li>11 \u7ec4\u9884\u8bbe\u79d1\u7814\u5b9e\u9a8c\u96c6</li>"
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
