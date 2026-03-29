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
            this, [this](const cxlsim::EpochStats& baseline) {
        emit baselineFixed(baseline);
        // 更新对比视图
        if (comparisonWidget_) {
            comparisonWidget_->updateComparison(currentStats_, baseline);
        }
    });
    connect(benchmarkWidget_, &BenchmarkWidget::baselineCleared,
            this, [this]() {
        emit baselineCleared();
        // 清空对比视图
        if (comparisonWidget_) {
            comparisonWidget_->clearComparison();
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
    
    // 更新基准测试widget
    if (benchmarkWidget_) {
        benchmarkWidget_->updateCurrentStats(stats);
    }
    
    // 如果有基准数据，更新对比视图
    if (comparisonWidget_ && benchmarkWidget_->hasBaseline()) {
        auto baseline = benchmarkWidget_->getBaselineStats();
        comparisonWidget_->updateComparison(stats, baseline);
    }
}

void BenchmarkPageWidget::reset() {
    if (benchmarkWidget_) {
        benchmarkWidget_->clearBaseline();
    }
    if (comparisonWidget_) {
        comparisonWidget_->clearComparison();
    }
    currentStats_ = cxlsim::EpochStats();
}
