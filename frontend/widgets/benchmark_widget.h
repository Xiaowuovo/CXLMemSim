#ifndef BENCHMARK_WIDGET_H
#define BENCHMARK_WIDGET_H

#include <QWidget>
#include <QGroupBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <vector>
#include "analyzer/timing_analyzer.h"

/**
 * @brief 基准测试配置和管理Widget
 *
 * 简化功能：
 * 1. 固定基准：将当前实验的全量历史统计聚合后记录为基准
 * 2. 运行测试：取最近 N 个 epoch 聚合后与基准对比
 */
class BenchmarkWidget : public QWidget {
    Q_OBJECT

public:
    struct BenchmarkStats {
        double avg_latency_ns;
        double p95_latency_ns;
        double p99_latency_ns;
        double bandwidth_gbps;
        double link_utilization_pct;
        double tiering_ratio;
        double queuing_delay_ns;
        uint64_t total_accesses;
        uint64_t cxl_accesses;
        uint64_t local_accesses;
        int      epoch_count;

        BenchmarkStats() : avg_latency_ns(0), p95_latency_ns(0), p99_latency_ns(0),
                           bandwidth_gbps(0), link_utilization_pct(0),
                           tiering_ratio(0), queuing_delay_ns(0),
                           total_accesses(0), cxl_accesses(0), local_accesses(0),
                           epoch_count(0) {}
    };

    explicit BenchmarkWidget(QWidget *parent = nullptr);
    ~BenchmarkWidget() = default;

    void setEpochHistory(const std::vector<cxlsim::EpochStats>* history);

    BenchmarkStats getCurrentBaseline() const { return baselineStats_; }
    bool hasBaseline() const { return hasBaseline_; }

    void clearBaseline();

signals:
    void baselineFixed(const BenchmarkStats& stats);
    void testRan(const BenchmarkStats& current, const BenchmarkStats& baseline);
    void baselineCleared();

private slots:
    void onFixBaseline();
    void onRunTest();
    void onClearBaseline();

private:
    void setupUI();
    void updateComparisonTable();
    BenchmarkStats aggregateEpochs(int fromIdx, int toIdx) const;

    const std::vector<cxlsim::EpochStats>* epochHistory_;

    // UI
    QPushButton* fixBtn_;
    QPushButton* runBtn_;
    QPushButton* clearBtn_;
    QSpinBox*    epochCountSpin_;
    QLabel*      statusLabel_;
    QLabel*      baselineEpochLabel_;
    QTableWidget* compTable_;

    // 数据
    BenchmarkStats baselineStats_;
    BenchmarkStats currentRunStats_;
    bool hasBaseline_;
    bool hasCurrentRun_;
};

#endif // BENCHMARK_WIDGET_H
