/**
 * @file mainwindow.h
 * @brief Main window for CXLMemSim GUI
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDockWidget>
#include <QTextEdit>
#include <QTreeView>
#include <QPushButton>
#include <QLabel>
#include <memory>
#include "config_parser.h"
#include "analyzer/timing_analyzer.h"
#include "analyzer/experiment_manager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class TopologyEditorWidget;
class ConfigTreeWidget;
class MetricsPanel;

/**
 * @brief Main application window
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // File menu actions
    void onNewConfig();
    void onOpenConfig();
    void onSaveConfig();
    void onSaveConfigAs();
    void onExportTopology();
    void onExportData();        // 导出实验数据
    void onExit();

    // Simulation menu actions
    void onStartSimulation();
    void onStopSimulation();
    void onResetSimulation();
    void onRunExperiments();    // 运行标准实验

    // Help menu actions
    void onAbout();
    void onShowDocs();          // 显示使用文档

    // Update handlers
    void updateStatus(const QString& message);
    void updateMetrics();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupDockWidgets();
    void setupCentralWidget();
    void createActions();
    void createConnections();

    void loadConfig(const QString& filename);
    void saveConfig(const QString& filename);

    // UI Components
    Ui::MainWindow *ui;

    // Central widget - Topology Editor
    TopologyEditorWidget* topologyEditor_;

    // Dock widgets
    QDockWidget* configDock_;
    QDockWidget* metricsDock_;
    QDockWidget* logDock_;

    // Child widgets
    ConfigTreeWidget* configTree_;
    MetricsPanel* metricsPanel_;
    QTextEdit* logView_;

    // Toolbar widgets
    QPushButton* startButton_;
    QPushButton* stopButton_;
    QLabel* statusLabel_;

    // Backend components
    cxlsim::CXLSimConfig config_;
    std::unique_ptr<cxlsim::TimingAnalyzer> analyzer_;

    // State
    QString currentConfigFile_;
    bool simulationRunning_;
};

#endif // MAINWINDOW_H
