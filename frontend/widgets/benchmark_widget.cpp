#include "benchmark_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFrame>
#include <QHeaderView>
#include <algorithm>
#include <numeric>
#include <cmath>

BenchmarkWidget::BenchmarkWidget(QWidget *parent)
    : QWidget(parent)
    , epochHistory_(nullptr)
    , fixBtn_(nullptr)
    , runBtn_(nullptr)
    , clearBtn_(nullptr)
    , epochCountSpin_(nullptr)
    , statusLabel_(nullptr)
    , baselineEpochLabel_(nullptr)
    , compTable_(nullptr)
    , hasBaseline_(false)
    , hasCurrentRun_(false)
{
    setupUI();
}

void BenchmarkWidget::setEpochHistory(const std::vector<cxlsim::EpochStats>* history) {
    epochHistory_ = history;
}

// ──────────────────────────────────────────────────────────────
//  UI 构建
// ──────────────────────────────────────────────────────────────
void BenchmarkWidget::setupUI() {
    const QString groupStyle =
        "QGroupBox { color: #888888; font-size: 11px; font-weight: bold; "
        "letter-spacing: 1px; border: 1px solid #222222; border-radius: 6px; "
        "padding-top: 20px; background: #0A0A0A; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; }";

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(14);

    // ── 操作区 ──────────────────────────────────────────────
    auto* actionGroup = new QGroupBox("BENCHMARK ACTIONS", this);
    actionGroup->setStyleSheet(groupStyle);
    auto* actionLayout = new QVBoxLayout(actionGroup);
    actionLayout->setContentsMargins(16, 20, 16, 16);
    actionLayout->setSpacing(12);

    // 固定基准说明
    auto* fixDesc = new QLabel(
        "📌 <b>固定基准</b>：将当前实验所有已记录的 epoch 聚合后保存为基准快照", this);
    fixDesc->setWordWrap(true);
    fixDesc->setStyleSheet("color: #AAAAAA; font-size: 12px;");
    actionLayout->addWidget(fixDesc);

    // 固定基准信息行
    auto* baselineInfoRow = new QHBoxLayout();
    baselineEpochLabel_ = new QLabel("尚未固定基准", this);
    baselineEpochLabel_->setStyleSheet(
        "color: #666666; font-size: 11px; font-style: italic;");
    baselineInfoRow->addWidget(baselineEpochLabel_);
    baselineInfoRow->addStretch();
    actionLayout->addLayout(baselineInfoRow);

    // 按钮行 1：固定基准 + 清除基准
    auto* fixRow = new QHBoxLayout();
    fixRow->setSpacing(10);

    fixBtn_ = new QPushButton("  固定基准", this);
    fixBtn_->setMinimumHeight(38);
    fixBtn_->setCursor(Qt::PointingHandCursor);
    fixBtn_->setStyleSheet(
        "QPushButton { background: #0A0A0A; color: #FBBF24; "
        "border: 1px solid #92400E; border-radius: 6px; "
        "padding: 8px 20px; font-weight: 600; font-size: 13px; }"
        "QPushButton:hover { background: #92400E; color: #FEF3C7; border-color: #F59E0B; }"
        "QPushButton:pressed { background: #B45309; }"
        "QPushButton:disabled { background: #0A0A0A; color: #444444; border-color: #222222; }");
    connect(fixBtn_, &QPushButton::clicked, this, &BenchmarkWidget::onFixBaseline);
    fixRow->addWidget(fixBtn_);

    clearBtn_ = new QPushButton("  清除基准", this);
    clearBtn_->setMinimumHeight(38);
    clearBtn_->setEnabled(false);
    clearBtn_->setCursor(Qt::PointingHandCursor);
    clearBtn_->setStyleSheet(
        "QPushButton { background: #0A0A0A; color: #F87171; "
        "border: 1px solid #7F1D1D; border-radius: 6px; "
        "padding: 8px 20px; font-weight: 600; font-size: 13px; }"
        "QPushButton:hover { background: #7F1D1D; color: #FEE2E2; border-color: #DC2626; }"
        "QPushButton:pressed { background: #991B1B; }"
        "QPushButton:disabled { background: #0A0A0A; color: #444444; border-color: #222222; }");
    connect(clearBtn_, &QPushButton::clicked, this, &BenchmarkWidget::onClearBaseline);
    fixRow->addWidget(clearBtn_);
    fixRow->addStretch();
    actionLayout->addLayout(fixRow);

    // 分隔线
    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #222222;");
    actionLayout->addWidget(sep);

    // 运行测试说明 + N epoch 设置
    auto* runDesc = new QLabel(
        "▶ <b>运行测试</b>：聚合最近 N 个 epoch 的统计结果，并与基准对比", this);
    runDesc->setWordWrap(true);
    runDesc->setStyleSheet("color: #AAAAAA; font-size: 12px;");
    actionLayout->addWidget(runDesc);

    auto* epochRow = new QHBoxLayout();
    epochRow->setSpacing(10);
    auto* epochLabel = new QLabel("最近 epoch 数 N：", this);
    epochLabel->setStyleSheet("color: #AAAAAA; font-size: 12px;");
    epochRow->addWidget(epochLabel);

    epochCountSpin_ = new QSpinBox(this);
    epochCountSpin_->setRange(1, 10000);
    epochCountSpin_->setValue(50);
    epochCountSpin_->setSuffix("  个");
    epochCountSpin_->setMinimumWidth(110);
    epochCountSpin_->setStyleSheet(
        "QSpinBox { background: #0A0A0A; color: #EDEDED; border: 1px solid #333333; "
        "border-radius: 4px; padding: 4px 8px; font-size: 12px; }"
        "QSpinBox::up-button, QSpinBox::down-button { border: none; background: #1A1A1A; width: 18px; }"
        "QSpinBox::up-arrow { image: none; } QSpinBox::down-arrow { image: none; }");
    epochRow->addWidget(epochCountSpin_);
    epochRow->addStretch();
    actionLayout->addLayout(epochRow);

    // 运行测试按钮
    runBtn_ = new QPushButton("▶  运行测试", this);
    runBtn_->setMinimumHeight(38);
    runBtn_->setCursor(Qt::PointingHandCursor);
    runBtn_->setStyleSheet(
        "QPushButton { background: #0A0A0A; color: #60A5FA; "
        "border: 1px solid #1E3A8A; border-radius: 6px; "
        "padding: 8px 20px; font-weight: 600; font-size: 13px; }"
        "QPushButton:hover { background: #1E3A8A; color: #DBEAFE; border-color: #3B82F6; }"
        "QPushButton:pressed { background: #1D4ED8; }"
        "QPushButton:disabled { background: #0A0A0A; color: #444444; border-color: #222222; }");
    connect(runBtn_, &QPushButton::clicked, this, &BenchmarkWidget::onRunTest);
    actionLayout->addWidget(runBtn_);

    mainLayout->addWidget(actionGroup);

    // ── 状态栏 ──────────────────────────────────────────────
    statusLabel_ = new QLabel("💡 先点击\"固定基准\"保存当前实验快照，再运行测试进行对比", this);
    statusLabel_->setStyleSheet(
        "color: #888888; font-size: 12px; padding: 8px 12px; "
        "background: #0A0A0A; border: 1px solid #222222; border-radius: 4px;");
    statusLabel_->setWordWrap(true);
    mainLayout->addWidget(statusLabel_);

    // ── 对比表格 ─────────────────────────────────────────────
    auto* tableGroup = new QGroupBox("CURRENT vs BASELINE", this);
    tableGroup->setStyleSheet(groupStyle);
    auto* tableLayout = new QVBoxLayout(tableGroup);
    tableLayout->setContentsMargins(12, 20, 12, 12);

    compTable_ = new QTableWidget(0, 3, this);
    compTable_->setHorizontalHeaderLabels({"指标", "基准", "当前测试"});
    compTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    compTable_->verticalHeader()->setVisible(false);
    compTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    compTable_->setSelectionMode(QAbstractItemView::NoSelection);
    compTable_->setShowGrid(false);
    compTable_->setAlternatingRowColors(true);
    compTable_->setStyleSheet(
        "QTableWidget { background: #0A0A0A; color: #EDEDED; border: none; "
        "font-size: 12px; gridline-color: #1A1A1A; }"
        "QTableWidget::item { padding: 6px 10px; border-bottom: 1px solid #1A1A1A; }"
        "QTableWidget::item:alternate { background: #111111; }"
        "QHeaderView::section { background: #111111; color: #888888; border: none; "
        "padding: 6px 10px; font-size: 11px; font-weight: bold; letter-spacing: 1px; "
        "border-bottom: 1px solid #333333; }");
    compTable_->setMinimumHeight(240);
    tableLayout->addWidget(compTable_);

    mainLayout->addWidget(tableGroup);
    mainLayout->addStretch();
}

// ──────────────────────────────────────────────────────────────
//  聚合辅助函数：对 epochHistory_[fromIdx, toIdx) 做均值聚合
// ──────────────────────────────────────────────────────────────
BenchmarkWidget::BenchmarkStats BenchmarkWidget::aggregateEpochs(int fromIdx, int toIdx) const {
    BenchmarkStats s;
    if (!epochHistory_ || fromIdx >= toIdx) return s;

    int n = toIdx - fromIdx;
    double sumAvgLat = 0, sumP95 = 0, sumP99 = 0, sumBw = 0;
    double sumUtil = 0, sumTiering = 0, sumQueue = 0;
    uint64_t sumTotal = 0, sumCxl = 0, sumLocal = 0;

    for (int i = fromIdx; i < toIdx; ++i) {
        const auto& e = (*epochHistory_)[i];
        sumAvgLat += e.avg_latency_ns;
        sumP95    += e.p95_latency_ns;
        sumP99    += e.p99_latency_ns;
        double bw = (e.total_accesses > 0)
            ? (e.cxl_accesses * 64.0) / (0.01 * 1e9) : 0.0;
        sumBw     += bw;
        sumUtil   += e.link_utilization_pct;
        sumTiering += e.tiering_ratio;
        sumQueue  += e.queuing_delay_ns;
        sumTotal  += e.total_accesses;
        sumCxl    += e.cxl_accesses;
        sumLocal  += e.local_dram_accesses;
    }

    s.avg_latency_ns      = sumAvgLat  / n;
    s.p95_latency_ns      = sumP95     / n;
    s.p99_latency_ns      = sumP99     / n;
    s.bandwidth_gbps      = sumBw      / n;
    s.link_utilization_pct = sumUtil   / n;
    s.tiering_ratio       = sumTiering / n;
    s.queuing_delay_ns    = sumQueue   / n;
    s.total_accesses      = sumTotal   / static_cast<uint64_t>(n);
    s.cxl_accesses        = sumCxl    / static_cast<uint64_t>(n);
    s.local_accesses      = sumLocal  / static_cast<uint64_t>(n);
    s.epoch_count         = n;
    return s;
}

// ──────────────────────────────────────────────────────────────
//  固定基准：聚合当前 epochHistory_ 全量
// ──────────────────────────────────────────────────────────────
void BenchmarkWidget::onFixBaseline() {
    if (!epochHistory_ || epochHistory_->empty()) {
        statusLabel_->setText("⚠  尚无 epoch 数据，请先运行仿真");
        statusLabel_->setStyleSheet(
            "color: #FBBF24; font-size: 12px; padding: 8px 12px; "
            "background: #0A0A0A; border: 1px solid #92400E; border-radius: 4px;");
        return;
    }

    int total = static_cast<int>(epochHistory_->size());
    baselineStats_ = aggregateEpochs(0, total);
    hasBaseline_ = true;

    clearBtn_->setEnabled(true);
    baselineEpochLabel_->setText(
        QString("基准已固定（共 %1 个 epoch，平均延迟 %2 ns）")
            .arg(total)
            .arg(baselineStats_.avg_latency_ns, 0, 'f', 1));
    baselineEpochLabel_->setStyleSheet("color: #FBBF24; font-size: 11px;");

    statusLabel_->setText(
        QString("  基准已固定！来源：全部 %1 个 epoch。现在可点击\"运行测试\"对比最近 N 个 epoch").arg(total));
    statusLabel_->setStyleSheet(
        "color: #FBBF24; font-size: 12px; padding: 8px 12px; "
        "background: #0A0A0A; border: 1px solid #92400E; border-radius: 4px;");

    if (hasCurrentRun_) updateComparisonTable();
    emit baselineFixed(baselineStats_);
}

// ──────────────────────────────────────────────────────────────
//  运行测试：聚合最近 N 个 epoch
// ──────────────────────────────────────────────────────────────
void BenchmarkWidget::onRunTest() {
    if (!epochHistory_ || epochHistory_->empty()) {
        statusLabel_->setText("⚠  尚无 epoch 数据，请先运行仿真");
        statusLabel_->setStyleSheet(
            "color: #FBBF24; font-size: 12px; padding: 8px 12px; "
            "background: #0A0A0A; border: 1px solid #92400E; border-radius: 4px;");
        return;
    }
    if (!hasBaseline_) {
        statusLabel_->setText("⚠  请先点击\"固定基准\"保存基准快照");
        statusLabel_->setStyleSheet(
            "color: #F87171; font-size: 12px; padding: 8px 12px; "
            "background: #0A0A0A; border: 1px solid #7F1D1D; border-radius: 4px;");
        return;
    }

    int total = static_cast<int>(epochHistory_->size());
    int n     = std::min(epochCountSpin_->value(), total);
    int from  = total - n;

    currentRunStats_ = aggregateEpochs(from, total);
    hasCurrentRun_ = true;

    statusLabel_->setText(
        QString("✓ 测试完成！统计最近 %1 个 epoch（#%2 – #%3），平均延迟 %4 ns")
            .arg(n).arg(from).arg(total - 1)
            .arg(currentRunStats_.avg_latency_ns, 0, 'f', 1));
    statusLabel_->setStyleSheet(
        "color: #4ADE80; font-size: 12px; padding: 8px 12px; "
        "background: #0A0A0A; border: 1px solid #166534; border-radius: 4px;");

    updateComparisonTable();
    emit testRan(currentRunStats_, baselineStats_);
}

// ──────────────────────────────────────────────────────────────
//  清除基准
// ──────────────────────────────────────────────────────────────
void BenchmarkWidget::onClearBaseline() {
    clearBaseline();
}

void BenchmarkWidget::clearBaseline() {
    baselineStats_  = BenchmarkStats();
    currentRunStats_ = BenchmarkStats();
    hasBaseline_    = false;
    hasCurrentRun_  = false;

    clearBtn_->setEnabled(false);
    compTable_->setRowCount(0);
    baselineEpochLabel_->setText("尚未固定基准");
    baselineEpochLabel_->setStyleSheet("color: #666666; font-size: 11px; font-style: italic;");

    statusLabel_->setText("💡 先点击\"固定基准\"保存当前实验快照，再运行测试进行对比");
    statusLabel_->setStyleSheet(
        "color: #888888; font-size: 12px; padding: 8px 12px; "
        "background: #0A0A0A; border: 1px solid #222222; border-radius: 4px;");

    emit baselineCleared();
}

// ──────────────────────────────────────────────────────────────
//  更新对比表格
// ──────────────────────────────────────────────────────────────
void BenchmarkWidget::updateComparisonTable() {
    struct Row { QString label; double baseline; double current; QString unit; bool lowerBetter; };
    const Row rows[] = {
        { "平均延迟",   baselineStats_.avg_latency_ns,       currentRunStats_.avg_latency_ns,       "ns",   true  },
        { "P95 延迟",   baselineStats_.p95_latency_ns,       currentRunStats_.p95_latency_ns,       "ns",   true  },
        { "P99 延迟",   baselineStats_.p99_latency_ns,       currentRunStats_.p99_latency_ns,       "ns",   true  },
        { "排队延迟",   baselineStats_.queuing_delay_ns,     currentRunStats_.queuing_delay_ns,     "ns",   true  },
        { "带宽",       baselineStats_.bandwidth_gbps,       currentRunStats_.bandwidth_gbps,       "GB/s", false },
        { "链路利用率", baselineStats_.link_utilization_pct, currentRunStats_.link_utilization_pct, "%",    true  },
        { "Tiering 比", baselineStats_.tiering_ratio,       currentRunStats_.tiering_ratio,       "%",    false },
        { "Epoch 数",   static_cast<double>(baselineStats_.epoch_count), static_cast<double>(currentRunStats_.epoch_count), "",false},
    };
    const int nRows = static_cast<int>(sizeof(rows) / sizeof(rows[0]));

    compTable_->setRowCount(nRows);
    for (int r = 0; r < nRows; ++r) {
        const auto& row = rows[r];

        // 列0: 指标名
        auto* labelItem = new QTableWidgetItem(row.label);
        labelItem->setForeground(QColor(0xAA, 0xAA, 0xAA));
        compTable_->setItem(r, 0, labelItem);

        // 列1: 基准值
        QString bStr = (row.unit == "" || row.label == "Epoch 数")
            ? QString::number(static_cast<int>(row.baseline))
            : QString("%1 %2").arg(row.baseline, 0, 'f', (row.unit == "ns" ? 1 : 2)).arg(row.unit);
        auto* baseItem = new QTableWidgetItem(bStr);
        baseItem->setForeground(QColor(0x88, 0x88, 0x88));
        baseItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        compTable_->setItem(r, 1, baseItem);

        // 列2: 当前值 + 差异百分比着色
        if (!hasCurrentRun_) {
            compTable_->setItem(r, 2, new QTableWidgetItem("—"));
            continue;
        }

        QString delta;
        QColor  color(0xED, 0xED, 0xED);
        if (row.label != "Epoch 数" && row.baseline > 1e-9) {
            double pct = (row.current - row.baseline) / row.baseline * 100.0;
            QString sign = pct > 0 ? "+" : "";
            delta = QString("  (%1%2%)").arg(sign).arg(pct, 0, 'f', 1);
            bool improved = row.lowerBetter ? (row.current < row.baseline)
                                            : (row.current > row.baseline);
            if (std::abs(pct) < 1.0)      color = QColor(0x60, 0xA5, 0xFA);
            else if (improved)             color = QColor(0x4A, 0xDE, 0x80);
            else                           color = QColor(0xF8, 0x71, 0x71);
        }

        QString cStr = (row.unit == "" || row.label == "Epoch 数")
            ? QString::number(static_cast<int>(row.current)) + delta
            : QString("%1 %2").arg(row.current, 0, 'f', (row.unit == "ns" ? 1 : 2)).arg(row.unit) + delta;
        auto* curItem = new QTableWidgetItem(cStr);
        curItem->setForeground(color);
        curItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        compTable_->setItem(r, 2, curItem);
    }
    compTable_->resizeRowsToContents();
}
