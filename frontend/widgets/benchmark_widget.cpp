#include "benchmark_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFrame>

BenchmarkWidget::BenchmarkWidget(QWidget *parent)
    : QWidget(parent)
    , hasBaseline_(false)
    , isRunning_(false)
{
    setupUI();
}

void BenchmarkWidget::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(16);

    // ══════════════════════════════════════════════════════════
    // 标题
    // ══════════════════════════════════════════════════════════
    auto* titleLabel = new QLabel("基准测试配置", this);
    titleLabel->setStyleSheet(
        "font-size: 16px; font-weight: 700; color: #EDEDED; "
        "padding: 8px 0; border-bottom: 2px solid #333333;");
    mainLayout->addWidget(titleLabel);

    // ══════════════════════════════════════════════════════════
    // 配置区
    // ══════════════════════════════════════════════════════════
    auto* configGroup = new QGroupBox("BENCHMARK SETUP", this);
    configGroup->setStyleSheet(
        "QGroupBox { color: #888888; font-size: 11px; font-weight: bold; "
        "letter-spacing: 1px; border: 1px solid #222222; border-radius: 6px; "
        "padding-top: 16px; background: #0A0A0A; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; }");
    
    auto* configLayout = new QFormLayout(configGroup);
    configLayout->setContentsMargins(16, 20, 16, 16);
    configLayout->setSpacing(12);
    configLayout->setLabelAlignment(Qt::AlignLeft);

    // 负载预设
    auto* workloadLabel = new QLabel("预设负载:", this);
    workloadLabel->setStyleSheet("color: #AAAAAA; font-size: 12px;");
    
    workloadCombo_ = new QComboBox(this);
    workloadCombo_->addItem("随机读写 (Random R/W)", RANDOM_RW);
    workloadCombo_->addItem("流式访问 (STREAM)", STREAM);
    workloadCombo_->addItem("只读模式 (100% Read)", READ_ONLY);
    workloadCombo_->addItem("写密集型 (30% Write)", WRITE_HEAVY);
    workloadCombo_->addItem("Memcached访问", MEMCACHED);
    workloadCombo_->setStyleSheet(
        "QComboBox { background: #0A0A0A; color: #EDEDED; border: 1px solid #333333; "
        "border-radius: 4px; padding: 6px 12px; font-size: 12px; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background: #0A0A0A; color: #EDEDED; "
        "selection-background-color: #1E3A8A; }");
    configLayout->addRow(workloadLabel, workloadCombo_);

    // 架构基准
    auto* archLabel = new QLabel("对照组:", this);
    archLabel->setStyleSheet("color: #AAAAAA; font-size: 12px;");
    
    archCombo_ = new QComboBox(this);
    archCombo_->addItem("纯本地 DRAM (低延迟)", PURE_LOCAL);
    archCombo_->addItem("纯 CXL Memory (高容量)", PURE_CXL);
    archCombo_->addItem("混合配置 (当前)", HYBRID_CURRENT);
    archCombo_->setStyleSheet(
        "QComboBox { background: #0A0A0A; color: #EDEDED; border: 1px solid #333333; "
        "border-radius: 4px; padding: 6px 12px; font-size: 12px; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background: #0A0A0A; color: #EDEDED; "
        "selection-background-color: #1E3A8A; }");
    archCombo_->setCurrentIndex(2); // 默认当前配置
    configLayout->addRow(archLabel, archCombo_);

    mainLayout->addWidget(configGroup);

    // ══════════════════════════════════════════════════════════
    // 操作按钮
    // ══════════════════════════════════════════════════════════
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);

    runBtn_ = new QPushButton("▶ 运行基准测试", this);
    runBtn_->setMinimumHeight(40);
    runBtn_->setCursor(Qt::PointingHandCursor);
    runBtn_->setStyleSheet(
        "QPushButton { background: #0A0A0A; color: #60A5FA; "
        "border: 1px solid #1E3A8A; border-radius: 6px; "
        "padding: 10px 24px; font-weight: 600; font-size: 13px; }"
        "QPushButton:hover { background: #1E3A8A; color: #DBEAFE; border-color: #3B82F6; }"
        "QPushButton:pressed { background: #1D4ED8; }"
        "QPushButton:disabled { background: #0A0A0A; color: #444444; border-color: #222222; }");
    connect(runBtn_, &QPushButton::clicked, this, &BenchmarkWidget::onRunBenchmark);
    btnLayout->addWidget(runBtn_);

    fixBtn_ = new QPushButton("📌 固定为基准", this);
    fixBtn_->setMinimumHeight(40);
    fixBtn_->setCursor(Qt::PointingHandCursor);
    fixBtn_->setEnabled(false);
    fixBtn_->setStyleSheet(
        "QPushButton { background: #0A0A0A; color: #FBBF24; "
        "border: 1px solid #92400E; border-radius: 6px; "
        "padding: 10px 24px; font-weight: 600; font-size: 13px; }"
        "QPushButton:hover { background: #92400E; color: #FEF3C7; border-color: #F59E0B; }"
        "QPushButton:pressed { background: #B45309; }"
        "QPushButton:disabled { background: #0A0A0A; color: #444444; border-color: #222222; }");
    connect(fixBtn_, &QPushButton::clicked, this, &BenchmarkWidget::onFixBaseline);
    btnLayout->addWidget(fixBtn_);

    clearBtn_ = new QPushButton("✕ 清除基准", this);
    clearBtn_->setMinimumHeight(40);
    clearBtn_->setCursor(Qt::PointingHandCursor);
    clearBtn_->setEnabled(false);
    clearBtn_->setStyleSheet(
        "QPushButton { background: #0A0A0A; color: #F87171; "
        "border: 1px solid #7F1D1D; border-radius: 6px; "
        "padding: 10px 24px; font-weight: 600; font-size: 13px; }"
        "QPushButton:hover { background: #7F1D1D; color: #FEE2E2; border-color: #DC2626; }"
        "QPushButton:pressed { background: #991B1B; }"
        "QPushButton:disabled { background: #0A0A0A; color: #444444; border-color: #222222; }");
    connect(clearBtn_, &QPushButton::clicked, this, &BenchmarkWidget::clearBaseline);
    btnLayout->addWidget(clearBtn_);

    mainLayout->addLayout(btnLayout);

    // ══════════════════════════════════════════════════════════
    // 状态显示
    // ══════════════════════════════════════════════════════════
    statusLabel_ = new QLabel("💡 选择负载和对照组，点击"运行基准测试"开始", this);
    statusLabel_->setStyleSheet(
        "color: #888888; font-size: 12px; padding: 8px 12px; "
        "background: #0A0A0A; border: 1px solid #222222; border-radius: 4px;");
    statusLabel_->setWordWrap(true);
    mainLayout->addWidget(statusLabel_);

    // ══════════════════════════════════════════════════════════
    // 结果区
    // ══════════════════════════════════════════════════════════
    resultsGroup_ = new QGroupBox("BASELINE RESULTS", this);
    resultsGroup_->setStyleSheet(
        "QGroupBox { color: #888888; font-size: 11px; font-weight: bold; "
        "letter-spacing: 1px; border: 1px solid #222222; border-radius: 6px; "
        "padding-top: 16px; background: #0A0A0A; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; }");
    resultsGroup_->setVisible(false);

    auto* resultsLayout = new QFormLayout(resultsGroup_);
    resultsLayout->setContentsMargins(16, 20, 16, 16);
    resultsLayout->setSpacing(10);

    auto createResultLabel = [this]() -> QLabel* {
        auto* lbl = new QLabel("--", this);
        lbl->setStyleSheet("color: #EDEDED; font-size: 13px; font-weight: 600; "
                          "font-family: 'Roboto Mono', 'Consolas', monospace;");
        return lbl;
    };

    avgLatencyLabel_ = createResultLabel();
    resultsLayout->addRow(new QLabel("平均延迟:", this), avgLatencyLabel_);

    p95LatencyLabel_ = createResultLabel();
    resultsLayout->addRow(new QLabel("P95 延迟:", this), p95LatencyLabel_);

    p99LatencyLabel_ = createResultLabel();
    resultsLayout->addRow(new QLabel("P99 延迟:", this), p99LatencyLabel_);

    bandwidthLabel_ = createResultLabel();
    resultsLayout->addRow(new QLabel("带宽:", this), bandwidthLabel_);

    mainLayout->addWidget(resultsGroup_);
    mainLayout->addStretch();
}

void BenchmarkWidget::onRunBenchmark() {
    auto workload = static_cast<WorkloadPreset>(workloadCombo_->currentData().toInt());
    auto arch = static_cast<ArchitectureBaseline>(archCombo_->currentData().toInt());
    
    isRunning_ = true;
    runBtn_->setEnabled(false);
    statusLabel_->setText("⏳ 正在运行基准测试...");
    statusLabel_->setStyleSheet(
        "color: #60A5FA; font-size: 12px; padding: 8px 12px; "
        "background: #0A0A0A; border: 1px solid #1E3A8A; border-radius: 4px;");
    
    emit runBenchmarkRequested(workload, arch);
}

void BenchmarkWidget::onBenchmarkCompleted(const cxlsim::EpochStats& stats) {
    currentRunStats_.avg_latency_ns = stats.avg_latency_ns;
    currentRunStats_.p95_latency_ns = stats.p95_latency_ns;
    currentRunStats_.p99_latency_ns = stats.p99_latency_ns;
    currentRunStats_.total_accesses = stats.total_accesses;
    currentRunStats_.cxl_accesses = stats.cxl_accesses;
    currentRunStats_.local_accesses = stats.local_dram_accesses;
    
    // 计算带宽
    double bw = (stats.total_accesses * 64.0) / 0.01 / 1e9;
    currentRunStats_.bandwidth_gbps = bw;
    currentRunStats_.link_utilization_pct = stats.link_utilization_pct;

    isRunning_ = false;
    runBtn_->setEnabled(true);
    fixBtn_->setEnabled(true);
    
    statusLabel_->setText("✓ 基准测试完成！点击"固定为基准"保存结果");
    statusLabel_->setStyleSheet(
        "color: #4ADE80; font-size: 12px; padding: 8px 12px; "
        "background: #0A0A0A; border: 1px solid #166534; border-radius: 4px;");

    // 显示结果
    resultsGroup_->setVisible(true);
    avgLatencyLabel_->setText(QString("%1 ns").arg(currentRunStats_.avg_latency_ns, 0, 'f', 1));
    p95LatencyLabel_->setText(QString("%1 ns").arg(currentRunStats_.p95_latency_ns, 0, 'f', 1));
    p99LatencyLabel_->setText(QString("%1 ns").arg(currentRunStats_.p99_latency_ns, 0, 'f', 1));
    bandwidthLabel_->setText(QString("%1 GB/s").arg(currentRunStats_.bandwidth_gbps, 0, 'f', 2));
}

void BenchmarkWidget::onFixBaseline() {
    baselineStats_ = currentRunStats_;
    hasBaseline_ = true;
    
    fixBtn_->setEnabled(false);
    clearBtn_->setEnabled(true);
    
    statusLabel_->setText("📌 基准已固定！现在可以修改配置并重新运行对比");
    statusLabel_->setStyleSheet(
        "color: #FBBF24; font-size: 12px; padding: 8px 12px; "
        "background: #0A0A0A; border: 1px solid #92400E; border-radius: 4px;");
    
    emit baselineFixed(baselineStats_);
}

void BenchmarkWidget::clearBaseline() {
    baselineStats_ = BenchmarkStats();
    hasBaseline_ = false;
    
    clearBtn_->setEnabled(false);
    resultsGroup_->setVisible(false);
    
    statusLabel_->setText("💡 基准已清除，选择新的配置开始测试");
    statusLabel_->setStyleSheet(
        "color: #888888; font-size: 12px; padding: 8px 12px; "
        "background: #0A0A0A; border: 1px solid #222222; border-radius: 4px;");
    
    emit baselineCleared();
}

QString BenchmarkWidget::getWorkloadDescription(WorkloadPreset preset) const {
    switch (preset) {
        case RANDOM_RW: return "随机读写 (Random R/W)";
        case STREAM: return "流式访问 (STREAM)";
        case READ_ONLY: return "只读模式 (100% Read)";
        case WRITE_HEAVY: return "写密集型 (30% Write)";
        case MEMCACHED: return "Memcached访问";
        default: return "未知";
    }
}

QString BenchmarkWidget::getArchDescription(ArchitectureBaseline arch) const {
    switch (arch) {
        case PURE_LOCAL: return "纯本地 DRAM";
        case PURE_CXL: return "纯 CXL Memory";
        case HYBRID_CURRENT: return "混合配置";
        default: return "未知";
    }
}
