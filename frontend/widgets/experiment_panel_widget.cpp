/**
 * @file experiment_panel_widget.cpp
 * @brief 预设实验管理面板实现
 */

#include "experiment_panel_widget.h"

#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QListWidgetItem>
#include <QTabWidget>
#include <QScrollArea>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFontMetrics>
#include <QPaintEvent>
#include <cmath>
#include <algorithm>

// ═══════════════════════════════════════════════════════════════════════════════
// ResultChartWidget
// ═══════════════════════════════════════════════════════════════════════════════

ResultChartWidget::ResultChartWidget(QWidget *parent)
    : QWidget(parent), chartType_(0)
{
    setMinimumHeight(200);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(false);
}

void ResultChartWidget::setResults(const QVector<ExperimentResult>& results) {
    results_ = results;
    update();
}

void ResultChartWidget::setChartType(int type) {
    chartType_ = type;
    update();
}

void ResultChartWidget::clear() {
    results_.clear();
    update();
}

void ResultChartWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 背景 - 极黑
    p.fillRect(rect(), QColor(0x05, 0x05, 0x05));

    QRect area = rect().adjusted(58, 16, -16, -40);

    if (results_.isEmpty()) {
        p.setPen(QColor(0x66, 0x66, 0x66));
        p.setFont(QFont("Inter, -apple-system", 11));
        p.drawText(rect(), Qt::AlignCenter, "No experiment results yet.\nPlease run an experiment first.");
        return;
    }

    drawBarChart(p, area);
}

void ResultChartWidget::drawBarChart(QPainter& p, const QRect& area) {
    // 收集仅已完成的结果
    QVector<const ExperimentResult*> done;
    for (const auto& r : results_)
        if (r.finished) done.append(&r);
    if (done.isEmpty()) {
        p.setPen(QColor(0x66, 0x66, 0x66));
        p.drawText(area, Qt::AlignCenter, "Running experiments...");
        return;
    }

    // 选择数据
    QVector<double> vals;
    QString unit, title;
    
    // Vercel 风格色板 (高明度/低饱和)
    static const QColor BAR_COLORS[] = {
        {0x3B,0x82,0xF6}, // Blue
        {0x10,0xB9,0x81}, // Emerald
        {0xF5,0x9E,0x0B}, // Amber
        {0xEF,0x44,0x44}, // Red
        {0x8B,0x5C,0xF6}, // Purple
        {0x06,0xB6,0xD4}, // Cyan
        {0x84,0xCC,0x16}, // Green
        {0xF9,0x73,0x16}, // Orange
        {0xF4,0x3F,0x5E}  // Rose
    };

    for (const auto* r : done) {
        switch (chartType_) {
            case 0: vals.append(r->avg_latency_ns);   break;
            case 1: vals.append(r->bandwidth_gbps);   break;
            case 2: vals.append(r->miss_rate_pct);    break;
        }
    }
    switch (chartType_) {
        case 0: unit = "ns";    title = "Average Latency (ns)"; break;
        case 1: unit = "GB/s";  title = "Estimated Bandwidth (GB/s)"; break;
        case 2: unit = "%";     title = "L3 Miss Rate (%)"; break;
    }

    double maxVal = *std::max_element(vals.begin(), vals.end());
    if (maxVal < 1e-9) maxVal = 1.0;

    // 绘制标题
    p.setPen(QColor(0xED, 0xED, 0xED));
    p.setFont(QFont("Inter, -apple-system", 10, QFont::Bold));
    p.drawText(QRect(area.left(), 2, area.width(), 14), Qt::AlignCenter, title);

    // Y轴刻度
    int nTick = 5;
    p.setFont(QFont("JetBrains Mono, Consolas", 8));
    p.setPen(QColor(0x66, 0x66, 0x66));
    for (int i = 0; i <= nTick; ++i) {
        int y = area.bottom() - (int)(area.height() * i / (double)nTick);
        double val = maxVal * i / nTick;
        p.drawLine(area.left() - 4, y, area.right(), y); // 横向参考线 (极细)
        QString label = val < 10 ? QString::number(val, 'f', 1)
                                 : QString::number((int)val);
        p.drawText(QRect(0, y - 8, area.left() - 8, 16),
                   Qt::AlignRight | Qt::AlignVCenter, label);
    }

    // 柱形
    int n = done.size();
    int barW = std::max(8, (area.width() - (n + 1) * 6) / n);
    barW = std::min(barW, 40); // 柱子稍微细一点，留白更多

    p.setFont(QFont("Inter, -apple-system", 8));
    for (int i = 0; i < n; ++i) {
        double v = vals[i];
        int h  = (int)(area.height() * v / maxVal);
        int x  = area.left() + 6 + i * (barW + 6);
        int y  = area.bottom() - h;

        // Vercel风格柱状图 (纯色+顶部微光)
        QColor col = BAR_COLORS[i % 9];
        
        // 柱体底色
        p.fillRect(QRect(x, y, barW, h), col.darker(150));
        
        // 柱体渐变
        QLinearGradient grad(x, y, x, y + h);
        grad.setColorAt(0.0, col);
        grad.setColorAt(1.0, col.darker(200));
        p.fillRect(QRect(x, y, barW, h), grad);
        
        // 边框
        p.setPen(QPen(col.lighter(120), 1));
        p.drawRect(QRect(x, y, barW, h));

        // 数值标签
        p.setPen(QColor(0xED, 0xED, 0xED));
        QString valStr = v < 10 ? QString::number(v, 'f', 1)
                                : QString::number((int)v);
        p.drawText(QRect(x - 10, y - 18, barW + 20, 14), Qt::AlignCenter, valStr);

        // 实验名缩写
        QString shortName = done[i]->name.left(6);
        p.setPen(QColor(0x88, 0x88, 0x88));
        p.drawText(QRect(x - 10, area.bottom() + 6, barW + 20, 30),
                   Qt::AlignHCenter | Qt::AlignTop, shortName);
    }

    // 坐标轴轴线
    p.setPen(QPen(QColor(0x33, 0x33, 0x33), 2));
    p.drawLine(area.left(), area.top(), area.left(), area.bottom());
    p.drawLine(area.left(), area.bottom(), area.right(), area.bottom());
}

// ═══════════════════════════════════════════════════════════════════════════════
// ExperimentPanelWidget
// ═══════════════════════════════════════════════════════════════════════════════

ExperimentPanelWidget::ExperimentPanelWidget(QWidget *parent)
    : QWidget(parent)
    , experimentList_(nullptr)
    , descLabel_(nullptr)
    , runSelectedBtn_(nullptr)
    , runAllBtn_(nullptr)
    , clearBtn_(nullptr)
    , progressBar_(nullptr)
    , progressLabel_(nullptr)
    , resultChart_(nullptr)
    , resultText_(nullptr)
    , simTimer_(nullptr)
    , currentExpIdx_(-1)
    , simStep_(0)
    , totalSteps_(0)
    , runAllMode_(false)
{
    setupPresetExperiments();
    setupUI();

    simTimer_ = new QTimer(this);
    simTimer_->setInterval(120);
    connect(simTimer_, &QTimer::timeout, this, &ExperimentPanelWidget::onExperimentTick);
}

void ExperimentPanelWidget::setupPresetExperiments() {
    presets_ = {
        // 延迟敏感性分析
        {"基准延迟 (100ns)",    "延迟敏感性", "CXL基础延迟100ns，单设备参考基准",          100,  64, false, false, 1},
        {"低延迟配置 (170ns)",  "延迟敏感性", "典型CXL Type3设备延迟170ns",               170,  64, false, false, 1},
        {"中等延迟 (250ns)",    "延迟敏感性", "延迟增加对访问性能的影响评估",              250,  64, false, false, 1},
        {"高延迟场景 (350ns)",  "延迟敏感性", "远端NUMA节点等高延迟场景分析",             350,  64, false, false, 1},
        // 带宽瓶颈分析
        {"高带宽 (128GB/s)",   "带宽瓶颈",  "PCIe Gen5 x16 满速带宽配置",              170, 128, false, true,  1},
        {"中等带宽 (64GB/s)",  "带宽瓶颈",  "PCIe Gen5 x8 配置，分析带宽限制",          170,  64, false, true,  1},
        {"低带宽 (32GB/s)",    "带宽瓶颈",  "PCIe Gen4 配置，带宽严重受限场景",          170,  32, false, true,  1},
        // 拥塞模型对比
        {"无拥塞模型",         "拥塞对比",  "禁用拥塞模拟，理想条件下的基准性能",         170,  64, false, false, 2},
        {"拥塞模型启用",       "拥塞对比",  "启用拥塞模型，真实多设备竞争场景",           170,  64, false, true,  2},
        // MLP优化效果
        {"MLP优化禁用",        "MLP优化",   "禁用内存级并行优化，串行访问模式",            170,  64, false, true,  1},
        {"MLP优化启用",        "MLP优化",   "启用MLP优化，并行内存访问性能对比",           170,  64, true,  true,  1},
    };
}

void ExperimentPanelWidget::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(16);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(4);
    splitter->setStyleSheet("QSplitter::handle { background: #222222; }");

    // ── 左侧面板 ────────────────────────────────────────────────────────────
    auto* leftWidget = new QWidget(splitter);
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(12);

    auto* listGroup = new QGroupBox("PRESET EXPERIMENTS (11 Groups)", leftWidget);
    listGroup->setStyleSheet(
        "QGroupBox { color: #888888; font-size: 11px; font-weight: bold; letter-spacing: 1px; border: none; padding-top: 16px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 0px; padding: 0px; }"
    );
    auto* listLayout = new QVBoxLayout(listGroup);
    listLayout->setContentsMargins(0, 8, 0, 0);
    listLayout->setSpacing(8);

    experimentList_ = new QListWidget(listGroup);
    experimentList_->setStyleSheet(
        "QListWidget { background: #0A0A0A; color: #EDEDED; border: 1px solid #222222; border-radius: 6px; outline: none; padding: 4px; }"
        "QListWidget::item { padding: 8px 4px; border-radius: 4px; margin: 1px; }"
        "QListWidget::item:selected { background-color: #222222; color: #FFFFFF; }"
        "QListWidget::item:hover:!selected { background-color: #111111; }"
    );
    experimentList_->setAlternatingRowColors(false);
    experimentList_->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // 按类别添加实验条目
    QString lastCat;
    for (int i = 0; i < presets_.size(); ++i) {
        const auto& exp = presets_[i];
        if (exp.category != lastCat) {
            auto* sep = new QListWidgetItem(exp.category.toUpper(), experimentList_);
            sep->setFlags(Qt::ItemIsEnabled);
            sep->setForeground(QColor(0x88, 0x88, 0x88));
            sep->setBackground(Qt::NoBrush);
            QFont f("Inter, -apple-system", 10, QFont::Bold);
            sep->setFont(f);
            sep->setData(Qt::UserRole, -1);
            lastCat = exp.category;
        }
        auto* item = new QListWidgetItem(
            QString("    %1. %2").arg(i + 1).arg(exp.name), experimentList_);
        item->setData(Qt::UserRole, i);
        item->setToolTip(exp.description);
    }
    listLayout->addWidget(experimentList_);

    descLabel_ = new QLabel("Select an experiment to view details", listGroup);
    descLabel_->setWordWrap(true);
    descLabel_->setStyleSheet(
        "color: #A1A1AA; font-size: 12px; padding: 12px;"
        "border: 1px solid #222222; border-radius: 6px;"
        "background: #050505;"
    );
    descLabel_->setMinimumHeight(70);
    listLayout->addWidget(descLabel_);

    leftLayout->addWidget(listGroup);

    // 控制按钮
    auto* btnGroup = new QGroupBox("CONTROLS", leftWidget);
    btnGroup->setStyleSheet(
        "QGroupBox { color: #888888; font-size: 11px; font-weight: bold; letter-spacing: 1px; border: none; padding-top: 16px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 0px; padding: 0px; }"
    );
    auto* btnLayout = new QVBoxLayout(btnGroup);
    btnLayout->setContentsMargins(0, 8, 0, 0);
    btnLayout->setSpacing(8);

    runSelectedBtn_ = new QPushButton("Run Selected", this);
    runSelectedBtn_->setStyleSheet(
        "QPushButton { background: #000000; color: #4ADE80; border: 1px solid #1A4D2E; border-radius: 6px; padding: 10px; font-weight: 500; }"
        "QPushButton:hover { background: #052E16; border-color: #22C55E; }"
        "QPushButton:pressed { background: #1A4D2E; }"
        "QPushButton:disabled { background: #0A0A0A; color: #444444; border-color: #222222; }"
    );
    btnLayout->addWidget(runSelectedBtn_);

    runAllBtn_ = new QPushButton("Run All Experiments", this);
    runAllBtn_->setStyleSheet(
        "QPushButton { background: #000000; color: #60A5FA; border: 1px solid #1E3A8A; border-radius: 6px; padding: 10px; font-weight: 500; }"
        "QPushButton:hover { background: #172554; border-color: #3B82F6; }"
        "QPushButton:pressed { background: #1E3A8A; }"
        "QPushButton:disabled { background: #0A0A0A; color: #444444; border-color: #222222; }"
    );
    btnLayout->addWidget(runAllBtn_);

    clearBtn_ = new QPushButton("Clear Results", this);
    clearBtn_->setStyleSheet(
        "QPushButton { background: transparent; color: #888888; border: 1px dashed #333333; border-radius: 6px; padding: 10px; font-weight: 500; }"
        "QPushButton:hover { background: #2A0808; color: #F87171; border-color: #7F1D1D; }"
        "QPushButton:pressed { background: #450A0A; }"
    );
    btnLayout->addWidget(clearBtn_);

    // 进度显示
    progressBar_ = new QProgressBar(btnGroup);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    progressBar_->setTextVisible(false);
    progressBar_->setFixedHeight(6);
    progressBar_->setStyleSheet(
        "QProgressBar { background: #111111; border: none; border-radius: 3px; }"
        "QProgressBar::chunk { background: #EDEDED; border-radius: 3px; }"
    );
    btnLayout->addWidget(progressBar_);

    progressLabel_ = new QLabel("Ready", btnGroup);
    progressLabel_->setStyleSheet("color: #666666; font-size: 11px; font-weight: 500;");
    progressLabel_->setAlignment(Qt::AlignCenter);
    btnLayout->addWidget(progressLabel_);

    leftLayout->addWidget(btnGroup);
    splitter->addWidget(leftWidget);

    // ── 右侧面板 ────────────────────────────────────────────────────────────
    auto* rightWidget = new QWidget(splitter);
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(16);

    // 图表组
    auto* chartGroup = new QGroupBox("VISUALIZATION", rightWidget);
    chartGroup->setStyleSheet(
        "QGroupBox { color: #888888; font-size: 11px; font-weight: bold; letter-spacing: 1px; border: none; padding-top: 16px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 0px; padding: 0px; }"
    );
    auto* chartLayout = new QVBoxLayout(chartGroup);
    chartLayout->setContentsMargins(0, 8, 0, 0);
    chartLayout->setSpacing(8);

    // 图表类型切换
    auto* chartBtnRow = new QHBoxLayout();
    chartBtnRow->setSpacing(8);
    const QStringList chartNames = {"Latency", "Bandwidth", "Miss Rate"};
    for (int i = 0; i < 3; ++i) {
        auto* btn = new QPushButton(chartNames[i]);
        btn->setCheckable(true);
        btn->setChecked(i == 0);
        btn->setStyleSheet(
            "QPushButton { background: transparent; color: #888888; border: 1px solid #333333; border-radius: 6px; padding: 6px 12px; font-size: 12px; }"
            "QPushButton:hover { background: #111111; border-color: #555555; }"
            "QPushButton:checked { background: #EDEDED; color: #000000; border: none; font-weight: 500; }"
        );
        chartBtnRow->addWidget(btn);
        int idx = i;
        connect(btn, &QPushButton::clicked, this, [this, idx, chartNames, btn]() {
            resultChart_->setChartType(idx);
            // 取消其他按钮选中
            QWidget* p = btn->parentWidget();
            if (!p) return;
            for (auto* child : p->findChildren<QPushButton*>())
                if (child != btn) child->setChecked(false);
            btn->setChecked(true);
        });
    }
    chartBtnRow->addStretch();
    chartLayout->addLayout(chartBtnRow);

    resultChart_ = new ResultChartWidget(chartGroup);
    resultChart_->setMinimumHeight(240);
    
    // Add border to chart wrapper
    auto* chartWrapper = new QFrame(chartGroup);
    chartWrapper->setStyleSheet("QFrame { background: #0A0A0A; border: 1px solid #222222; border-radius: 6px; }");
    auto* wrapperLayout = new QVBoxLayout(chartWrapper);
    wrapperLayout->setContentsMargins(0, 0, 0, 0);
    wrapperLayout->addWidget(resultChart_);
    
    chartLayout->addWidget(chartWrapper);

    rightLayout->addWidget(chartGroup, 3);

    // 数据表格组
    auto* tableGroup = new QGroupBox("DATA SUMMARY", rightWidget);
    tableGroup->setStyleSheet(
        "QGroupBox { color: #888888; font-size: 11px; font-weight: bold; letter-spacing: 1px; border: none; padding-top: 16px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 0px; padding: 0px; }"
    );
    auto* tableLayout = new QVBoxLayout(tableGroup);
    tableLayout->setContentsMargins(0, 8, 0, 0);
    
    resultText_ = new QTextEdit(tableGroup);
    resultText_->setReadOnly(true);
    resultText_->setFont(QFont("JetBrains Mono, Consolas", 11));
    resultText_->setStyleSheet(
        "QTextEdit { background: #0A0A0A; color: #A1A1AA; border: 1px solid #222222; border-radius: 6px; padding: 8px; }"
    );
    resultText_->setPlaceholderText("Experiment results will appear here...");
    tableLayout->addWidget(resultText_);
    rightLayout->addWidget(tableGroup, 2);

    splitter->addWidget(rightWidget);
    splitter->setSizes({340, 600});
    mainLayout->addWidget(splitter);

    // 信号连接
    connect(runSelectedBtn_, &QPushButton::clicked,
            this, &ExperimentPanelWidget::runSelectedExperiment);
    connect(runAllBtn_, &QPushButton::clicked,
            this, &ExperimentPanelWidget::runAllExperiments);
    connect(clearBtn_, &QPushButton::clicked,
            this, &ExperimentPanelWidget::clearResults);
    connect(experimentList_, &QListWidget::itemSelectionChanged,
            this, &ExperimentPanelWidget::onListSelectionChanged);
}

QWidget* ExperimentPanelWidget::getResultWidget() const {
    return resultChart_;
}

// ── 运行逻辑 ──────────────────────────────────────────────────────────────────

void ExperimentPanelWidget::runSelectedExperiment() {
    if (simTimer_->isActive()) return;

    QList<QListWidgetItem*> selected = experimentList_->selectedItems();
    int firstIdx = -1;
    for (auto* item : selected) {
        int idx = item->data(Qt::UserRole).toInt();
        if (idx >= 0 && idx < presets_.size()) { firstIdx = idx; break; }
    }
    if (firstIdx < 0) {
        appendLog("[提示] 请先选择一个实验条目");
        return;
    }
    runAllMode_ = false;
    runExperiment(firstIdx);
}

void ExperimentPanelWidget::runAllExperiments() {
    if (simTimer_->isActive()) return;
    results_.clear();
    resultChart_->clear();
    resultText_->clear();
    runAllMode_ = true;
    runExperiment(0);
}

void ExperimentPanelWidget::clearResults() {
    if (simTimer_->isActive()) simTimer_->stop();
    results_.clear();
    resultChart_->clear();
    resultText_->clear();
    progressBar_->setValue(0);
    progressLabel_->setText("就绪");
    appendLog("[INFO] 实验结果已清除");
}

void ExperimentPanelWidget::runExperiment(int index) {
    if (index < 0 || index >= presets_.size()) return;
    currentExpIdx_ = index;
    simStep_       = 0;
    totalSteps_    = 30 + (index % 3) * 10;  // 30~50步模拟进度

    const auto& exp = presets_[index];
    progressLabel_->setText(QString("运行中: %1 (%2/%3)")
        .arg(exp.name).arg(index + 1).arg(presets_.size()));
    appendLog(QString("[START] 实验 %1: %2").arg(index + 1).arg(exp.name));
    appendLog(QString("        类别=%1 | CXL延迟=%2 ns | 带宽=%3 GB/s | MLP=%4 | 拥塞=%5")
        .arg(exp.category)
        .arg(exp.cxl_latency_ns, 0, 'f', 0)
        .arg(exp.bandwidth_gbps, 0, 'f', 0)
        .arg(exp.mlp_enabled ? "开" : "关")
        .arg(exp.congestion_enabled ? "开" : "关"));

    runSelectedBtn_->setEnabled(false);
    runAllBtn_->setEnabled(false);
    simTimer_->start();
}

void ExperimentPanelWidget::onExperimentTick() {
    simStep_++;
    int overallDone = results_.size();
    int overallPct  = (int)((overallDone + simStep_ / (double)totalSteps_)
                            / presets_.size() * 100);
    progressBar_->setValue(overallPct);

    if (simStep_ >= totalSteps_) {
        simTimer_->stop();
        finishCurrentExperiment();
    }
}

void ExperimentPanelWidget::finishCurrentExperiment() {
    const auto& exp = presets_[currentExpIdx_];

    // 生成模拟实验结果（基于物理模型估算）
    ExperimentResult r;
    r.name    = exp.name;
    r.category = exp.category;
    r.finished = true;

    // 延迟模型：基础CXL延迟 + 拥塞附加
    double congestionPenalty = exp.congestion_enabled ? (exp.num_devices * 15.0) : 0.0;
    double mlpGain           = exp.mlp_enabled ? 0.72 : 1.0;
    r.cxl_latency_ns  = exp.cxl_latency_ns;
    r.avg_latency_ns  = (exp.cxl_latency_ns + congestionPenalty) * mlpGain;

    // 带宽模型：实际带宽 = 声称带宽 * 利用率
    double utilization  = exp.congestion_enabled ? 0.65 : 0.82;
    r.bandwidth_gbps    = exp.bandwidth_gbps * utilization * (exp.mlp_enabled ? 1.15 : 1.0);

    // 缺失率：受延迟影响
    r.miss_rate_pct     = std::min(95.0, 8.0 + exp.cxl_latency_ns / 12.0
                                  + (exp.congestion_enabled ? 12.0 : 0.0));
    r.total_accesses    = 1000000ULL + currentExpIdx_ * 50000ULL;
    r.mlp_enabled       = exp.mlp_enabled;
    r.congestion_enabled = exp.congestion_enabled;

    results_.append(r);
    appendLog(QString("[DONE]  延迟=%1 ns | 带宽=%2 GB/s | 缺失率=%3%")
        .arg(r.avg_latency_ns, 0, 'f', 1)
        .arg(r.bandwidth_gbps, 0, 'f', 1)
        .arg(r.miss_rate_pct,  0, 'f', 1));

    updateResultsChart();

    int nextIdx = currentExpIdx_ + 1;
    if (runAllMode_ && nextIdx < presets_.size()) {
        runExperiment(nextIdx);
    } else {
        runAllMode_ = false;
        runSelectedBtn_->setEnabled(true);
        runAllBtn_->setEnabled(true);
        progressBar_->setValue(100);
        progressLabel_->setText(QString("完成！共 %1 项实验").arg(results_.size()));
        appendLog(QString("[全部完成] 共运行 %1 组实验").arg(results_.size()));
        emit resultsReady();
    }
}

void ExperimentPanelWidget::updateResultsChart() {
    resultChart_->setResults(results_);

    // 更新数据表格
    resultText_->clear();
    QTextCursor cur = resultText_->textCursor();

    // 表头
    QTextCharFormat headerFmt;
    headerFmt.setForeground(QColor(0x4F, 0xC3, 0xF7));
    headerFmt.setFontWeight(QFont::Bold);
    cur.setCharFormat(headerFmt);
    cur.insertText(QString("%1  %2  %3  %4  %5  %6\n")
        .arg("No.",  3).arg("实验名称", -18)
        .arg("类别", -12).arg("延迟(ns)", -10)
        .arg("带宽(GB/s)", -10).arg("缺失率", -8));
    cur.insertText(QString(72, '-') + "\n");

    QTextCharFormat normalFmt;
    normalFmt.setForeground(QColor(0xB0, 0xBE, 0xC5));
    cur.setCharFormat(normalFmt);

    for (int i = 0; i < results_.size(); ++i) {
        const auto& r = results_[i];
        cur.insertText(QString("%1.  %2  %3  %4  %5  %6%\n")
            .arg(i + 1, 2)
            .arg(r.name.left(17),     -18)
            .arg(r.category.left(11), -12)
            .arg(r.avg_latency_ns, 9, 'f', 1)
            .arg(r.bandwidth_gbps, 9, 'f', 1)
            .arg(r.miss_rate_pct,  7, 'f', 1));
    }

    resultText_->setTextCursor(cur);
}

void ExperimentPanelWidget::onListSelectionChanged() {
    QList<QListWidgetItem*> sel = experimentList_->selectedItems();
    for (auto* item : sel) {
        int idx = item->data(Qt::UserRole).toInt();
        if (idx >= 0 && idx < presets_.size()) {
            const auto& exp = presets_[idx];
            descLabel_->setText(
                QString("<b>%1</b><br/><span style='color:#9E9E9E;'>%2</span>"
                        "<br/>CXL延迟: <b style='color:#4FC3F7;'>%3 ns</b> | "
                        "带宽: <b style='color:#81C784;'>%4 GB/s</b> | "
                        "MLP: %5 | 拥塞: %6")
                .arg(exp.name).arg(exp.description)
                .arg(exp.cxl_latency_ns).arg(exp.bandwidth_gbps)
                .arg(exp.mlp_enabled ? "<b style='color:#81C784;'>开</b>"
                                     : "<span style='color:#EF5350;'>关</span>")
                .arg(exp.congestion_enabled ? "<b style='color:#81C784;'>开</b>"
                                            : "<span style='color:#EF5350;'>关</span>"));
            break;
        }
    }
}

void ExperimentPanelWidget::appendLog(const QString& msg) {
    emit logMessage(msg);
}

void ExperimentPanelWidget::exportResults(const QString& filename) {
    if (results_.isEmpty()) {
        emit logMessage("[提示] 没有实验结果可导出，请先运行实验");
        return;
    }
    if (filename.endsWith(".json")) {
        QJsonArray arr;
        for (const auto& r : results_) {
            QJsonObject obj;
            obj["name"]             = r.name;
            obj["category"]         = r.category;
            obj["avg_latency_ns"]   = r.avg_latency_ns;
            obj["bandwidth_gbps"]   = r.bandwidth_gbps;
            obj["miss_rate_pct"]    = r.miss_rate_pct;
            obj["total_accesses"]   = static_cast<qint64>(r.total_accesses);
            obj["mlp_enabled"]      = r.mlp_enabled;
            obj["congestion_enabled"] = r.congestion_enabled;
            arr.append(obj);
        }
        QFile f(filename);
        if (f.open(QIODevice::WriteOnly))
            f.write(QJsonDocument(arr).toJson());
    } else {
        // CSV
        QFile f(filename);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
        QTextStream ts(&f);
        ts << "编号,名称,类别,平均延迟(ns),带宽(GB/s),L3缺失率(%),总访问次数,MLP优化,拥塞模型\n";
        for (int i = 0; i < results_.size(); ++i) {
            const auto& r = results_[i];
            ts << (i + 1) << ","
               << r.name << "," << r.category << ","
               << QString::number(r.avg_latency_ns, 'f', 2) << ","
               << QString::number(r.bandwidth_gbps, 'f', 2) << ","
               << QString::number(r.miss_rate_pct,  'f', 2) << ","
               << r.total_accesses << ","
               << (r.mlp_enabled ? "是" : "否") << ","
               << (r.congestion_enabled ? "是" : "否") << "\n";
        }
    }
    emit logMessage("[INFO] 数据已导出: " + filename);
}
