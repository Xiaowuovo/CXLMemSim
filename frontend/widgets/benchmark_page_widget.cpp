#include "benchmark_page_widget.h"
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QFrame>

BenchmarkPageWidget::BenchmarkPageWidget(QWidget *parent)
    : QWidget(parent)
    , benchmarkWidget_(nullptr)
{
    setupUI();
}

BenchmarkPageWidget::~BenchmarkPageWidget() {}

void BenchmarkPageWidget::setEpochHistory(const std::vector<cxlsim::EpochStats>* history) {
    if (benchmarkWidget_) {
        benchmarkWidget_->setEpochHistory(history);
    }
}

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
        "💡 固定当前实验快照为基准，再对比最近 N 个 epoch 的统计结果。", this);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("color: #888888; font-size: 12px; padding: 4px 0 12px 0;");
    mainLayout->addWidget(descLabel);

    // 滚动区包裹 BenchmarkWidget（内容可能较长）
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; }");

    benchmarkWidget_ = new BenchmarkWidget(this);
    scrollArea->setWidget(benchmarkWidget_);
    mainLayout->addWidget(scrollArea);

    // 信号透传
    connect(benchmarkWidget_, &BenchmarkWidget::baselineFixed,
            this, &BenchmarkPageWidget::baselineFixed);
    connect(benchmarkWidget_, &BenchmarkWidget::baselineCleared,
            this, &BenchmarkPageWidget::baselineCleared);
}

void BenchmarkPageWidget::reset() {
    if (benchmarkWidget_) {
        benchmarkWidget_->clearBaseline();
    }
}
