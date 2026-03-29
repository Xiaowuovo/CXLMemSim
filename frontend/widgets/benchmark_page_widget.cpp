#include "benchmark_page_widget.h"
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>

BenchmarkPageWidget::BenchmarkPageWidget(QWidget *parent)
    : QWidget(parent)
    , benchmarkWidget_(nullptr)
    , comparisonWidget_(nullptr)
{
    setupUI();
}

BenchmarkPageWidget::~BenchmarkPageWidget() {}

void BenchmarkPageWidget::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(16);
    
    // 页面标题
    auto* titleLabel = new QLabel("基准测试与性能对比", this);
    titleLabel->setStyleSheet(
        "color: #4FC3F7; font-size: 18px; font-weight: 700; "
        "padding: 8px 0; border-bottom: 2px solid #1A1A1A;");
    mainLayout->addWidget(titleLabel);
    
    // 描述文本
    auto* descLabel = new QLabel(
        "💡 通过基准测试对比不同负载配置的性能差异，支持多组实验对比分析。", this);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("color: #888888; font-size: 12px; padding: 4px 0 12px 0;");
    mainLayout->addWidget(descLabel);
    
    // 基准测试配置区
    benchmarkWidget_ = new BenchmarkWidget(this);
    connect(benchmarkWidget_, &BenchmarkWidget::baselineFixed, 
            this, [this](const BenchmarkWidget::BenchmarkStats& baseline) {
        // 转换为EpochStats发送信号
        cxlsim::EpochStats stats;
        stats.avg_latency_ns = baseline.avg_latency_ns;
        stats.p95_latency_ns = baseline.p95_latency_ns;
        stats.p99_latency_ns = baseline.p99_latency_ns;
        stats.total_accesses = baseline.total_accesses;
        stats.cxl_accesses = baseline.cxl_accesses;
        stats.local_dram_accesses = baseline.local_accesses;
        emit baselineFixed(stats);
        
        // 更新对比视图 - 将当前数据转换为BenchmarkStats
        if (comparisonWidget_) {
            BenchmarkWidget::BenchmarkStats current;
            current.avg_latency_ns = currentStats_.avg_latency_ns;
            current.p95_latency_ns = currentStats_.p95_latency_ns;
            current.p99_latency_ns = currentStats_.p99_latency_ns;
            current.bandwidth_gbps = (currentStats_.total_accesses * 64.0) / 0.01 / 1e9;
            current.link_utilization_pct = currentStats_.link_utilization_pct;
            current.total_accesses = currentStats_.total_accesses;
            current.cxl_accesses = currentStats_.cxl_accesses;
            current.local_accesses = currentStats_.local_dram_accesses;
            comparisonWidget_->updateComparison(current, baseline);
        }
    });
    connect(benchmarkWidget_, &BenchmarkWidget::baselineCleared,
            this, [this]() {
        emit baselineCleared();
        // 清空对比视图
        if (comparisonWidget_) {
            comparisonWidget_->clear();
        }
    });
    mainLayout->addWidget(benchmarkWidget_);
    
    // 性能对比视图
    comparisonWidget_ = new ComparisonWidget(this);
    comparisonWidget_->setMinimumHeight(320);
    mainLayout->addWidget(comparisonWidget_);
    
    mainLayout->addStretch();
}

void BenchmarkPageWidget::updateCurrentStats(const cxlsim::EpochStats& stats) {
    currentStats_ = stats;
    
    // 如果有基准数据，更新对比视图
    if (comparisonWidget_ && benchmarkWidget_ && benchmarkWidget_->hasBaseline()) {
        // 将当前EpochStats转换为BenchmarkStats
        BenchmarkWidget::BenchmarkStats current;
        current.avg_latency_ns = stats.avg_latency_ns;
        current.p95_latency_ns = stats.p95_latency_ns;
        current.p99_latency_ns = stats.p99_latency_ns;
        current.bandwidth_gbps = (stats.total_accesses * 64.0) / 0.01 / 1e9;
        current.link_utilization_pct = stats.link_utilization_pct;
        current.total_accesses = stats.total_accesses;
        current.cxl_accesses = stats.cxl_accesses;
        current.local_accesses = stats.local_dram_accesses;
        
        auto baseline = benchmarkWidget_->getCurrentBaseline();
        comparisonWidget_->updateComparison(current, baseline);
    }
}

void BenchmarkPageWidget::reset() {
    if (benchmarkWidget_) {
        benchmarkWidget_->clearBaseline();
    }
    if (comparisonWidget_) {
        comparisonWidget_->clear();
    }
    currentStats_ = cxlsim::EpochStats();
}
