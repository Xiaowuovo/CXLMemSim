/**
 * @file metrics_panel.cpp
 * @brief CXL 实时性能指标面板实现
 */

#include "metrics_panel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QFrame>

// ── 辅助函数：创建指标数值标签 ─────────────────────────────────────────────
static QLabel* makeValueLabel(QWidget* parent, const QString& color = "#4FC3F7") {
    auto* lbl = new QLabel("0", parent);
    lbl->setStyleSheet(QString(
        "font-size:15px; font-weight:bold; color:%1; "
        "background:#0D0D1A; border:1px solid #0F3460; "
        "border-radius:4px; padding:3px 8px; min-width:80px;").arg(color));
    lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return lbl;
}

// ── MetricsPanel ─────────────────────────────────────────────────────────────

MetricsPanel::MetricsPanel(QWidget *parent)
    : QWidget(parent)
    , epochNumber_(nullptr)
    , totalAccesses_(nullptr)
    , l3Misses_(nullptr)
    , cxlAccesses_(nullptr)
    , avgLatency_(nullptr)
    , totalDelay_(nullptr)
    , missRate_(nullptr)
    , chartTabs_(nullptr)
    , latencyChart_(nullptr)
    , bandwidthChart_(nullptr)
    , missRateChart_(nullptr)
{
    setupUI();
}

MetricsPanel::~MetricsPanel() {}

void MetricsPanel::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(6);

    mainLayout->addWidget(createEpochGroup());
    mainLayout->addWidget(createAccessGroup());
    mainLayout->addWidget(createLatencyGroup());
    mainLayout->addWidget(createChartGroup());
    mainLayout->addStretch();
}

QGroupBox* MetricsPanel::createEpochGroup() {
    auto* group = new QGroupBox("\u25d4  \u5f53\u524d Epoch", this);
    auto* layout = new QHBoxLayout(group);
    layout->setSpacing(8);

    epochNumber_ = new QLCDNumber(this);
    epochNumber_->setDigitCount(6);
    epochNumber_->setSegmentStyle(QLCDNumber::Flat);
    epochNumber_->display(0);
    epochNumber_->setFixedHeight(50);
    epochNumber_->setStyleSheet(
        "QLCDNumber{background:#0D0D1A;color:#4FC3F7;"
        "border:1px solid #0F3460;border-radius:6px;}");
    layout->addWidget(epochNumber_);

    auto* infoLayout = new QVBoxLayout();
    auto* epochLabel = new QLabel("\u5468\u671f\u7f16\u53f7", this);
    epochLabel->setStyleSheet("color:#9E9E9E; font-size:11px;");
    infoLayout->addWidget(epochLabel);
    auto* unitLabel = new QLabel("epoch", this);
    unitLabel->setStyleSheet("color:#4FC3F7; font-size:13px; font-weight:bold;");
    infoLayout->addWidget(unitLabel);
    layout->addLayout(infoLayout);
    layout->addStretch();
    return group;
}

QGroupBox* MetricsPanel::createAccessGroup() {
    auto* group = new QGroupBox("\u25d4  \u5185\u5b58\u8bbf\u95ee\u7edf\u8ba1", this);
    auto* layout = new QFormLayout(group);
    layout->setSpacing(6);
    layout->setLabelAlignment(Qt::AlignRight);

    totalAccesses_ = makeValueLabel(this, "#E0E0E0");
    auto* tlbl = new QLabel("\u603b\u8bbf\u95ee\u6b21\u6570:", this);
    tlbl->setStyleSheet("color:#B0BEC5;");
    layout->addRow(tlbl, totalAccesses_);

    l3Misses_ = makeValueLabel(this, "#EF5350");
    auto* l3lbl = new QLabel("L3 \u7f3a\u5931\u6b21\u6570:", this);
    l3lbl->setStyleSheet("color:#B0BEC5;");
    layout->addRow(l3lbl, l3Misses_);

    cxlAccesses_ = makeValueLabel(this, "#4FC3F7");
    auto* cxllbl = new QLabel("CXL \u8bbf\u95ee\u6b21\u6570:", this);
    cxllbl->setStyleSheet("color:#B0BEC5;");
    layout->addRow(cxllbl, cxlAccesses_);

    auto* rateLbl = new QLabel("L3 \u7f3a\u5931\u7387:", this);
    rateLbl->setStyleSheet("color:#B0BEC5;");
    missRate_ = new QProgressBar(this);
    missRate_->setRange(0, 100);
    missRate_->setValue(0);
    missRate_->setFormat("%v%");
    missRate_->setFixedHeight(16);
    missRate_->setStyleSheet(
        "QProgressBar{background:#1E1E2E;border:1px solid #0F3460;"
        "border-radius:4px;text-align:center;color:#E0E0E0;font-size:11px;}"
        "QProgressBar::chunk{background:#4FC3F7;border-radius:3px;}");
    layout->addRow(rateLbl, missRate_);

    return group;
}

QGroupBox* MetricsPanel::createLatencyGroup() {
    auto* group = new QGroupBox("\u25d4  \u5ef6\u8fdf\u6307\u6807", this);
    auto* layout = new QFormLayout(group);
    layout->setSpacing(6);
    layout->setLabelAlignment(Qt::AlignRight);

    avgLatency_ = makeValueLabel(this, "#FFB74D");
    auto* albl = new QLabel("\u5e73\u5747 CXL \u5ef6\u8fdf:", this);
    albl->setStyleSheet("color:#B0BEC5;");
    layout->addRow(albl, avgLatency_);

    totalDelay_ = makeValueLabel(this, "#9E9E9E");
    auto* tdlbl = new QLabel("\u603b\u6ce8\u5165\u5ef6\u8fdf:", this);
    tdlbl->setStyleSheet("color:#B0BEC5;");
    layout->addRow(tdlbl, totalDelay_);

    return group;
}

QGroupBox* MetricsPanel::createChartGroup() {
    auto* group = new QGroupBox("\u25d4  \u5b9e\u65f6\u6027\u80fd\u8d8b\u52bf\u56fe", this);
    auto* layout = new QVBoxLayout(group);

    chartTabs_ = new QTabWidget(this);

    latencyChart_ = new RealTimeChartWidget(this);
    latencyChart_->setTitle("\u5e73\u5747\u5ef6\u8fdf (ns)");
    latencyChart_->setYAxisLabel("ns");
    latencyChart_->setLineColor(QColor(0xFF, 0xB7, 0x4D));
    chartTabs_->addTab(latencyChart_, "\u5ef6\u8fdf\u8d8b\u52bf");

    bandwidthChart_ = new RealTimeChartWidget(this);
    bandwidthChart_->setTitle("\u5e26\u5bbd\u4f7f\u7528\u7387 (GB/s)");
    bandwidthChart_->setYAxisLabel("GB/s");
    bandwidthChart_->setLineColor(QColor(0x81, 0xC7, 0x84));
    chartTabs_->addTab(bandwidthChart_, "\u5e26\u5bbd\u8d8b\u52bf");

    missRateChart_ = new RealTimeChartWidget(this);
    missRateChart_->setTitle("L3 \u7f3a\u5931\u7387 (%)");
    missRateChart_->setYAxisLabel("%");
    missRateChart_->setLineColor(QColor(0xEF, 0x53, 0x50));
    chartTabs_->addTab(missRateChart_, "\u7f3a\u5931\u7387\u8d8b\u52bf");

    layout->addWidget(chartTabs_);
    return group;
}

void MetricsPanel::updateStats(const cxlsim::EpochStats& stats) {
    epochNumber_->display(static_cast<int>(stats.epoch_number));

    totalAccesses_->setText(QString::number(stats.total_accesses));
    l3Misses_->setText(QString::number(stats.l3_misses));
    cxlAccesses_->setText(QString::number(stats.cxl_accesses));

    int missRatePct = 0;
    if (stats.total_accesses > 0)
        missRatePct = (int)((double)stats.l3_misses * 100.0 / stats.total_accesses);
    missRate_->setValue(missRatePct);

    QString chunkColor;
    if (missRatePct < 20)       chunkColor = "#81C784";
    else if (missRatePct < 50)  chunkColor = "#FFB74D";
    else                         chunkColor = "#EF5350";
    missRate_->setStyleSheet(QString(
        "QProgressBar{background:#1E1E2E;border:1px solid #0F3460;"
        "border-radius:4px;text-align:center;color:#E0E0E0;font-size:11px;}"
        "QProgressBar::chunk{background:%1;border-radius:3px;}").arg(chunkColor));

    avgLatency_->setText(QString("%1 ns").arg(stats.avg_latency_ns, 0, 'f', 1));
    totalDelay_->setText(QString("%1 ms").arg(stats.total_injected_delay_ns / 1e6, 0, 'f', 3));

    latencyChart_->addDataPoint(stats.avg_latency_ns);
    missRateChart_->addDataPoint(missRatePct);

    double bw = 0.0;
    if (stats.total_accesses > 0)
        bw = (stats.total_accesses * 64.0) / 0.01 / 1e9;
    bandwidthChart_->addDataPoint(bw);
}

void MetricsPanel::reset() {
    epochNumber_->display(0);
    totalAccesses_->setText("0");
    l3Misses_->setText("0");
    cxlAccesses_->setText("0");
    avgLatency_->setText("0.0 ns");
    totalDelay_->setText("0.0 ms");
    missRate_->setValue(0);
    latencyChart_->clear();
    bandwidthChart_->clear();
    missRateChart_->clear();
}
