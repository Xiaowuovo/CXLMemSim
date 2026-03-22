/**
 * @file experiment_panel_widget.h
 * @brief 预设实验管理面板 - 支持直接在GUI中运行实验并生成结果图表
 */

#ifndef EXPERIMENT_PANEL_WIDGET_H
#define EXPERIMENT_PANEL_WIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QProgressBar>
#include <QLabel>
#include <QGroupBox>
#include <QSplitter>
#include <QVector>
#include <QTimer>
#include "config_parser.h"

class ResultChartWidget;

struct ExperimentResult {
    QString name;
    QString category;
    double avg_latency_ns   = 0.0;
    double bandwidth_gbps   = 0.0;
    double miss_rate_pct    = 0.0;
    double cxl_latency_ns   = 0.0;
    uint64_t total_accesses = 0;
    bool mlp_enabled        = false;
    bool congestion_enabled = false;
    bool finished           = false;
};

class ExperimentPanelWidget : public QWidget {
    Q_OBJECT

public:
    explicit ExperimentPanelWidget(QWidget *parent = nullptr);
    ~ExperimentPanelWidget() = default;

    QWidget* getResultWidget() const;
    void exportResults(const QString& filename);

signals:
    void logMessage(const QString& message);
    void resultsReady();

public slots:
    void runSelectedExperiment();
    void runAllExperiments();
    void clearResults();

private slots:
    void onExperimentTick();
    void onListSelectionChanged();

private:
    void setupUI();
    void setupPresetExperiments();
    void runExperiment(int index);
    void finishCurrentExperiment();
    void updateResultsChart();
    void appendLog(const QString& msg);

    // 左侧：实验列表
    QListWidget*  experimentList_;
    QLabel*       descLabel_;
    QPushButton*  runSelectedBtn_;
    QPushButton*  runAllBtn_;
    QPushButton*  clearBtn_;
    QProgressBar* progressBar_;
    QLabel*       progressLabel_;

    // 右侧：结果
    ResultChartWidget* resultChart_;
    QTextEdit*         resultText_;

    // 预设实验定义
    struct PresetExperiment {
        QString name;
        QString category;
        QString description;
        double  cxl_latency_ns;
        double  bandwidth_gbps;
        bool    mlp_enabled;
        bool    congestion_enabled;
        int     num_devices;
    };
    QVector<PresetExperiment> presets_;
    QVector<ExperimentResult> results_;

    // 模拟运行状态
    QTimer* simTimer_;
    int     currentExpIdx_;
    int     simStep_;
    int     totalSteps_;
    bool    runAllMode_;
};

// ─── 内置结果图表 ────────────────────────────────────────────────────────────
class ResultChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit ResultChartWidget(QWidget *parent = nullptr);

    void setResults(const QVector<ExperimentResult>& results);
    void setChartType(int type);   // 0=延迟对比, 1=带宽对比, 2=缺失率对比
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void drawBarChart(QPainter& p, const QRect& area);

    QVector<ExperimentResult> results_;
    int chartType_ = 0;
};

#endif // EXPERIMENT_PANEL_WIDGET_H
