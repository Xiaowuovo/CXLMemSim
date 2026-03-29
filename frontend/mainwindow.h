/**
 * @file mainwindow.h
 * @brief CXLMemSim 主窗口
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDockWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QTimer>
#include <memory>
#include "config_parser.h"
#include "analyzer/timing_analyzer.h"

class TopologyEditorWidget;
class ConfigTreeWidget;
class MetricsPanel;
class ExperimentPanelWidget;
class WorkloadConfigWidget;
class SidebarWidget;
class QStackedWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 文件菜单
    void onNewConfig();
    void onOpenConfig();
    void onSaveConfig();
    void onSaveConfigAs();
    void onExportTopology();
    void onExportData();
    void onExit();

    // 模拟菜单
    void onStartSimulation();
    void onStopSimulation();
    void onResetSimulation();

    // 实验菜单
    void onShowDocs();
    void onAbout();

    // 更新处理
    void updateStatus(const QString& message);
    void updateMetrics();

private:
    void setupUI();
    void setupStyle();
    void setupMenuBar();
    void setupSidebar();
    void setupPages();
    void createConnections();
    void loadConfig(const QString& filename);
    void saveConfig(const QString& filename);
    void onPageChanged(int pageIndex);

    // 侧边栏和页面系统（VSCode风格）
    SidebarWidget* sidebar_;
    QStackedWidget* pageStack_;
    
    // 页面组件（顺序必须与构造函数初始化列表一致）
    TopologyEditorWidget* topologyEditor_;
    ConfigTreeWidget* configTree_;
    WorkloadConfigWidget* workloadWidget_;
    ExperimentPanelWidget* expPanel_;
    MetricsPanel* metricsPanel_;
    QTextEdit* logView_;

    // 状态标签（移到状态栏）
    QLabel* statusLabel_;

    // 后端组件
    std::unique_ptr<cxlsim::TimingAnalyzer> analyzer_;
    QTimer* updateTimer_;
    
    // 配置和状态
    cxlsim::CXLSimConfig config_;
    QString currentConfigFile_;
    bool simulationRunning_;
};

#endif // MAINWINDOW_H
