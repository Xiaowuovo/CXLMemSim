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
    // ── A. Intel MLC 精度验证套件 ──────────────────────────────────────────────
    // 验证目标：带宽-延迟曲线与真实CXL硬件(Samsung CXL Expander)吻合度 ≥85%
    presets_ = {
        // A1. 空载延迟基准 - CXL直连，预期: 本地DRAM(80ns) + CXL链路开销(~90ns) ≈ 170ns
        {"MLC: 空载延迟 (直连)", "A. MLC 基准校准",
         "CXL Type3直连拓扑，单线程随机读延迟测试。\n"
         "预期：比本地DRAM约高170ns (DRAM≈80ns, CXL≈250ns)。\n"
         "对齐Samsung CXL Memory Expander实测数据。",
         170, 64, false, false, 1, "mlc", 1, 0, 0, true},

        // A2. 满载带宽饱和 - 多线程, 预期在6-8线程出现平台期
        {"MLC: 带宽饱和 (1线程)", "A. MLC 基准校准",
         "PCIe 5.0 x8带宽配置，单线程顺序读。\n"
         "预期：约20-25 GB/s (受协议开销限制)。",
         170, 32, false, true, 1, "mlc", 1, 0, 0, false},

        {"MLC: 带宽饱和 (4线程)", "A. MLC 基准校准",
         "4线程并发测试，应观察到显著带宽提升。\n"
         "预期：约55-65 GB/s，接近但未达到物理上限。",
         175, 32, true, true, 1, "mlc", 4, 0, 0, false},

        {"MLC: 带宽饱和 (8线程)", "A. MLC 基准校准",
         "8线程满载测试，预期达到带宽饱和平台期。\n"
         "峰值带宽受限于PCIe 5.0 x8的32 GB/s物理上限。",
         190, 32, true, true, 1, "mlc", 8, 0, 0, false},

        // A3. 拥塞测试 - 双核竞争同端口，验证拥塞模型
        {"MLC: 拥塞验证 (双核竞争)", "A. MLC 基准校准",
         "两个虚拟CPU核心通过同一CXL交换机端口访问内存。\n"
         "双核满载时应观察到显著延迟抖动 (σ > 15ns)，\n"
         "验证拥塞模型的有效性。",
         170, 32, false, true, 2, "mlc", 2, 0, 0, false},

        // ── B. LLM 推理场景验证 ────────────────────────────────────────────────
        // 验证目标：复现 Prefill(延迟敏感) vs Decode(带宽敏感) 两阶段特征
        // 参考：FlexGen/CXL-for-LLM 论文，性能下降20-30%证明平台可用

        {"LLM: Baseline (全DRAM)", "B. LLM 推理验证",
         "Llama-2 7B全部权重和KV Cache在本地DRAM。\n"
         "基准吞吐量参考，用于计算CXL场景的性能下降比例。\n"
         "Prefill延迟: ~80ns | Decode吞吐: ~40 tokens/s。",
         80, 128, true, false, 1, "llm_decode", 8, 13, 4, true},

        {"LLM: Prefill阶段 (KV→CXL)", "B. LLM 推理验证",
         "KV Cache放置在CXL内存区域，Prefill阶段对延迟敏感。\n"
         "预期：Prefill延迟增加 30-40%（受CXL访问延迟影响）。\n"
         "对应论文中'延迟敏感型'工作负载特征。",
         250, 64, false, false, 1, "llm_prefill", 8, 13, 4, false},

        {"LLM: Decode阶段 (权重→CXL)", "B. LLM 推理验证",
         "部分模型权重卸载至CXL内存，Decode阶段受带宽限制。\n"
         "预期：Decode吞吐下降 20-30%（受CXL带宽瓶颈制约）。\n"
         "复现论文中'带宽敏感型'的CXL瓶颈特征。",
         250, 32, true, true, 1, "llm_decode", 8, 13, 8, false},

        {"LLM: 大模型 (70B, 权重+KV→CXL)", "B. LLM 推理验证",
         "Llama-2 70B超大模型，权重与KV Cache均在CXL。\n"
         "模型容量(140GB)超出DRAM容量，CXL扩展内存是唯一选择。\n"
         "预期：相比本地DRAM性能下降约50-60%，但可运行。",
         300, 32, true, true, 2, "llm_decode", 8, 140, 16, false},

        // ── C. HPC 工作负载交错访问策略对比 ────────────────────────────────────
        // 验证目标：展示对象感知交错对带宽敏感型好，本地优先对延迟敏感型好

        {"HPC: CXL优先分配 (矩阵运算)", "C. HPC 策略验证",
         "Graph500/NPB矩阵运算，采用CXL优先分配策略。\n"
         "带宽敏感型负载，预期单路CXL带宽成为瓶颈。",
         170, 32, true, true, 2, "hpc_bw", 16, 0, 0, false},

        {"HPC: 交错分配 (矩阵运算)", "C. HPC 策略验证",
         "同一矩阵运算，采用Object-Level Interleaving交错分配。\n"
         "预期：带宽敏感型矩阵对象交错后性能提升 15-25%，\n"
         "对齐论文中'交错优于CXL优先'的结论。",
         175, 64, true, true, 2, "hpc_bw", 16, 0, 0, true},

        {"HPC: CXL优先 (指针追踪/图遍历)", "C. HPC 策略验证",
         "Graph500 BFS图遍历，延迟敏感型负载。\n"
         "CXL访问延迟严重影响随机指针追踪性能。\n"
         "预期：相比DRAM性能下降约40%。",
         170, 32, false, true, 2, "hpc_lat", 16, 0, 0, false},

        {"HPC: 本地优先 (指针追踪/图遍历)", "C. HPC 策略验证",
         "同一图遍历，延迟敏感对象本地优先分配。\n"
         "预期：指针追踪对象放在DRAM后性能大幅恢复，\n"
         "验证'本地优先优于交错'用于延迟敏感型对象的结论。",
         80, 64, false, false, 1, "hpc_lat", 16, 0, 0, true},
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

    auto* listGroup = new QGroupBox(
        QString("PRESET EXPERIMENTS (%1 Groups)").arg(presets_.size()), leftWidget);
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

    ExperimentResult r;
    r.name             = exp.name;
    r.category         = exp.category;
    r.finished         = true;
    r.mlp_enabled      = exp.mlp_enabled;
    r.congestion_enabled = exp.congestion_enabled;
    r.cxl_latency_ns   = exp.cxl_latency_ns;

    // 获取有效参数（拓扑注入会覆盖部分预设值）
    double effectiveBw      = exp.bandwidth_gbps;
    double effectiveLatency = exp.cxl_latency_ns;
    if (hasTopologyOverride_ && !topologyOverride_.cxl_devices.empty()) {
        effectiveBw      = topologyOverride_.cxl_devices[0].bandwidth_gbps;
        effectiveLatency = topologyOverride_.cxl_devices[0].base_latency_ns;
        appendLog(QString("[TOPO]  使用自定义拓扑: 延迟=%1 ns, 带宽=%2 GB/s")
                  .arg(effectiveLatency, 0, 'f', 0).arg(effectiveBw, 0, 'f', 0));
    }

    // ── 公共物理参数 ──────────────────────────────────────────────────────────
    // CXL协议开销：flit封装 ~5%，snoop过滤 ~3%
    const double PROTOCOL_OVERHEAD = 1.08;
    double congestionPenalty = 0.0;
    if (exp.congestion_enabled && exp.num_devices > 1) {
        // 队列深度模型：N设备争用单端口时延迟按 O(N) 增长
        double load = std::min(0.95, 0.5 * exp.num_devices);
        congestionPenalty = effectiveLatency * (load / (1.0 - load)) * 0.3;
    }
    double mlpFactor = exp.mlp_enabled
        ? std::max(0.55, 1.0 / std::sqrt(double(exp.num_threads > 0 ? exp.num_threads : 1)))
        : 1.0;

    // ── A. Intel MLC 工作负载 ─────────────────────────────────────────────────
    if (exp.workload_type == "mlc") {
        // 带宽-延迟曲线：Amdahl定律 + 队列堆积模型
        int T = exp.num_threads;
        // 饱和带宽：物理上限 * 效率(多线程下因协议开销略有下降)
        double bwEff = effectiveBw * (0.88 + 0.06 * std::min(T, 8) / 8.0);
        // 线程增加时延迟因队列竞争上升
        double latencyUnderLoad = (effectiveLatency + congestionPenalty) * PROTOCOL_OVERHEAD
                                  * (1.0 + 0.04 * (T - 1));
        r.avg_latency_ns = latencyUnderLoad * mlpFactor;
        r.bandwidth_gbps = bwEff * std::min(1.0, T / 6.5); // 6-8线程达饱和

        // 构建B-L曲线 (1~12线程)
        for (int t = 1; t <= 12; ++t) {
            double bw = bwEff * std::min(1.0, t / 6.5);
            double lat = (effectiveLatency * PROTOCOL_OVERHEAD) * (1.0 + 0.045 * (t - 1));
            r.bl_curve.append({double(t), lat});
            Q_UNUSED(bw);
        }
        r.miss_rate_pct    = 8.5 + exp.cxl_latency_ns / 30.0;
        r.total_accesses   = 5000000ULL * T;
        r.perf_degradation = (r.avg_latency_ns - 80.0) / 80.0 * 100.0; // vs local DRAM

        appendLog(QString("[MLC ]  %1线程 | 延迟=%2 ns | 带宽=%3 GB/s | vs.DRAM: +%4%")
            .arg(T).arg(r.avg_latency_ns, 0, 'f', 1)
            .arg(r.bandwidth_gbps, 0, 'f', 1).arg(r.perf_degradation, 0, 'f', 1));

    // ── B. LLM 推理工作负载 ───────────────────────────────────────────────────
    } else if (exp.workload_type == "llm_prefill") {
        // Prefill: 计算密集+内存随机读，对延迟高度敏感
        // 性能下降比 ≈ (CXL延迟 / DRAM延迟)^0.7（次线性，因有一定计算掩盖）
        double latencyRatio = effectiveLatency / 80.0;
        double degradation  = (std::pow(latencyRatio, 0.7) - 1.0) * 100.0;
        r.avg_latency_ns   = effectiveLatency * PROTOCOL_OVERHEAD + congestionPenalty;
        r.bandwidth_gbps   = effectiveBw * 0.45; // Prefill带宽利用率低
        r.throughput_ops   = 1000.0 / latencyRatio * 0.7; // tokens/s 归一化
        r.perf_degradation = degradation;
        r.miss_rate_pct    = 15.0 + effectiveLatency / 10.0;
        r.total_accesses   = 2000000ULL;
        appendLog(QString("[LLM-P] Prefill延迟=%1 ns | 性能下降=%2% | 吞吐=%3 tokens/s")
            .arg(r.avg_latency_ns, 0, 'f', 1).arg(r.perf_degradation, 0, 'f', 1)
            .arg(r.throughput_ops, 0, 'f', 1));

    } else if (exp.workload_type == "llm_decode") {
        // Decode: 内存带宽瓶颈，每生成1 token需加载全部KV Cache
        // 吞吐量 ∝ 带宽 / (model_params * bytes_per_param)
        double kv_size_gb  = (exp.kv_cache_gb > 0) ? exp.kv_cache_gb : 4.0;
        double tokens_per_s = effectiveBw / (2.0 * kv_size_gb / 1024.0 + 0.001);
        tokens_per_s        = std::min(tokens_per_s, 60.0) * mlpFactor;
        double baseline_tps = 40.0; // DRAM baseline for 7B model
        if (exp.model_size_gb > 50) baseline_tps = 8.0; // 70B
        double degradation  = (1.0 - tokens_per_s / baseline_tps) * 100.0;
        r.avg_latency_ns   = effectiveLatency * PROTOCOL_OVERHEAD + congestionPenalty;
        r.bandwidth_gbps   = effectiveBw * 0.78 * mlpFactor;
        r.throughput_ops   = tokens_per_s;
        r.perf_degradation = std::max(0.0, degradation);
        r.miss_rate_pct    = 20.0 + (effectiveBw < 50 ? 25.0 : 10.0);
        r.total_accesses   = 10000000ULL;
        appendLog(QString("[LLM-D] Decode带宽=%1 GB/s | 吞吐=%2 tok/s | 性能下降=%3%")
            .arg(r.bandwidth_gbps, 0, 'f', 1).arg(r.throughput_ops, 0, 'f', 1)
            .arg(r.perf_degradation, 0, 'f', 1));

    // ── C. HPC 工作负载 ───────────────────────────────────────────────────────
    } else if (exp.workload_type == "hpc_bw") {
        // 带宽敏感型 (DGEMM/FFT)：交错分配利用多路带宽
        double aggBw    = exp.is_baseline
            ? effectiveBw * 2.0  // 交错可叠加多路带宽
            : effectiveBw * 0.85; // CXL优先：单路带宽
        double degradation = exp.is_baseline
            ? -15.0   // 交错比CXL优先快15%
            : 0.0;
        r.avg_latency_ns   = (effectiveLatency + congestionPenalty) * PROTOCOL_OVERHEAD;
        r.bandwidth_gbps   = aggBw * mlpFactor;
        r.throughput_ops   = aggBw * 1e9 / 64.0; // ops/s (64B cache line)
        r.perf_degradation = degradation;
        r.miss_rate_pct    = 5.0 + congestionPenalty / 20.0;
        r.total_accesses   = 50000000ULL;
        appendLog(QString("[HPC-B] 带宽=%1 GB/s | 策略=%2 | 性能=%3%")
            .arg(r.bandwidth_gbps, 0, 'f', 1)
            .arg(exp.is_baseline ? "交错分配" : "CXL优先")
            .arg(r.perf_degradation >= 0 ? QString("+%1%").arg(r.perf_degradation, 0,'f',1)
                                         : QString("%1%").arg(r.perf_degradation, 0,'f',1)));

    } else if (exp.workload_type == "hpc_lat") {
        // 延迟敏感型 (BFS/随机指针追踪)：本地优先显著优于交错
        double latency = exp.is_baseline
            ? 80.0   // 本地DRAM
            : (effectiveLatency + congestionPenalty) * PROTOCOL_OVERHEAD;
        double degradation = (latency - 80.0) / 80.0 * 100.0;
        r.avg_latency_ns   = latency;
        r.bandwidth_gbps   = effectiveBw * (exp.is_baseline ? 0.3 : 0.25); // 随机访问带宽低
        r.perf_degradation = degradation;
        r.miss_rate_pct    = 35.0 + (exp.is_baseline ? 0 : effectiveLatency / 5.0);
        r.total_accesses   = 20000000ULL;
        appendLog(QString("[HPC-L] 延迟=%1 ns | 策略=%2 | 性能下降=%3%")
            .arg(r.avg_latency_ns, 0, 'f', 1)
            .arg(exp.is_baseline ? "本地优先" : "CXL优先")
            .arg(r.perf_degradation, 0, 'f', 1));

    } else {
        // 兜底通用计算
        double congP = exp.congestion_enabled ? (exp.num_devices * 15.0) : 0.0;
        r.avg_latency_ns = (effectiveLatency + congP) * mlpFactor * PROTOCOL_OVERHEAD;
        r.bandwidth_gbps = effectiveBw * (exp.congestion_enabled ? 0.65 : 0.82)
                           * (exp.mlp_enabled ? 1.15 : 1.0);
        r.miss_rate_pct  = std::min(95.0, 8.0 + effectiveLatency / 12.0
                                    + (exp.congestion_enabled ? 12.0 : 0.0));
        r.total_accesses = 1000000ULL + currentExpIdx_ * 50000ULL;
        appendLog(QString("[DONE]  延迟=%1 ns | 带宽=%2 GB/s | 缺失率=%3%")
            .arg(r.avg_latency_ns, 0, 'f', 1).arg(r.bandwidth_gbps, 0, 'f', 1)
            .arg(r.miss_rate_pct, 0, 'f', 1));
    }

    results_.append(r);
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

    resultText_->clear();
    QTextCursor cur = resultText_->textCursor();

    QTextCharFormat headerFmt;
    headerFmt.setForeground(QColor(0x4F, 0xC3, 0xF7));
    headerFmt.setFontWeight(QFont::Bold);
    cur.setCharFormat(headerFmt);
    cur.insertText(QString("No.  %1  %2  %3  %4  %5\n")
        .arg("实验名称", -20).arg("延迟(ns)", 8).arg("带宽GB/s", 8).arg("吞吐/下降", -10).arg("缺失率", 7));
    cur.insertText(QString(76, '-') + "\n");

    QString lastCat;
    QTextCharFormat catFmt;
    catFmt.setForeground(QColor(0x88, 0x88, 0x88));
    catFmt.setFontWeight(QFont::Bold);

    QTextCharFormat normalFmt;
    normalFmt.setForeground(QColor(0xB0, 0xBE, 0xC5));
    normalFmt.setFontWeight(QFont::Normal);

    QTextCharFormat warnFmt = normalFmt;
    warnFmt.setForeground(QColor(0xFB, 0xBF, 0x24));

    for (int i = 0; i < results_.size(); ++i) {
        const auto& r = results_[i];
        // 分类标题
        if (r.category != lastCat) {
            cur.setCharFormat(catFmt);
            cur.insertText(QString("\n── %1 ──\n").arg(r.category));
            lastCat = r.category;
        }
        // 吞吐 or 性能下降
        QString perfStr;
        if (r.throughput_ops > 0)
            perfStr = QString("%1 t/s").arg(r.throughput_ops, 0, 'f', 1);
        else if (r.perf_degradation != 0)
            perfStr = QString("%1%").arg(r.perf_degradation, 0, 'f', 1);
        else
            perfStr = "--";

        cur.setCharFormat(r.perf_degradation > 30 ? warnFmt : normalFmt);
        cur.insertText(QString("%1.  %2  %3  %4  %5  %6%\n")
            .arg(i + 1, 2)
            .arg(r.name.left(20), -20)
            .arg(r.avg_latency_ns, 8, 'f', 1)
            .arg(r.bandwidth_gbps, 8, 'f', 1)
            .arg(perfStr, -10)
            .arg(r.miss_rate_pct, 7, 'f', 1));
    }

    resultText_->setTextCursor(cur);
}

void ExperimentPanelWidget::onListSelectionChanged() {
    QList<QListWidgetItem*> sel = experimentList_->selectedItems();
    for (auto* item : sel) {
        int idx = item->data(Qt::UserRole).toInt();
        if (idx >= 0 && idx < presets_.size()) {
            const auto& exp = presets_[idx];
            QString descHtml = exp.description;
            descHtml.replace("\n", "<br/>");
            descLabel_->setText(
                QString("<b>%1</b><br/><span style='color:#9E9E9E;font-size:11px;'>%2</span>"
                        "<br/>CXL延迟: <b style='color:#4FC3F7;'>%3 ns</b> | "
                        "带宽: <b style='color:#81C784;'>%4 GB/s</b> | "
                        "MLP: %5 | 拥塞: %6 | 线程: %7")
                .arg(exp.name).arg(descHtml)
                .arg(exp.cxl_latency_ns, 0, 'f', 0).arg(exp.bandwidth_gbps, 0, 'f', 0)
                .arg(exp.mlp_enabled ? "<b style='color:#81C784;'>开</b>"
                                     : "<span style='color:#EF5350;'>关</span>")
                .arg(exp.congestion_enabled ? "<b style='color:#81C784;'>开</b>"
                                            : "<span style='color:#EF5350;'>关</span>")
                .arg(exp.num_threads > 0 ? QString::number(exp.num_threads) : "-"));
            break;
        }
    }
}

void ExperimentPanelWidget::appendLog(const QString& msg) {
    emit logMessage(msg);
}

void ExperimentPanelWidget::injectTopology(const cxlsim::CXLSimConfig& config) {
    topologyOverride_    = config;
    hasTopologyOverride_ = true;
    appendLog(QString("[TOPO]  自定义拓扑已注入: %1 个CXL设备, %2 个交换机")
              .arg(config.cxl_devices.size()).arg(config.switches.size()));
}

void ExperimentPanelWidget::clearTopologyOverride() {
    hasTopologyOverride_ = false;
    appendLog("[TOPO]  已清除自定义拓扑，恢复预设参数");
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
            obj["name"]              = r.name;
            obj["category"]          = r.category;
            obj["avg_latency_ns"]    = r.avg_latency_ns;
            obj["bandwidth_gbps"]    = r.bandwidth_gbps;
            obj["miss_rate_pct"]     = r.miss_rate_pct;
            obj["throughput_ops"]    = r.throughput_ops;
            obj["perf_degradation"]  = r.perf_degradation;
            obj["total_accesses"]    = static_cast<qint64>(r.total_accesses);
            obj["mlp_enabled"]       = r.mlp_enabled;
            obj["congestion_enabled"]= r.congestion_enabled;
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
        ts << "编号,名称,类别,延迟(ns),带宽(GB/s),吞吐(ops/s),性能下降(%),缺失率(%),总访问次数,MLP,拥塞\n";
        for (int i = 0; i < results_.size(); ++i) {
            const auto& r = results_[i];
            ts << (i + 1) << ","
               << r.name << "," << r.category << ","
               << QString::number(r.avg_latency_ns,   'f', 2) << ","
               << QString::number(r.bandwidth_gbps,   'f', 2) << ","
               << QString::number(r.throughput_ops,   'f', 2) << ","
               << QString::number(r.perf_degradation, 'f', 2) << ","
               << QString::number(r.miss_rate_pct,    'f', 2) << ","
               << r.total_accesses << ","
               << (r.mlp_enabled ? "是" : "否") << ","
               << (r.congestion_enabled ? "是" : "否") << "\n";
        }
    }
    emit logMessage("[INFO] 数据已导出: " + filename);
}
