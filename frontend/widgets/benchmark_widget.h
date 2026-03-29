#ifndef BENCHMARK_WIDGET_H
#define BENCHMARK_WIDGET_H

#include <QWidget>
#include <QGroupBox>
#include <QComboBox>
#include <QRadioButton>
#include <QPushButton>
#include <QLabel>
#include <vector>
#include "analyzer/timing_analyzer.h"

/**
 * @brief 基准测试配置和管理Widget
 * 
 * 核心功能：
 * 1. 预设负载选择（随机读写、流式访问、只读等）
 * 2. 对照组设定（纯本地DRAM、纯CXL、混合）
 * 3. 运行基准测试并固定结果
 */
class BenchmarkWidget : public QWidget {
    Q_OBJECT

public:
    enum WorkloadPreset {
        RANDOM_RW,      // 随机读写
        STREAM,         // 流式访问 (STREAM benchmark)
        READ_ONLY,      // 100% 读
        WRITE_HEAVY,    // 写密集 (30% 写)
        MEMCACHED       // Memcached访问模式
    };

    enum ArchitectureBaseline {
        PURE_LOCAL,     // 100% Local DRAM（低延迟基准）
        PURE_CXL,       // 100% CXL Memory（高容量基准）
        HYBRID_CURRENT  // 当前混合配置
    };

    struct BenchmarkStats {
        double avg_latency_ns;
        double p95_latency_ns;
        double p99_latency_ns;
        double bandwidth_gbps;
        double link_utilization_pct;
        uint64_t total_accesses;
        uint64_t cxl_accesses;
        uint64_t local_accesses;
        
        BenchmarkStats() : avg_latency_ns(0), p95_latency_ns(0), p99_latency_ns(0),
                          bandwidth_gbps(0), link_utilization_pct(0),
                          total_accesses(0), cxl_accesses(0), local_accesses(0) {}
    };

    explicit BenchmarkWidget(QWidget *parent = nullptr);
    ~BenchmarkWidget() = default;

    // 获取当前基准统计
    BenchmarkStats getCurrentBaseline() const { return baselineStats_; }
    bool hasBaseline() const { return hasBaseline_; }
    
    // 清除基准
    void clearBaseline();

signals:
    void runBenchmarkRequested(WorkloadPreset workload, ArchitectureBaseline arch);
    void baselineFixed(const BenchmarkStats& stats);
    void baselineCleared();

public slots:
    void onBenchmarkCompleted(const cxlsim::EpochStats& stats);

private slots:
    void onRunBenchmark();
    void onFixBaseline();

private:
    void setupUI();
    QString getWorkloadDescription(WorkloadPreset preset) const;
    QString getArchDescription(ArchitectureBaseline arch) const;

    // UI组件
    QComboBox* workloadCombo_;
    QComboBox* archCombo_;
    QPushButton* runBtn_;
    QPushButton* fixBtn_;
    QPushButton* clearBtn_;
    
    // 结果显示
    QLabel* statusLabel_;
    QGroupBox* resultsGroup_;
    QLabel* avgLatencyLabel_;
    QLabel* p95LatencyLabel_;
    QLabel* p99LatencyLabel_;
    QLabel* bandwidthLabel_;
    
    // 基准数据
    BenchmarkStats baselineStats_;
    BenchmarkStats currentRunStats_;
    bool hasBaseline_;
    bool isRunning_;
};

#endif // BENCHMARK_WIDGET_H
