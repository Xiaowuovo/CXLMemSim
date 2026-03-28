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
static QLabel* makeValueLabel(QWidget* parent, const QString& color = "#EDEDED") {
    auto* lbl = new QLabel("0", parent);
    lbl->setStyleSheet(QString(
        "font-size:16px; font-weight:600; color:%1; "
        "background:transparent; border:none; "
        "padding:2px 4px; min-width:80px; font-family: 'JetBrains Mono', 'Inter', monospace;").arg(color));
    lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return lbl;
}

// ── MetricsPanel ─────────────────────────────────────────────────────────────

MetricsPanel::MetricsPanel(QWidget *parent)
    : QWidget(parent)
    , epochNumber_(nullptr)
    , bandwidthDisplay_(nullptr)
    , latencyDisplay_(nullptr)
    , totalAccesses_(nullptr)
    , l3Misses_(nullptr)
    , cxlAccesses_(nullptr)
    , avgLatency_(nullptr)
    , totalDelay_(nullptr)
    , p95Latency_(nullptr)
    , p99Latency_(nullptr)
    , queuingDelay_(nullptr)
    , tieringRatio_(nullptr)
    , linkUtilBar_(nullptr)
    , missRate_(nullptr)
    , latencyChart_(nullptr)
    , bandwidthChart_(nullptr)
    , chartTabs_(nullptr)
    , missRateChart_(nullptr)
{
    setupUI();
}

MetricsPanel::~MetricsPanel() {}

void MetricsPanel::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(16);

    // ── 模式指示器（科研可信度关键）──
    auto* modeFrame = new QFrame(this);
    modeFrame->setStyleSheet(
        "QFrame { background: #1A1A2E; border: 1px solid #4A148C; border-radius: 6px; padding: 8px; }");
    auto* modeLayout = new QHBoxLayout(modeFrame);
    modeLayout->setContentsMargins(8, 6, 8, 6);
    
    auto* modeIcon = new QLabel("⚡", this);
    modeIcon->setStyleSheet("color: #CE93D8; font-size: 16px; font-weight: bold;");
    modeLayout->addWidget(modeIcon);
    
    auto* modeLabel = new QLabel("实时仿真模式 (Live Simulation)", this);
    modeLabel->setStyleSheet("color: #CE93D8; font-size: 11px; font-weight: bold;");
    modeLayout->addWidget(modeLabel, 1);
    
    auto* modeHint = new QLabel("需点击 '▶ 开始模拟' 运行", this);
    modeHint->setStyleSheet("color: #666666; font-size: 9px;");
    modeLayout->addWidget(modeHint);
    
    mainLayout->addWidget(modeFrame);
    
    // ── 科研关键指标：带宽和延迟仪表盘（最突出）──
    auto* dashboardFrame = new QFrame(this);
    dashboardFrame->setStyleSheet("QFrame { background: #0A0A0A; border: 1px solid #222222; border-radius: 6px; padding: 12px; }");
    auto* dashLayout = new QHBoxLayout(dashboardFrame);
    dashLayout->setSpacing(16);
    
    // 带宽仪表
    auto* bwBox = new QVBoxLayout();
    auto* bwLabel = new QLabel("实时带宽 (GB/s)", this);
    bwLabel->setStyleSheet("color: #888888; font-size: 10px; text-align: center;");
    bwLabel->setAlignment(Qt::AlignCenter);
    bwBox->addWidget(bwLabel);
    
    bandwidthDisplay_ = new QLabel("0.0", this);
    bandwidthDisplay_->setStyleSheet("color: #4ADE80; font-size: 28px; font-weight: bold; text-align: center;");
    bandwidthDisplay_->setAlignment(Qt::AlignCenter);
    bwBox->addWidget(bandwidthDisplay_);
    dashLayout->addLayout(bwBox, 1);
    
    // 分隔线
    auto* vline = new QFrame(this);
    vline->setFrameShape(QFrame::VLine);
    vline->setStyleSheet("border: none; background: #333333; max-width: 1px;");
    dashLayout->addWidget(vline);
    
    // 延迟仪表
    auto* latBox = new QVBoxLayout();
    auto* latLabel = new QLabel("平均延迟 (ns)", this);
    latLabel->setStyleSheet("color: #888888; font-size: 10px; text-align: center;");
    latLabel->setAlignment(Qt::AlignCenter);
    latBox->addWidget(latLabel);
    
    latencyDisplay_ = new QLabel("0.0", this);
    latencyDisplay_->setStyleSheet("color: #FBBF24; font-size: 28px; font-weight: bold; text-align: center;");
    latencyDisplay_->setAlignment(Qt::AlignCenter);
    latBox->addWidget(latencyDisplay_);
    dashLayout->addLayout(latBox, 1);
    
    mainLayout->addWidget(dashboardFrame);
    
    // ── Epoch显示（缩小，移至顶部状态栏样式）──
    auto* epochFrame = new QFrame(this);
    epochFrame->setStyleSheet("QFrame { background: transparent; border: none; }");
    auto* epochLayout = new QHBoxLayout(epochFrame);
    epochLayout->setContentsMargins(0, 4, 0, 4);
    
    auto* epochLabel = new QLabel("Epoch:", this);
    epochLabel->setStyleSheet("color: #666666; font-size: 11px;");
    epochLayout->addWidget(epochLabel);
    
    epochNumber_ = new QLCDNumber(this);
    epochNumber_->setDigitCount(6);
    epochNumber_->setSegmentStyle(QLCDNumber::Flat);
    epochNumber_->setMaximumHeight(24);  // 缩小高度
    epochNumber_->setStyleSheet(
        "QLCDNumber { background: #111111; color: #4ADE80; border: 1px solid #222222; border-radius: 3px; }");
    epochLayout->addWidget(epochNumber_);
    epochLayout->addStretch();
    
    mainLayout->addWidget(epochFrame);
    
    mainLayout->addWidget(createAccessGroup());
    mainLayout->addWidget(createLatencyGroup());
    mainLayout->addWidget(createChartGroup());
    mainLayout->addStretch();
}

QGroupBox* MetricsPanel::createEpochGroup() {
    auto* group = new QGroupBox("CURRENT EPOCH", this);
    group->setStyleSheet(
        "QGroupBox { color: #888888; font-size: 11px; font-weight: bold; letter-spacing: 1px; border: none; padding-top: 16px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 0px; padding: 0px; }"
    );
    auto* layout = new QHBoxLayout(group);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(12);

    epochNumber_ = new QLCDNumber(this);
    epochNumber_->setDigitCount(6);
    epochNumber_->setSegmentStyle(QLCDNumber::Flat);
    epochNumber_->display(0);
    epochNumber_->setFixedHeight(44);
    epochNumber_->setStyleSheet(
        "QLCDNumber { background: #0A0A0A; color: #EDEDED; "
        "border: 1px solid #222222; border-radius: 6px; padding: 4px; }"
    );
    layout->addWidget(epochNumber_, 1);

    auto* unitLabel = new QLabel("EPOCH", this);
    unitLabel->setStyleSheet("color: #666666; font-size: 10px; font-weight: bold; letter-spacing: 1px;");
    layout->addWidget(unitLabel, 0, Qt::AlignBottom);
    
    return group;
}

QGroupBox* MetricsPanel::createAccessGroup() {
    auto* group = new QGroupBox("ACCESS STATISTICS", this);
    group->setStyleSheet(
        "QGroupBox { color: #888888; font-size: 11px; font-weight: bold; letter-spacing: 1px; border: none; padding-top: 16px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 0px; padding: 0px; }"
    );
    
    auto* frame = new QFrame(group);
    frame->setStyleSheet("QFrame { background: #0A0A0A; border: 1px solid #222222; border-radius: 6px; }");
    auto* frameLayout = new QVBoxLayout(frame);
    frameLayout->setContentsMargins(12, 12, 12, 12);
    frameLayout->setSpacing(8);

    auto* layout = new QFormLayout();
    layout->setSpacing(8);
    layout->setLabelAlignment(Qt::AlignLeft);
    layout->setFormAlignment(Qt::AlignRight | Qt::AlignTop);

    totalAccesses_ = makeValueLabel(this, "#EDEDED");
    auto* tlbl = new QLabel("Total Accesses", this);
    tlbl->setStyleSheet("color: #888888; font-size: 12px; background: transparent; border: none;");
    layout->addRow(tlbl, totalAccesses_);

    l3Misses_ = makeValueLabel(this, "#F87171"); // Vercel Red
    auto* l3lbl = new QLabel("L3 Misses", this);
    l3lbl->setStyleSheet("color: #888888; font-size: 12px; background: transparent; border: none;");
    layout->addRow(l3lbl, l3Misses_);

    cxlAccesses_ = makeValueLabel(this, "#60A5FA"); // Vercel Blue
    auto* cxllbl = new QLabel("CXL Accesses", this);
    cxllbl->setStyleSheet("color: #888888; font-size: 12px; background: transparent; border: none;");
    layout->addRow(cxllbl, cxlAccesses_);
    
    frameLayout->addLayout(layout);

    // ── CXL科研关键：内存层次化比例 ──
    auto* sep1 = new QFrame(frame);
    sep1->setFrameShape(QFrame::HLine);
    sep1->setStyleSheet("border: none; background: #222222; max-height: 1px;");
    frameLayout->addWidget(sep1);

    auto* tieringLayout = new QHBoxLayout();
    auto* tieringLbl = new QLabel("Local DRAM vs CXL Ratio", this);
    tieringLbl->setStyleSheet("color: #888888; font-size: 11px; background: transparent; border: none;");
    tieringLayout->addWidget(tieringLbl);
    
    tieringRatio_ = new QLabel("0:0", this);
    tieringRatio_->setStyleSheet("color: #10B981; font-size: 12px; font-weight: 600; background: transparent;");
    tieringLayout->addWidget(tieringRatio_, 0, Qt::AlignRight);
    frameLayout->addLayout(tieringLayout);

    // ── 链路利用率进度条 ──
    auto* linkLayout = new QHBoxLayout();
    auto* linkLbl = new QLabel("Link Utilization", this);
    linkLbl->setStyleSheet("color: #888888; font-size: 11px; background: transparent; border: none;");
    linkLayout->addWidget(linkLbl);
    
    linkUtilBar_ = new QProgressBar(this);
    linkUtilBar_->setRange(0, 100);
    linkUtilBar_->setValue(0);
    linkUtilBar_->setTextVisible(false);
    linkUtilBar_->setFixedHeight(6);
    linkUtilBar_->setStyleSheet(
        "QProgressBar { background: #111111; border: none; border-radius: 3px; }"
        "QProgressBar::chunk { background: #10B981; border-radius: 3px; }"
    );
    linkLayout->addWidget(linkUtilBar_, 1, Qt::AlignVCenter);
    frameLayout->addLayout(linkLayout);

    // Separator
    auto* line = new QFrame(frame);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("border: none; background: #222222; max-height: 1px;");
    frameLayout->addWidget(line);

    auto* rateLayout = new QHBoxLayout();
    auto* rateLbl = new QLabel("L3 Miss Rate", this);
    rateLbl->setStyleSheet("color: #888888; font-size: 12px; background: transparent; border: none;");
    rateLayout->addWidget(rateLbl);
    
    missRate_ = new QProgressBar(this);
    missRate_->setRange(0, 100);
    missRate_->setValue(0);
    missRate_->setTextVisible(false);
    missRate_->setFixedHeight(6);
    missRate_->setStyleSheet(
        "QProgressBar { background: #111111; border: none; border-radius: 3px; }"
        "QProgressBar::chunk { background: #F87171; border-radius: 3px; }"
    );
    rateLayout->addWidget(missRate_, 1, Qt::AlignVCenter);
    
    frameLayout->addLayout(rateLayout);

    auto* groupLayout = new QVBoxLayout(group);
    groupLayout->setContentsMargins(0, 8, 0, 0);
    groupLayout->addWidget(frame);

    return group;
}

QGroupBox* MetricsPanel::createLatencyGroup() {
    auto* group = new QGroupBox("LATENCY METRICS", this);
    group->setStyleSheet(
        "QGroupBox { color: #888888; font-size: 11px; font-weight: bold; letter-spacing: 1px; border: none; padding-top: 16px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 0px; padding: 0px; }"
    );
    
    auto* frame = new QFrame(group);
    frame->setStyleSheet("QFrame { background: #0A0A0A; border: 1px solid #222222; border-radius: 6px; }");
    auto* layout = new QFormLayout(frame);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);
    layout->setLabelAlignment(Qt::AlignLeft);

    avgLatency_ = makeValueLabel(this, "#FBBF24");
    auto* albl = new QLabel("Avg CXL Latency", this);
    albl->setStyleSheet("color: #888888; font-size: 12px; background: transparent; border: none;");
    layout->addRow(albl, avgLatency_);

    // ── CXL科研关键指标：尾延迟 ──
    p95Latency_ = makeValueLabel(this, "#F97316"); // Orange
    auto* p95lbl = new QLabel("P95 Latency (抖动)", this);
    p95lbl->setStyleSheet("color: #888888; font-size: 12px; background: transparent; border: none;");
    layout->addRow(p95lbl, p95Latency_);

    p99Latency_ = makeValueLabel(this, "#EF4444"); // Red
    auto* p99lbl = new QLabel("P99 Latency (最坏)", this);
    p99lbl->setStyleSheet("color: #888888; font-size: 12px; background: transparent; border: none;");
    layout->addRow(p99lbl, p99Latency_);

    queuingDelay_ = makeValueLabel(this, "#8B5CF6"); // Purple
    auto* qlbl = new QLabel("Queuing Delay (拥塞)", this);
    qlbl->setStyleSheet("color: #888888; font-size: 12px; background: transparent; border: none;");
    layout->addRow(qlbl, queuingDelay_);

    totalDelay_ = makeValueLabel(this, "#A1A1AA");
    auto* tdlbl = new QLabel("Total Injected Delay", this);
    tdlbl->setStyleSheet("color: #888888; font-size: 12px; background: transparent; border: none;");
    layout->addRow(tdlbl, totalDelay_);

    auto* groupLayout = new QVBoxLayout(group);
    groupLayout->setContentsMargins(0, 8, 0, 0);
    groupLayout->addWidget(frame);

    return group;
}

QGroupBox* MetricsPanel::createChartGroup() {
    auto* group = new QGroupBox("REALTIME TRENDS", this);
    group->setStyleSheet(
        "QGroupBox { color: #888888; font-size: 11px; font-weight: bold; letter-spacing: 1px; border: none; padding-top: 16px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 0px; padding: 0px; }"
    );
    auto* layout = new QVBoxLayout(group);
    layout->setContentsMargins(0, 8, 0, 0);

    chartTabs_ = new QTabWidget(this);
    chartTabs_->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #222222; border-radius: 6px; background: #0A0A0A; top: -1px; }"
        "QTabBar::tab { background: transparent; color: #888888; padding: 6px 12px; border: 1px solid transparent; font-size: 11px; }"
        "QTabBar::tab:selected { color: #EDEDED; border-bottom: 2px solid #EDEDED; }"
    );

    latencyChart_ = new RealTimeChartWidget(this);
    latencyChart_->setTitle("Latency (ns)");
    latencyChart_->setYAxisLabel("ns");
    latencyChart_->setLineColor(QColor(0xFB, 0xBF, 0x24)); // #FBBF24
    chartTabs_->addTab(latencyChart_, "Latency");

    bandwidthChart_ = new RealTimeChartWidget(this);
    bandwidthChart_->setTitle("Bandwidth (GB/s)");
    bandwidthChart_->setYAxisLabel("GB/s");
    bandwidthChart_->setLineColor(QColor(0x4A, 0xDE, 0x80)); // #4ADE80
    chartTabs_->addTab(bandwidthChart_, "Bandwidth");

    missRateChart_ = new RealTimeChartWidget(this);
    missRateChart_->setTitle("L3 Miss Rate (%)");
    missRateChart_->setYAxisLabel("%");
    missRateChart_->setLineColor(QColor(0xF8, 0x71, 0x71)); // #F87171
    chartTabs_->addTab(missRateChart_, "Miss Rate");

    layout->addWidget(chartTabs_);
    return group;
}

void MetricsPanel::updateStats(const cxlsim::EpochStats& stats) {
    epochNumber_->display(static_cast<int>(stats.epoch_number));

    // ── 科研关键：实时带宽和延迟仪表盘 ──
    double bw = 0.0;
    if (stats.total_accesses > 0)
        bw = (stats.total_accesses * 64.0) / 0.01 / 1e9;  // GB/s
    bandwidthDisplay_->setText(QString::number(bw, 'f', 2));
    
    latencyDisplay_->setText(QString::number(stats.avg_latency_ns, 'f', 1));

    totalAccesses_->setText(QString::number(stats.total_accesses));
    l3Misses_->setText(QString::number(stats.l3_misses));
    cxlAccesses_->setText(QString::number(stats.cxl_accesses));

    // ── 内存层次化比例（科研关键）──
    uint64_t localAccesses = stats.local_dram_accesses;
    uint64_t cxlAccesses = stats.cxl_accesses;
    if (localAccesses + cxlAccesses > 0) {
        tieringRatio_->setText(QString("%1:%2")
            .arg(localAccesses).arg(cxlAccesses));
    } else {
        tieringRatio_->setText("0:0");
    }

    // ── 链路利用率 ──
    int linkUtilPct = static_cast<int>(stats.link_utilization_pct);
    linkUtilBar_->setValue(linkUtilPct);
    QString linkColor = linkUtilPct > 80 ? "#EF4444"   // 红色-拥塞
                      : linkUtilPct > 50 ? "#FBBF24"   // 黄色-中等
                      : "#10B981";                     // 绿色-正常
    linkUtilBar_->setStyleSheet(QString(
        "QProgressBar { background: #111111; border: none; border-radius: 3px; }"
        "QProgressBar::chunk { background: %1; border-radius: 3px; }").arg(linkColor));

    int missRatePct = 0;
    if (stats.total_accesses > 0)
        missRatePct = (int)((double)stats.l3_misses * 100.0 / stats.total_accesses);
    missRate_->setValue(missRatePct);

    QString chunkColor;
    if (missRatePct < 20)       chunkColor = "#4ADE80";
    else if (missRatePct < 50)  chunkColor = "#FBBF24";
    else                         chunkColor = "#F87171";
    
    missRate_->setStyleSheet(QString(
        "QProgressBar { background: #111111; border: none; border-radius: 3px; }"
        "QProgressBar::chunk { background: %1; border-radius: 3px; }").arg(chunkColor));

    // ── 延迟指标（含尾延迟）──
    avgLatency_->setText(QString("%1 ns").arg(stats.avg_latency_ns, 0, 'f', 1));
    p95Latency_->setText(QString("%1 ns").arg(stats.p95_latency_ns, 0, 'f', 1));
    p99Latency_->setText(QString("%1 ns").arg(stats.p99_latency_ns, 0, 'f', 1));
    queuingDelay_->setText(QString("%1 ns").arg(stats.queuing_delay_ns, 0, 'f', 1));
    totalDelay_->setText(QString("%1 ms").arg(stats.total_injected_delay_ns / 1e6, 0, 'f', 3));

    latencyChart_->addDataPoint(stats.avg_latency_ns);
    missRateChart_->addDataPoint(missRatePct);
    bandwidthChart_->addDataPoint(bw);  // 复用前面计算的带宽值
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

void MetricsPanel::pinCurrentAsBaseline() {
    // 固定当前所有图表为基准（科研多组对比）
    if (latencyChart_) latencyChart_->pinCurrentAsBaseline("拓扑A");
    if (bandwidthChart_) bandwidthChart_->pinCurrentAsBaseline("拓扑A");
    if (missRateChart_) missRateChart_->pinCurrentAsBaseline("拓扑A");
}

void MetricsPanel::clearBaseline() {
    if (latencyChart_) latencyChart_->clearBaseline();
    if (bandwidthChart_) bandwidthChart_->clearBaseline();
    if (missRateChart_) missRateChart_->clearBaseline();
}
