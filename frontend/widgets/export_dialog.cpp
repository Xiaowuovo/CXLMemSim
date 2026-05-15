#include "export_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QTimer>
#include <QSplitter>
#include <QUuid>
#include <cmath>
#include <algorithm>

// ─────────────────────────────────────────────────────────
//  Shared style helpers
// ─────────────────────────────────────────────────────────
static const char* GRP_STYLE =
    "QGroupBox { color: #AAAAAA; font-size: 12px; font-weight: bold; "
    "border: 1px solid #2A2A2A; border-radius: 6px; padding-top: 18px; "
    "background: #080808; margin-top: 8px; }"
    "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; }";

static const char* CHK_GREEN =
    "QCheckBox { color: #4ADE80; font-size: 12px; font-weight: 600; }"
    "QCheckBox::indicator { width:14px; height:14px; border:1px solid #166534; border-radius:3px; background:#0A0A0A; }"
    "QCheckBox::indicator:checked { background:#166534; }";

static const char* CHK_BLUE =
    "QCheckBox { color: #60A5FA; font-size: 12px; font-weight: 600; }"
    "QCheckBox::indicator { width:14px; height:14px; border:1px solid #1E3A5F; border-radius:3px; background:#0A0A0A; }"
    "QCheckBox::indicator:checked { background:#1E3A5F; }";

static const char* CHK_AMBER =
    "QCheckBox { color: #FBBF24; font-size: 12px; font-weight: 600; }"
    "QCheckBox::indicator { width:14px; height:14px; border:1px solid #78350F; border-radius:3px; background:#0A0A0A; }"
    "QCheckBox::indicator:checked { background:#78350F; }";

static const char* SPIN_STYLE =
    "QSpinBox { background:#0A0A0A; color:#EDEDED; border:1px solid #333; "
    "border-radius:4px; padding:3px 8px; font-size:12px; }"
    "QSpinBox:disabled { color:#444; }";

static const char* RADIO_STYLE = "QRadioButton { color:#EDEDED; font-size:12px; }";

static QString makeBtn(const QString& fg, const QString& border, const QString& hoverBg) {
    return QString(
        "QPushButton { background:#0A0A0A; color:%1; border:1px solid %2; "
        "border-radius:6px; padding:8px 20px; font-size:13px; font-weight:600; }"
        "QPushButton:hover { background:%3; }"
        "QPushButton:disabled { color:#444; border-color:#1A1A1A; }").arg(fg, border, hoverBg);
}

// ─────────────────────────────────────────────────────────

ExportDialog::ExportDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("导出实验数据 — CXLMemSim");
    setMinimumSize(640, 520);
    resize(720, 580);
    setupUI();
}

void ExportDialog::setupUI() {
    setStyleSheet("QDialog { background: #0A0A0A; } "
                  "QTabWidget::pane { border: 1px solid #2A2A2A; background: #080808; } "
                  "QTabBar::tab { background:#111; color:#888; padding:8px 18px; border:1px solid #2A2A2A; "
                  "  border-bottom:none; border-radius:4px 4px 0 0; font-size:12px; }"
                  "QTabBar::tab:selected { background:#080808; color:#EDEDED; border-bottom:1px solid #080808; }"
                  "QTabBar::tab:hover:!selected { background:#1A1A1A; color:#AAAAAA; }");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(10);

    // 顶部标题 + 数据摘要
    auto* headerRow = new QHBoxLayout();
    auto* titleLabel = new QLabel("📦 导出实验数据", this);
    titleLabel->setStyleSheet("font-size:15px; font-weight:700; color:#EDEDED;");
    headerRow->addWidget(titleLabel);
    headerRow->addStretch();

    summaryLabel_ = new QLabel("暂无数据", this);
    summaryLabel_->setStyleSheet("font-size:11px; color:#888; padding:4px 10px; "
        "background:#111; border:1px solid #2A2A2A; border-radius:4px;");
    headerRow->addWidget(summaryLabel_);
    mainLayout->addLayout(headerRow);

    // Tab 容器
    auto* tabs = new QTabWidget(this);
    mainLayout->addWidget(tabs, 1);

    auto* exportTab  = new QWidget(this);
    auto* historyTab = new QWidget(this);
    auto* noteTab    = new QWidget(this);
    tabs->addTab(exportTab,  "⬇ 导出当前会话");
    tabs->addTab(historyTab, "📋 历史会话");
    tabs->addTab(noteTab,    "📝 实验备注");

    setupExportTab(exportTab);
    setupHistoryTab(historyTab);
    setupNoteTab(noteTab);

    // 底部按钮
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    cancelBtn_ = new QPushButton("取消", this);
    cancelBtn_->setStyleSheet(makeBtn("#888", "#333", "#1A1A1A"));
    cancelBtn_->setMinimumHeight(34);
    cancelBtn_->setCursor(Qt::PointingHandCursor);
    connect(cancelBtn_, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn_);

    exportBtn_ = new QPushButton("⬇ 立即导出", this);
    exportBtn_->setStyleSheet(makeBtn("#4ADE80", "#166534", "#0D3D26"));
    exportBtn_->setMinimumHeight(34);
    exportBtn_->setMinimumWidth(120);
    exportBtn_->setCursor(Qt::PointingHandCursor);
    connect(exportBtn_, &QPushButton::clicked, this, &ExportDialog::onExport);
    btnLayout->addWidget(exportBtn_);

    mainLayout->addLayout(btnLayout);
}

// ─────────────────────────────────────────────────────────
//  Tab 1: 导出当前会话
// ─────────────────────────────────────────────────────────
void ExportDialog::setupExportTab(QWidget* tab) {
    auto* L = new QVBoxLayout(tab);
    L->setContentsMargins(14, 14, 14, 10);
    L->setSpacing(10);

    // ── 范围选择 ──
    auto* rangeGrp = new QGroupBox("导出范围", tab);
    rangeGrp->setStyleSheet(GRP_STYLE);
    auto* rL = new QVBoxLayout(rangeGrp);
    rL->setContentsMargins(14, 18, 14, 12);
    rL->setSpacing(8);

    allEpochsRadio_ = new QRadioButton("全部记录 (All Epochs)", tab);
    allEpochsRadio_->setStyleSheet(RADIO_STYLE);
    allEpochsRadio_->setChecked(true);
    rL->addWidget(allEpochsRadio_);

    auto* recentRow = new QHBoxLayout();
    recentEpochsRadio_ = new QRadioButton("最近 N 个 Epoch：", tab);
    recentEpochsRadio_->setStyleSheet(RADIO_STYLE);
    recentRow->addWidget(recentEpochsRadio_);
    recentCountSpin_ = new QSpinBox(tab);
    recentCountSpin_->setRange(100, 500000);
    recentCountSpin_->setValue(10000);
    recentCountSpin_->setSingleStep(1000);
    recentCountSpin_->setEnabled(false);
    recentCountSpin_->setFixedWidth(90);
    recentCountSpin_->setStyleSheet(SPIN_STYLE);
    recentRow->addWidget(recentCountSpin_);
    recentRow->addWidget(new QLabel("Epochs", tab));
    recentRow->addStretch();
    rL->addLayout(recentRow);

    auto* rangeRow = new QHBoxLayout();
    epochRangeRadio_ = new QRadioButton("指定区间  Epoch", tab);
    epochRangeRadio_->setStyleSheet(RADIO_STYLE);
    rangeRow->addWidget(epochRangeRadio_);
    rangeStartSpin_ = new QSpinBox(tab);
    rangeStartSpin_->setRange(0, 999999); rangeStartSpin_->setValue(0);
    rangeStartSpin_->setEnabled(false); rangeStartSpin_->setFixedWidth(80);
    rangeStartSpin_->setStyleSheet(SPIN_STYLE);
    rangeRow->addWidget(rangeStartSpin_);
    rangeRow->addWidget(new QLabel("→", tab));
    rangeEndSpin_ = new QSpinBox(tab);
    rangeEndSpin_->setRange(0, 999999); rangeEndSpin_->setValue(999999);
    rangeEndSpin_->setEnabled(false); rangeEndSpin_->setFixedWidth(80);
    rangeEndSpin_->setStyleSheet(SPIN_STYLE);
    rangeRow->addWidget(rangeEndSpin_);
    rangeRow->addStretch();
    rL->addLayout(rangeRow);

    for (auto* lbl : tab->findChildren<QLabel*>())
        lbl->setStyleSheet("color:#AAAAAA; font-size:12px;");

    connect(allEpochsRadio_,   &QRadioButton::toggled, this, &ExportDialog::onRangeChanged);
    connect(recentEpochsRadio_,&QRadioButton::toggled, this, &ExportDialog::onRangeChanged);
    connect(epochRangeRadio_,  &QRadioButton::toggled, this, &ExportDialog::onRangeChanged);
    L->addWidget(rangeGrp);

    // ── 格式选择 ──
    auto* fmtGrp = new QGroupBox("导出格式", tab);
    fmtGrp->setStyleSheet(GRP_STYLE);
    auto* fL = new QVBoxLayout(fmtGrp);
    fL->setContentsMargins(14, 18, 14, 12); fL->setSpacing(8);

    csvCheckbox_ = new QCheckBox("性能数据时间序列 (.csv)  — 含完整数据字典，可直接导入 Excel / Python", tab);
    csvCheckbox_->setChecked(true);
    csvCheckbox_->setStyleSheet(CHK_GREEN);
    fL->addWidget(csvCheckbox_);

    jsonCheckbox_ = new QCheckBox("实验配置快照 (.json)  — 拓扑、负载、模拟参数完整记录", tab);
    jsonCheckbox_->setStyleSheet(CHK_BLUE);
    fL->addWidget(jsonCheckbox_);

    markdownCheckbox_ = new QCheckBox("Markdown 实验报告 (.md)  — 含摘要表格 + 关键指标，适合论文附录", tab);
    markdownCheckbox_->setStyleSheet(CHK_AMBER);
    fL->addWidget(markdownCheckbox_);
    L->addWidget(fmtGrp);

    // ── 会话标签 ──
    auto* tagGrp = new QGroupBox("会话标签 (用于历史记录检索)", tab);
    tagGrp->setStyleSheet(GRP_STYLE);
    auto* tL = new QHBoxLayout(tagGrp);
    tL->setContentsMargins(14, 18, 14, 12);
    tagEdit_ = new QLineEdit(tab);
    tagEdit_->setPlaceholderText("例：baseline / Gen5-x16 / 256GB-tiering");
    tagEdit_->setStyleSheet("QLineEdit { background:#0A0A0A; color:#EDEDED; border:1px solid #333; "
                            "border-radius:4px; padding:5px 10px; font-size:12px; }");
    tL->addWidget(tagEdit_);
    L->addWidget(tagGrp);

    // ── 状态 + 进度 ──
    statusLabel_ = new QLabel("", tab);
    statusLabel_->setStyleSheet("color:#888; font-size:11px; padding:5px 10px; "
        "background:#0A0A0A; border:1px solid #222; border-radius:4px;");
    statusLabel_->setVisible(false);
    L->addWidget(statusLabel_);

    progressBar_ = new QProgressBar(tab);
    progressBar_->setRange(0, 100); progressBar_->setValue(0);
    progressBar_->setTextVisible(true);
    progressBar_->setStyleSheet(
        "QProgressBar { background:#0A0A0A; border:1px solid #333; border-radius:4px; "
        "text-align:center; color:#EDEDED; height:18px; }"
        "QProgressBar::chunk { background:#4ADE80; border-radius:3px; }");
    progressBar_->setVisible(false);
    L->addWidget(progressBar_);
    L->addStretch();
}

// ─────────────────────────────────────────────────────────
//  Tab 2: 历史会话
// ─────────────────────────────────────────────────────────
void ExportDialog::setupHistoryTab(QWidget* tab) {
    auto* L = new QVBoxLayout(tab);
    L->setContentsMargins(14, 14, 14, 10);
    L->setSpacing(8);

    auto* hintLbl = new QLabel(
        "双击条目可查看详情并重新导出。会话在当次程序运行期间保留。", tab);
    hintLbl->setStyleSheet("color:#666; font-size:11px;");
    L->addWidget(hintLbl);

    historyList_ = new QListWidget(tab);
    historyList_->setStyleSheet(
        "QListWidget { background:#080808; color:#EDEDED; border:1px solid #2A2A2A; "
        "border-radius:4px; font-size:12px; }"
        "QListWidget::item { padding:8px 12px; border-bottom:1px solid #1A1A1A; }"
        "QListWidget::item:selected { background:#1E3A5F; color:#EDEDED; }"
        "QListWidget::item:hover { background:#151515; }");
    connect(historyList_, &QListWidget::itemDoubleClicked,
            this, &ExportDialog::onHistoryItemDoubleClicked);
    L->addWidget(historyList_, 1);

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();

    reExportBtn_ = new QPushButton("⬇ 重新导出", tab);
    reExportBtn_->setStyleSheet(makeBtn("#60A5FA", "#1E3A5F", "#0D1F3C"));
    reExportBtn_->setCursor(Qt::PointingHandCursor);
    connect(reExportBtn_, &QPushButton::clicked, this, &ExportDialog::onReExportHistory);
    btnRow->addWidget(reExportBtn_);

    deleteHistoryBtn_ = new QPushButton("🗑 删除记录", tab);
    deleteHistoryBtn_->setStyleSheet(makeBtn("#F87171", "#3A1A1A", "#2A1515"));
    deleteHistoryBtn_->setCursor(Qt::PointingHandCursor);
    connect(deleteHistoryBtn_, &QPushButton::clicked, this, &ExportDialog::onDeleteHistory);
    btnRow->addWidget(deleteHistoryBtn_);
    L->addLayout(btnRow);

    refreshHistoryList();
}

// ─────────────────────────────────────────────────────────
//  Tab 3: 实验备注
// ─────────────────────────────────────────────────────────
void ExportDialog::setupNoteTab(QWidget* tab) {
    auto* L = new QVBoxLayout(tab);
    L->setContentsMargins(14, 14, 14, 10);
    L->setSpacing(8);

    auto* hintLbl = new QLabel(
        "在此记录本次实验的目的、参数变化和初步结论，备注将随导出文件一起保存。", tab);
    hintLbl->setWordWrap(true);
    hintLbl->setStyleSheet("color:#888; font-size:11px;");
    L->addWidget(hintLbl);

    noteEdit_ = new QTextEdit(tab);
    noteEdit_->setPlaceholderText(
        "例：\n"
        "# 实验目的\n"
        "测试 PCIe Gen5 x16 对比 Gen4 x16 的延迟差异。\n\n"
        "# 配置变更\n"
        "- 将 CXL_MEM0 的 link_gen 从 Gen4 改为 Gen5\n"
        "- Epoch 时长保持 10ms\n\n"
        "# 初步结论\n"
        "待补充...");
    noteEdit_->setStyleSheet(
        "QTextEdit { background:#0A0A0A; color:#EDEDED; border:1px solid #2A2A2A; "
        "border-radius:4px; padding:8px; font-size:12px; font-family:monospace; }");
    L->addWidget(noteEdit_, 1);
}

// ─────────────────────────────────────────────────────────
//  Data setters
// ─────────────────────────────────────────────────────────
void ExportDialog::setEpochData(const std::vector<cxlsim::EpochStats>& data) {
    epochData_ = data;
    if (data.empty()) {
        summaryLabel_->setText("暂无数据");
        return;
    }
    // 计算摘要
    double sumLat = 0, sumBw = 0;
    for (const auto& s : data) {
        sumLat += s.avg_latency_ns;
        double bw = (s.total_accesses * 64.0) / (0.01 * 1e9);
        sumBw += bw;
    }
    double avgLat = sumLat / data.size();
    double avgBw  = sumBw  / data.size();
    summaryLabel_->setText(
        QString("%1 Epochs  |  avg lat %.1f ns  |  avg bw %.2f GB/s")
        .arg(data.size()).arg(avgLat).arg(avgBw));

    rangeEndSpin_->setValue(static_cast<int>(data.size()) - 1);
    rangeStartSpin_->setMaximum(static_cast<int>(data.size()) - 1);
    rangeEndSpin_->setMaximum(static_cast<int>(data.size()) - 1);
}

void ExportDialog::setConfigData(const QString& configJson) {
    configJson_ = configJson;
}

void ExportDialog::setSessionHistory(const QList<SessionRecord>& history) {
    history_ = history;
    refreshHistoryList();
}

// ─────────────────────────────────────────────────────────
//  getExportOptions
// ─────────────────────────────────────────────────────────
ExportDialog::ExportOptions ExportDialog::getExportOptions() const {
    ExportOptions opts;
    opts.exportCSV      = csvCheckbox_->isChecked();
    opts.exportJSON     = jsonCheckbox_->isChecked();
    opts.exportMarkdown = markdownCheckbox_->isChecked();
    if (allEpochsRadio_->isChecked())        opts.range = ALL_EPOCHS;
    else if (recentEpochsRadio_->isChecked()) opts.range = RECENT_EPOCHS;
    else                                      opts.range = EPOCH_RANGE;
    opts.recentCount  = recentCountSpin_->value();
    opts.rangeStart   = rangeStartSpin_->value();
    opts.rangeEnd     = rangeEndSpin_->value();
    opts.sessionTag   = tagEdit_->text().trimmed();
    opts.sessionNote  = noteEdit_ ? noteEdit_->toPlainText() : QString();
    return opts;
}

// ─────────────────────────────────────────────────────────
//  onRangeChanged
// ─────────────────────────────────────────────────────────
void ExportDialog::onRangeChanged() {
    recentCountSpin_->setEnabled(recentEpochsRadio_->isChecked());
    rangeStartSpin_->setEnabled(epochRangeRadio_->isChecked());
    rangeEndSpin_->setEnabled(epochRangeRadio_->isChecked());
}

// ─────────────────────────────────────────────────────────
//  onExport — 主导出逻辑
// ─────────────────────────────────────────────────────────
void ExportDialog::onExport() {
    auto opts = getExportOptions();
    if (!opts.exportCSV && !opts.exportJSON && !opts.exportMarkdown) {
        QMessageBox::warning(this, "请选择格式", "请至少勾选一项导出格式。");
        return;
    }

    exportBtn_->setEnabled(false);
    exportBtn_->setText("导出中...");
    statusLabel_->setText("⏳ 准备中...");
    statusLabel_->setVisible(true);
    progressBar_->setValue(0);
    progressBar_->setVisible(true);

    // 计算数据区间
    int startIdx = 0, endIdx = static_cast<int>(epochData_.size());
    if (opts.range == RECENT_EPOCHS) {
        startIdx = std::max(0, endIdx - opts.recentCount);
    } else if (opts.range == EPOCH_RANGE) {
        startIdx = std::max(0, opts.rangeStart);
        endIdx   = std::min(endIdx, opts.rangeEnd + 1);
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString tag       = opts.sessionTag.isEmpty() ? timestamp : opts.sessionTag;
    QStringList saved;

    SessionRecord rec;
    rec.id         = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    rec.timestamp  = QDateTime::currentDateTime();
    rec.tag        = tag;
    rec.note       = opts.sessionNote;
    rec.epochCount = endIdx - startIdx;

    // CSV
    if (opts.exportCSV && !epochData_.empty()) {
        statusLabel_->setText("📊 生成 CSV...");
        progressBar_->setValue(20);
        QString csv = generateCSV(epochData_, startIdx, endIdx);
        QString path = QFileDialog::getSaveFileName(
            this, "保存 CSV", QString("cxl_%1.csv").arg(tag), "CSV Files (*.csv)");
        if (!path.isEmpty() && saveTextFile(csv, path)) {
            saved << path;
            rec.csvPath = path;
        }
    }
    progressBar_->setValue(45);

    // JSON
    if (opts.exportJSON && !configJson_.isEmpty()) {
        statusLabel_->setText("⚙️ 生成 JSON...");
        QString path = QFileDialog::getSaveFileName(
            this, "保存配置快照", QString("cxl_%1.json").arg(tag), "JSON Files (*.json)");
        if (!path.isEmpty() && saveTextFile(configJson_, path)) {
            saved << path;
            rec.jsonPath = path;
        }
    }
    progressBar_->setValue(70);

    // Markdown
    if (opts.exportMarkdown && !epochData_.empty()) {
        statusLabel_->setText("📝 生成 Markdown 报告...");
        QString md   = generateMarkdown(epochData_, startIdx, endIdx);
        QString path = QFileDialog::getSaveFileName(
            this, "保存 Markdown 报告", QString("cxl_%1.md").arg(tag), "Markdown (*.md)");
        if (!path.isEmpty() && saveTextFile(md, path)) saved << path;
    }
    progressBar_->setValue(100);

    // 计算摘要指标写入 rec
    if (!epochData_.empty() && startIdx < endIdx) {
        double sumLat = 0, sumBw = 0;
        for (int i = startIdx; i < endIdx && i < (int)epochData_.size(); ++i) {
            sumLat += epochData_[i].avg_latency_ns;
            sumBw  += (epochData_[i].total_accesses * 64.0) / (0.01 * 1e9);
        }
        int n = endIdx - startIdx;
        rec.avgLatencyNs     = sumLat / n;
        rec.avgBandwidthGbps = sumBw  / n;
    }

    if (saved.isEmpty()) {
        statusLabel_->setText("⚠️ 未保存任何文件（可能已取消）");
        exportBtn_->setEnabled(true);
        exportBtn_->setText("⬇ 立即导出");
        return;
    }

    statusLabel_->setStyleSheet("color:#4ADE80; font-size:11px; padding:5px 10px; "
        "background:#0A0A0A; border:1px solid #166534; border-radius:4px;");
    statusLabel_->setText(QString("✓ 已保存 %1 个文件").arg(saved.size()));
    exportBtn_->setText("✓ 完成");

    // 加入历史
    history_.prepend(rec);
    refreshHistoryList();
    currentSession_ = rec;

    emit exportDone(rec);
    QTimer::singleShot(1500, this, [this]() { accept(); });
}

// ─────────────────────────────────────────────────────────
//  历史操作
// ─────────────────────────────────────────────────────────
void ExportDialog::refreshHistoryList() {
    if (!historyList_) return;
    historyList_->clear();
    for (int i = 0; i < history_.size(); ++i) {
        const auto& r = history_[i];
        QString line = QString("[%1]  %2  |  %3 epochs  |  avg %.1f ns  |  %4")
            .arg(r.timestamp.toString("MM-dd HH:mm"))
            .arg(r.tag.isEmpty() ? "(无标签)" : r.tag)
            .arg(r.epochCount)
            .arg(r.avgLatencyNs)
            .arg(r.note.isEmpty() ? "" : r.note.left(40) + "…");
        auto* item = new QListWidgetItem(line, historyList_);
        item->setData(Qt::UserRole, i);
        // 最新记录高亮
        if (i == 0) item->setForeground(QColor(0x4A, 0xDE, 0x80));
    }
    if (history_.isEmpty()) {
        historyList_->addItem("（暂无历史记录）");
        historyList_->item(0)->setForeground(QColor(0x44, 0x44, 0x44));
    }
}

void ExportDialog::onHistoryItemDoubleClicked(QListWidgetItem* item) {
    int idx = item->data(Qt::UserRole).toInt();
    if (idx < 0 || idx >= history_.size()) return;
    const auto& r = history_[idx];
    QString msg = QString("会话 ID: %1\n时间: %2\n标签: %3\n"
                          "Epochs: %4\n平均延迟: %.2f ns\n平均带宽: %.3f GB/s\n\n"
                          "CSV: %5\nJSON: %6\n\n备注:\n%7")
        .arg(r.id)
        .arg(r.timestamp.toString("yyyy-MM-dd HH:mm:ss"))
        .arg(r.tag.isEmpty() ? "(无)" : r.tag)
        .arg(r.epochCount)
        .arg(r.avgLatencyNs)
        .arg(r.avgBandwidthGbps)
        .arg(r.csvPath.isEmpty()  ? "(未导出)" : r.csvPath)
        .arg(r.jsonPath.isEmpty() ? "(未导出)" : r.jsonPath)
        .arg(r.note.isEmpty() ? "(无)" : r.note);
    QMessageBox::information(this, "历史会话详情", msg);
}

void ExportDialog::onDeleteHistory() {
    auto* item = historyList_->currentItem();
    if (!item) return;
    int idx = item->data(Qt::UserRole).toInt();
    if (idx < 0 || idx >= history_.size()) return;
    if (QMessageBox::question(this, "删除记录",
            "确认删除该历史记录？（不影响已保存的文件）",
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        history_.removeAt(idx);
        refreshHistoryList();
    }
}

void ExportDialog::onReExportHistory() {
    auto* item = historyList_->currentItem();
    if (!item) { QMessageBox::information(this, "提示", "请先选中一条历史记录"); return; }
    QMessageBox::information(this, "重新导出",
        "请切换到「导出当前会话」Tab，设置标签后重新导出。\n"
        "（历史会话的原始数据在内存中，当前仅支持在本次运行内重新导出）");
}

// ─────────────────────────────────────────────────────────
//  生成 CSV
// ─────────────────────────────────────────────────────────
QString ExportDialog::generateCSV(const std::vector<cxlsim::EpochStats>& data,
                                   int start, int end) const {
    QString csv;
    QTextStream s(&csv);
    s << "epoch_id,total_accesses,l3_misses,l3_miss_rate_pct,"
      << "local_dram_accesses,cxl_accesses,tiering_ratio_pct,"
      << "avg_latency_ns,p95_latency_ns,p99_latency_ns,queuing_delay_ns,"
      << "local_dram_latency_ns,link_utilization_pct,bandwidth_gbps\n";

    for (int i = start; i < end && i < (int)data.size(); ++i) {
        const auto& st = data[i];
        double l3Rate = st.total_accesses > 0
            ? 100.0 * st.l3_misses / st.total_accesses : 0.0;
        double bw = (st.cxl_accesses * 64.0) / (0.01 * 1e9);
        s << st.epoch_number          << ","
          << st.total_accesses        << ","
          << st.l3_misses             << ","
          << QString::number(l3Rate,                   'f', 2) << ","
          << st.local_dram_accesses   << ","
          << st.cxl_accesses          << ","
          << QString::number(st.tiering_ratio,          'f', 2) << ","
          << QString::number(st.avg_latency_ns,         'f', 2) << ","
          << QString::number(st.p95_latency_ns,         'f', 2) << ","
          << QString::number(st.p99_latency_ns,         'f', 2) << ","
          << QString::number(st.queuing_delay_ns,       'f', 2) << ","
          << QString::number(st.local_dram_latency_ns,  'f', 1) << ","
          << QString::number(st.link_utilization_pct,   'f', 2) << ","
          << QString::number(bw,                        'f', 3) << "\n";
    }
    return csv;
}

// ─────────────────────────────────────────────────────────
//  生成 Markdown 报告
// ─────────────────────────────────────────────────────────
QString ExportDialog::generateMarkdown(const std::vector<cxlsim::EpochStats>& data,
                                        int start, int end) const {
    QString md;
    QTextStream s(&md);

    int n = std::min(end, (int)data.size()) - start;
    if (n <= 0) { s << "# 无数据\n"; return md; }

    double sl=0, sp95=0, sp99=0, sb=0, st2=0, su=0, sq=0;
    double mxP99=0;
    uint64_t ta=0, ca=0, la=0;
    for (int i = start; i < start + n; ++i) {
        const auto& e = data[i];
        sl   += e.avg_latency_ns;
        sp95 += e.p95_latency_ns;
        sp99 += e.p99_latency_ns;
        sb   += (e.cxl_accesses * 64.0) / (0.01 * 1e9);
        st2  += e.tiering_ratio;
        su   += e.link_utilization_pct;
        sq   += e.queuing_delay_ns;
        ta   += e.total_accesses;
        ca   += e.cxl_accesses;
        la   += e.local_dram_accesses;
        mxP99 = std::max(mxP99, e.p99_latency_ns);
    }

    s << "# CXLMemSim 实验报告\n\n";
    s << "**导出时间：** " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "  \n";
    s << "**数据区间：** Epoch " << start << " – " << (start + n - 1)
      << "（共 " << n << " 个）\n\n";

    s << "## 关键指标摘要\n\n| 指标 | 均值 |\n|---|---|\n";
    s << "| 平均 CXL 访问延迟 | " << QString::number(sl/n,   'f', 2) << " ns |\n";
    s << "| 平均 P95 延迟     | " << QString::number(sp95/n, 'f', 2) << " ns |\n";
    s << "| 平均 P99 延迟     | " << QString::number(sp99/n, 'f', 2) << " ns |\n";
    s << "| P99 延迟峰值      | " << QString::number(mxP99,  'f', 2) << " ns |\n";
    s << "| 排队延迟          | " << QString::number(sq/n,   'f', 2) << " ns |\n";
    s << "| 平均带宽          | " << QString::number(sb/n,   'f', 3) << " GB/s |\n";
    s << "| 链路利用率        | " << QString::number(su/n,   'f', 1) << "% |\n";
    s << "| CXL Tiering Ratio | " << QString::number(st2/n,  'f', 1) << "% |\n";
    if (!data.empty() && start < (int)data.size())
        s << "| 本地 DRAM 参考延迟 | "
          << QString::number(data[start].local_dram_latency_ns, 'f', 1) << " ns |\n";
    s << "| 累计总访问量      | " << ta << " |\n";
    s << "| 累计 CXL 访问量   | " << ca << " |\n";
    s << "| 累计本地 DRAM 访问 | " << la << " |\n\n";

    s << "## 各 Epoch 详情（前 20 条）\n\n";
    s << "| Epoch | 总访问 | CXL访问 | 本地DRAM | 均延迟(ns) | P95(ns) | P99(ns) | 排队(ns) | Tiering% | 利用率% | 带宽(GB/s) |\n";
    s << "|---|---|---|---|---|---|---|---|---|---|---|\n";
    int limit = std::min(n, 20);
    for (int i = start; i < start + limit; ++i) {
        const auto& st = data[i];
        double bw = (st.cxl_accesses * 64.0) / (0.01 * 1e9);
        s << "| " << st.epoch_number
          << " | " << st.total_accesses
          << " | " << st.cxl_accesses
          << " | " << st.local_dram_accesses
          << " | " << QString::number(st.avg_latency_ns,       'f', 1)
          << " | " << QString::number(st.p95_latency_ns,       'f', 1)
          << " | " << QString::number(st.p99_latency_ns,       'f', 1)
          << " | " << QString::number(st.queuing_delay_ns,     'f', 1)
          << " | " << QString::number(st.tiering_ratio,        'f', 1)
          << " | " << QString::number(st.link_utilization_pct, 'f', 1)
          << " | " << QString::number(bw,                      'f', 2) << " |\n";
    }
    if (n > 20) s << "\n*（仅显示前 20 条，完整数据见 CSV 文件）*\n";
    s << "\n---\n*本报告由 CXLMemSim 自动生成*\n";
    return md;
}

// ─────────────────────────────────────────────────────────
//  saveTextFile
// ─────────────────────────────────────────────────────────
bool ExportDialog::saveTextFile(const QString& content, const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&f);
    out << content;
    f.close();
    return true;
}
