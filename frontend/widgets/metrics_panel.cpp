/**
 * @file metrics_panel.cpp
 * @brief Metrics panel implementation
 */

#include "metrics_panel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFrame>

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

MetricsPanel::~MetricsPanel() {
}

void MetricsPanel::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    // Add group boxes
    mainLayout->addWidget(createEpochGroup());
    mainLayout->addWidget(createAccessGroup());
    mainLayout->addWidget(createLatencyGroup());
    mainLayout->addWidget(createChartGroup());

    mainLayout->addStretch();
}

QGroupBox* MetricsPanel::createEpochGroup() {
    QGroupBox* group = new QGroupBox("Current Epoch", this);
    QVBoxLayout* layout = new QVBoxLayout(group);

    epochNumber_ = new QLCDNumber(this);
    epochNumber_->setDigitCount(6);
    epochNumber_->setSegmentStyle(QLCDNumber::Flat);
    epochNumber_->display(0);

    layout->addWidget(epochNumber_);

    return group;
}

QGroupBox* MetricsPanel::createAccessGroup() {
    QGroupBox* group = new QGroupBox("Memory Accesses", this);
    QFormLayout* layout = new QFormLayout(group);

    totalAccesses_ = new QLabel("0", this);
    totalAccesses_->setStyleSheet("font-size: 14pt; font-weight: bold;");
    layout->addRow("Total:", totalAccesses_);

    l3Misses_ = new QLabel("0", this);
    l3Misses_->setStyleSheet("font-size: 14pt; color: #ff6b6b;");
    layout->addRow("L3 Misses:", l3Misses_);

    cxlAccesses_ = new QLabel("0", this);
    cxlAccesses_->setStyleSheet("font-size: 14pt; color: #4ecdc4;");
    layout->addRow("CXL Accesses:", cxlAccesses_);

    // Miss rate progress bar
    QLabel* missRateLabel = new QLabel("Miss Rate:", this);
    layout->addRow(missRateLabel);

    missRate_ = new QProgressBar(this);
    missRate_->setRange(0, 100);
    missRate_->setValue(0);
    missRate_->setFormat("%v%");
    layout->addRow(missRate_);

    return group;
}

QGroupBox* MetricsPanel::createLatencyGroup() {
    QGroupBox* group = new QGroupBox("Latency", this);
    QFormLayout* layout = new QFormLayout(group);

    avgLatency_ = new QLabel("0.0 ns", this);
    avgLatency_->setStyleSheet("font-size: 14pt; font-weight: bold;");
    layout->addRow("Average:", avgLatency_);

    totalDelay_ = new QLabel("0.0 ms", this);
    totalDelay_->setStyleSheet("font-size: 12pt; color: #95a5a6;");
    layout->addRow("Total Injected:", totalDelay_);

    return group;
}

QGroupBox* MetricsPanel::createChartGroup() {
    QGroupBox* group = new QGroupBox("Performance Trends", this);
    QVBoxLayout* layout = new QVBoxLayout(group);

    chartTabs_ = new QTabWidget(this);

    // Latency Chart
    latencyChart_ = new RealTimeChartWidget(this);
    latencyChart_->setTitle("Avg Latency (ns)");
    latencyChart_->setYAxisLabel("ns");
    latencyChart_->setLineColor(QColor(52, 152, 219)); // Blue
    chartTabs_->addTab(latencyChart_, "Latency");

    // Bandwidth Chart
    bandwidthChart_ = new RealTimeChartWidget(this);
    bandwidthChart_->setTitle("Bandwidth Usage (GB/s)");
    bandwidthChart_->setYAxisLabel("GB/s");
    bandwidthChart_->setLineColor(QColor(46, 204, 113)); // Green
    chartTabs_->addTab(bandwidthChart_, "Bandwidth");

    // Miss Rate Chart
    missRateChart_ = new RealTimeChartWidget(this);
    missRateChart_->setTitle("L3 Miss Rate (%)");
    missRateChart_->setYAxisLabel("%");
    missRateChart_->setLineColor(QColor(231, 76, 60)); // Red
    chartTabs_->addTab(missRateChart_, "Miss Rate");

    layout->addWidget(chartTabs_);

    return group;
}

void MetricsPanel::updateStats(const cxlsim::EpochStats& stats) {
    // Update epoch number
    epochNumber_->display(static_cast<int>(stats.epoch_number));

    // Update access counts
    totalAccesses_->setText(QString::number(stats.total_accesses));
    l3Misses_->setText(QString::number(stats.l3_misses));
    cxlAccesses_->setText(QString::number(stats.cxl_accesses));

    // Update miss rate
    if (stats.total_accesses > 0) {
        int missRatePercent = (stats.l3_misses * 100) / stats.total_accesses;
        missRate_->setValue(missRatePercent);

        // Color code: green (<20%), yellow (20-50%), red (>50%)
        if (missRatePercent < 20) {
            missRate_->setStyleSheet("QProgressBar::chunk { background-color: #2ecc71; }");
        } else if (missRatePercent < 50) {
            missRate_->setStyleSheet("QProgressBar::chunk { background-color: #f39c12; }");
        } else {
            missRate_->setStyleSheet("QProgressBar::chunk { background-color: #e74c3c; }");
        }
    } else {
        missRate_->setValue(0);
    }

    // Update latency
    avgLatency_->setText(QString("%1 ns").arg(stats.avg_latency_ns, 0, 'f', 2));

    double totalDelayMs = stats.total_injected_delay_ns / 1e6;
    totalDelay_->setText(QString("%1 ms").arg(totalDelayMs, 0, 'f', 3));

    // Update charts
    if (stats.cxl_accesses > 0) {
        latencyChart_->addDataPoint(stats.avg_latency_ns);
    } else {
        latencyChart_->addDataPoint(0.0);
    }

    // Calculate and update miss rate chart
    double missRateVal = 0.0;
    if (stats.total_accesses > 0) {
        missRateVal = (double)stats.l3_misses * 100.0 / stats.total_accesses;
    }
    missRateChart_->addDataPoint(missRateVal);

    // Estimate bandwidth (simplified: 64 bytes per access / epoch duration)
    // Assuming 10ms epoch for now, ideally should get from config or stats
    double bandwidthGbps = 0.0;
    if (stats.total_accesses > 0) {
        // 64 bytes * accesses / 0.01s / 1e9
        bandwidthGbps = (stats.total_accesses * 64.0) / 0.01 / 1e9;
    }
    bandwidthChart_->addDataPoint(bandwidthGbps);
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
