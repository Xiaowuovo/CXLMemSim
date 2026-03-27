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
#include <QPair>
#include <QTimer>
#include "config_parser.h"

class ResultChartWidget;

struct ExperimentResult {
    QString name;
    QString category;
    double avg_latency_ns    = 0.0;
    double bandwidth_gbps    = 0.0;
    double miss_rate_pct     = 0.0;
    double cxl_latency_ns    = 0.0;
    double throughput_ops    = 0.0;  ///< ops/sec for LLM/HPC
    double perf_degradation  = 0.0;  ///< % degradation vs baseline
    uint64_t total_accesses  = 0;
    bool mlp_enabled         = false;
    bool congestion_enabled  = false;
    bool finished            = false;
    // MLC bandwidth-latency curve points
    QVector<QPair<double,double>> bl_curve;  ///< (threads, latency_ns)
};

class ExperimentPanelWidget : public QWidget {
    Q_OBJECT

public:
    explicit ExperimentPanelWidget(QWidget *parent = nullptr);
    ~ExperimentPanelWidget() = default;

    QWidget* getResultWidget() const;
    void exportResults(const QString& filename);

    /**
     * @brief 将自定义拓扑注入下一次实验，覆盖预设参数
     */
    void injectTopology(const cxlsim::CXLSimConfig& config);
    void clearTopologyOverride();

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
        // 扩展字段
        QString workload_type;  ///< "mlc", "llm_prefill", "llm_decode", "hpc_bw", "hpc_lat"
        int     num_threads;    ///< MLC并发线程数
        double  model_size_gb;  ///< LLM模型大小(GB)
        double  kv_cache_gb;    ///< KV Cache大小(GB)
        bool    is_baseline;    ///< 是否为对照基准
    };

    QVector<PresetExperiment> presets_;
    QVector<ExperimentResult> results_;

    // 自定义拓扑注入
    cxlsim::CXLSimConfig topologyOverride_;
    bool hasTopologyOverride_ = false;

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
