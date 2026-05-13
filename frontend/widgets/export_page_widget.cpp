#include "export_page_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QUuid>
#include <QScrollArea>
#include <cmath>
#include <algorithm>

// ─────────────────────────────────────────────────────────
//  Style constants (shared with export_dialog)
// ─────────────────────────────────────────────────────────
static const char* PG_GRP =
    "QGroupBox { color:#AAAAAA; font-size:11px; font-weight:700; letter-spacing:1px; "
    "border:1px solid #1E1E1E; border-radius:6px; padding-top:18px; "
    "background:#080808; margin-top:6px; }"
    "QGroupBox::title { subcontrol-origin:margin; left:12px; padding:0 4px; }";

static const char* PG_RADIO = "QRadioButton { color:#CCCCCC; font-size:12px; }";
static const char* PG_SPIN  =
    "QSpinBox { background:#0A0A0A; color:#EDEDED; border:1px solid #2A2A2A; "
    "border-radius:4px; padding:3px 8px; font-size:12px; }"
    "QSpinBox:disabled { color:#444; }";

static const char* PG_CHK_GREEN =
    "QCheckBox { color:#4ADE80; font-size:12px; font-weight:600; }"
    "QCheckBox::indicator { width:13px; height:13px; border:1px solid #166534; "
    "border-radius:3px; background:#0A0A0A; }"
    "QCheckBox::indicator:checked { background:#166534; }";
static const char* PG_CHK_BLUE =
    "QCheckBox { color:#60A5FA; font-size:12px; font-weight:600; }"
    "QCheckBox::indicator { width:13px; height:13px; border:1px solid #1E3A5F; "
    "border-radius:3px; background:#0A0A0A; }"
    "QCheckBox::indicator:checked { background:#1E3A5F; }";
static const char* PG_CHK_AMBER =
    "QCheckBox { color:#FBBF24; font-size:12px; font-weight:600; }"
    "QCheckBox::indicator { width:13px; height:13px; border:1px solid #78350F; "
    "border-radius:3px; background:#0A0A0A; }"
    "QCheckBox::indicator:checked { background:#78350F; }";

static QString pgBtn(const QString& fg, const QString& bd, const QString& hov) {
    return QString(
        "QPushButton { background:#0A0A0A; color:%1; border:1px solid %2; "
        "border-radius:6px; padding:8px 22px; font-size:13px; font-weight:600; }"
        "QPushButton:hover { background:%3; }"
        "QPushButton:disabled { color:#444; border-color:#1A1A1A; }").arg(fg, bd, hov);
}

// ─────────────────────────────────────────────────────────

ExportPageWidget::ExportPageWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void ExportPageWidget::setupUI() {
    setStyleSheet(
        "QWidget { background:#050505; }"
        "QTabWidget::pane { border:1px solid #1E1E1E; background:#080808; }"
        "QTabBar::tab { background:#0D0D0D; color:#666; padding:7px 18px; "
        "  border:1px solid #1E1E1E; border-bottom:none; border-radius:4px 4px 0 0; font-size:12px; }"
        "QTabBar::tab:selected { background:#080808; color:#EDEDED; border-bottom:1px solid #080808; }"
        "QTabBar::tab:hover:!selected { background:#141414; color:#AAAAAA; }"
        "QLabel { color:#CCCCCC; }");

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 16, 20, 16);
    root->setSpacing(12);

    // ── 页面标题行 ────────────────────────────────────────
    auto* hdr = new QHBoxLayout();
    auto* titleLbl = new QLabel("导出实验数据", this);
    titleLbl->setStyleSheet("font-size:16px; font-weight:700; color:#EDEDED;");
    hdr->addWidget(titleLbl);
    hdr->addStretch();
    summaryLabel_ = new QLabel("暂无数据", this);
    summaryLabel_->setStyleSheet(
        "font-size:11px; color:#666; padding:4px 12px; "
        "background:#111; border:1px solid #1E1E1E; border-radius:4px;");
    hdr->addWidget(summaryLabel_);
    root->addLayout(hdr);

    // ── Tab 容器 ─────────────────────────────────────────
    auto* tabs = new QTabWidget(this);
    root->addWidget(tabs, 1);

    auto* exportTab  = new QWidget(this);
    auto* historyTab = new QWidget(this);
    auto* noteTab    = new QWidget(this);
    tabs->addTab(exportTab,  "⬇  导出当前会话");
    tabs->addTab(historyTab, "📋  历史会话");
    tabs->addTab(noteTab,    "📝  实验备注");

    setupExportTab(exportTab);
    setupHistoryTab(historyTab);
    setupNoteTab(noteTab);

    // ── 底部操作行 ────────────────────────────────────────
    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    exportBtn_ = new QPushButton("⬇  立即导出", this);
    exportBtn_->setStyleSheet(pgBtn("#4ADE80", "#166534", "#0D3D26"));
    exportBtn_->setMinimumHeight(36);
    exportBtn_->setMinimumWidth(130);
    exportBtn_->setCursor(Qt::PointingHandCursor);
    connect(exportBtn_, &QPushButton::clicked, this, &ExportPageWidget::onExport);
    btnRow->addWidget(exportBtn_);
    root->addLayout(btnRow);
}

// ─────────────────────────────────────────────────────────
void ExportPageWidget::setupExportTab(QWidget* tab) {
    auto* sv = new QScrollArea(tab);
    sv->setWidgetResizable(true);
    sv->setFrameShape(QFrame::NoFrame);
    sv->setStyleSheet("QScrollArea { background:transparent; border:none; }");
    auto* tabL = new QVBoxLayout(tab);
    tabL->setContentsMargins(0, 0, 0, 0);
    tabL->addWidget(sv);

    auto* inner = new QWidget();
    inner->setStyleSheet("background:transparent;");
    sv->setWidget(inner);
    auto* L = new QVBoxLayout(inner);
    L->setContentsMargins(14, 12, 14, 12);
    L->setSpacing(10);

    // 范围
    auto* rgrp = new QGroupBox("导出范围", inner);
    rgrp->setStyleSheet(PG_GRP);
    auto* rL = new QVBoxLayout(rgrp);
    rL->setContentsMargins(14, 18, 14, 12); rL->setSpacing(8);

    allEpochsRadio_ = new QRadioButton("全部记录 (All Epochs)", inner);
    allEpochsRadio_->setStyleSheet(PG_RADIO);
    allEpochsRadio_->setChecked(true);
    rL->addWidget(allEpochsRadio_);

    auto* recRow = new QHBoxLayout();
    recentEpochsRadio_ = new QRadioButton("最近 N 个 Epoch：", inner);
    recentEpochsRadio_->setStyleSheet(PG_RADIO);
    recRow->addWidget(recentEpochsRadio_);
    recentCountSpin_ = new QSpinBox(inner);
    recentCountSpin_->setRange(100, 500000); recentCountSpin_->setValue(10000);
    recentCountSpin_->setSingleStep(1000); recentCountSpin_->setEnabled(false);
    recentCountSpin_->setFixedWidth(90); recentCountSpin_->setStyleSheet(PG_SPIN);
    recRow->addWidget(recentCountSpin_);
    recRow->addWidget(new QLabel("epochs", inner)); recRow->addStretch();
    rL->addLayout(recRow);

    auto* rangeRow = new QHBoxLayout();
    epochRangeRadio_ = new QRadioButton("指定区间 Epoch", inner);
    epochRangeRadio_->setStyleSheet(PG_RADIO);
    rangeRow->addWidget(epochRangeRadio_);
    rangeStartSpin_ = new QSpinBox(inner);
    rangeStartSpin_->setRange(0, 999999); rangeStartSpin_->setValue(0);
    rangeStartSpin_->setEnabled(false); rangeStartSpin_->setFixedWidth(80);
    rangeStartSpin_->setStyleSheet(PG_SPIN);
    rangeRow->addWidget(rangeStartSpin_);
    rangeRow->addWidget(new QLabel("→", inner));
    rangeEndSpin_ = new QSpinBox(inner);
    rangeEndSpin_->setRange(0, 999999); rangeEndSpin_->setValue(999999);
    rangeEndSpin_->setEnabled(false); rangeEndSpin_->setFixedWidth(80);
    rangeEndSpin_->setStyleSheet(PG_SPIN);
    rangeRow->addWidget(rangeEndSpin_); rangeRow->addStretch();
    rL->addLayout(rangeRow);

    connect(allEpochsRadio_,    &QRadioButton::toggled, this, &ExportPageWidget::onRangeChanged);
    connect(recentEpochsRadio_, &QRadioButton::toggled, this, &ExportPageWidget::onRangeChanged);
    connect(epochRangeRadio_,   &QRadioButton::toggled, this, &ExportPageWidget::onRangeChanged);
    L->addWidget(rgrp);

    // 格式
    auto* fgrp = new QGroupBox("导出格式", inner);
    fgrp->setStyleSheet(PG_GRP);
    auto* fL = new QVBoxLayout(fgrp);
    fL->setContentsMargins(14, 18, 14, 12); fL->setSpacing(8);

    csvCheckbox_ = new QCheckBox(
        "性能数据时间序列 (.csv)  — 含完整数据字典，可直接导入 Excel / Python / R", inner);
    csvCheckbox_->setChecked(true);
    csvCheckbox_->setStyleSheet(PG_CHK_GREEN);
    fL->addWidget(csvCheckbox_);

    jsonCheckbox_ = new QCheckBox(
        "实验配置快照 (.json)  — 拓扑、负载、模拟参数完整记录", inner);
    jsonCheckbox_->setStyleSheet(PG_CHK_BLUE);
    fL->addWidget(jsonCheckbox_);

    markdownCheckbox_ = new QCheckBox(
        "Markdown 实验报告 (.md)  — 含摘要表格 + 前20 Epoch 详情，适合论文附录", inner);
    markdownCheckbox_->setStyleSheet(PG_CHK_AMBER);
    fL->addWidget(markdownCheckbox_);
    L->addWidget(fgrp);

    // 标签
    auto* tgrp = new QGroupBox("会话标签（用于历史检索）", inner);
    tgrp->setStyleSheet(PG_GRP);
    auto* tL = new QHBoxLayout(tgrp);
    tL->setContentsMargins(14, 18, 14, 12);
    tagEdit_ = new QLineEdit(inner);
    tagEdit_->setPlaceholderText("例：baseline / Gen5-x16 / 256GB-tiering");
    tagEdit_->setStyleSheet(
        "QLineEdit { background:#0A0A0A; color:#EDEDED; border:1px solid #2A2A2A; "
        "border-radius:4px; padding:5px 10px; font-size:12px; }");
    tL->addWidget(tagEdit_);
    L->addWidget(tgrp);

    // 状态 + 进度
    statusLabel_ = new QLabel("", inner);
    statusLabel_->setStyleSheet(
        "color:#888; font-size:11px; padding:5px 10px; "
        "background:#0A0A0A; border:1px solid #1E1E1E; border-radius:4px;");
    statusLabel_->setVisible(false);
    L->addWidget(statusLabel_);

    progressBar_ = new QProgressBar(inner);
    progressBar_->setRange(0, 100); progressBar_->setValue(0);
    progressBar_->setTextVisible(true);
    progressBar_->setStyleSheet(
        "QProgressBar { background:#0A0A0A; border:1px solid #2A2A2A; border-radius:4px; "
        "text-align:center; color:#EDEDED; height:16px; }"
        "QProgressBar::chunk { background:#4ADE80; border-radius:3px; }");
    progressBar_->setVisible(false);
    L->addWidget(progressBar_);
    L->addStretch();
}

// ─────────────────────────────────────────────────────────
void ExportPageWidget::setupHistoryTab(QWidget* tab) {
    auto* L = new QVBoxLayout(tab);
    L->setContentsMargins(14, 12, 14, 12);
    L->setSpacing(8);

    auto* hint = new QLabel(
        "双击条目查看会话详情。记录在当次程序运行期间保留，重启后清空。", tab);
    hint->setStyleSheet("color:#555; font-size:11px;");
    L->addWidget(hint);

    historyList_ = new QListWidget(tab);
    historyList_->setStyleSheet(
        "QListWidget { background:#080808; color:#EDEDED; border:1px solid #1E1E1E; "
        "border-radius:4px; font-size:12px; }"
        "QListWidget::item { padding:9px 14px; border-bottom:1px solid #141414; }"
        "QListWidget::item:selected { background:#1E3A5F; }"
        "QListWidget::item:hover { background:#111; }");
    connect(historyList_, &QListWidget::itemDoubleClicked,
            this, &ExportPageWidget::onHistoryItemDoubleClicked);
    L->addWidget(historyList_, 1);

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    deleteHistoryBtn_ = new QPushButton("删除记录", tab);
    deleteHistoryBtn_->setStyleSheet(pgBtn("#F87171", "#3A1A1A", "#2A1515"));
    deleteHistoryBtn_->setCursor(Qt::PointingHandCursor);
    connect(deleteHistoryBtn_, &QPushButton::clicked, this, &ExportPageWidget::onDeleteHistory);
    btnRow->addWidget(deleteHistoryBtn_);
    L->addLayout(btnRow);

    refreshHistoryList();
}

// ─────────────────────────────────────────────────────────
void ExportPageWidget::setupNoteTab(QWidget* tab) {
    auto* L = new QVBoxLayout(tab);
    L->setContentsMargins(14, 12, 14, 12);
    L->setSpacing(8);

    auto* hint = new QLabel(
        "记录本次实验目的、参数变更及初步结论。备注将随导出文件一同保存。", tab);
    hint->setWordWrap(true);
    hint->setStyleSheet("color:#888; font-size:11px;");
    L->addWidget(hint);

    noteEdit_ = new QTextEdit(tab);
    noteEdit_->setPlaceholderText(
        "# 实验目的\n"
        "测试 PCIe Gen5 x16 对比 Gen4 x16 的延迟差异。\n\n"
        "# 配置变更\n"
        "- CXL_MEM0 link_gen: Gen4 → Gen5\n\n"
        "# 初步结论\n"
        "待补充...");
    noteEdit_->setStyleSheet(
        "QTextEdit { background:#080808; color:#EDEDED; border:1px solid #1E1E1E; "
        "border-radius:4px; padding:8px; font-size:12px; font-family:monospace; }");
    L->addWidget(noteEdit_, 1);
}

// ─────────────────────────────────────────────────────────
//  Data setters
// ─────────────────────────────────────────────────────────
void ExportPageWidget::setEpochData(const std::vector<cxlsim::EpochStats>& data) {
    epochData_ = data;
    if (data.empty()) { summaryLabel_->setText("暂无数据"); return; }

    double sumLat = 0, sumBw = 0;
    for (const auto& s : data) {
        sumLat += s.avg_latency_ns;
        sumBw  += (s.total_accesses * 64.0) / (0.01 * 1e9);
    }
    int n = (int)data.size();
    summaryLabel_->setText(
        QString("%1 Epochs  |  avg lat %.1f ns  |  avg bw %.2f GB/s")
        .arg(n).arg(sumLat / n).arg(sumBw / n));

    rangeEndSpin_->setValue(n - 1);
    rangeStartSpin_->setMaximum(n - 1);
    rangeEndSpin_->setMaximum(n - 1);
}

void ExportPageWidget::setConfigData(const QString& j) { configJson_ = j; }

void ExportPageWidget::setSessionHistory(const QList<SessionRecord>& h) {
    history_ = h; refreshHistoryList();
}

// ─────────────────────────────────────────────────────────
ExportPageWidget::ExportOptions ExportPageWidget::getExportOptions() const {
    ExportOptions o;
    o.exportCSV      = csvCheckbox_->isChecked();
    o.exportJSON     = jsonCheckbox_->isChecked();
    o.exportMarkdown = markdownCheckbox_->isChecked();
    if (allEpochsRadio_->isChecked())         o.range = ExportRange::ALL_EPOCHS;
    else if (recentEpochsRadio_->isChecked()) o.range = ExportRange::RECENT_EPOCHS;
    else                                       o.range = ExportRange::EPOCH_RANGE;
    o.recentCount = recentCountSpin_->value();
    o.rangeStart  = rangeStartSpin_->value();
    o.rangeEnd    = rangeEndSpin_->value();
    o.sessionTag  = tagEdit_->text().trimmed();
    o.sessionNote = noteEdit_ ? noteEdit_->toPlainText() : QString();
    return o;
}

void ExportPageWidget::onRangeChanged() {
    recentCountSpin_->setEnabled(recentEpochsRadio_->isChecked());
    rangeStartSpin_->setEnabled(epochRangeRadio_->isChecked());
    rangeEndSpin_->setEnabled(epochRangeRadio_->isChecked());
}

// ─────────────────────────────────────────────────────────
void ExportPageWidget::onExport() {
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

    int endIdx = (int)epochData_.size(), startIdx = 0;
    if (opts.range == ExportRange::RECENT_EPOCHS)
        startIdx = std::max(0, endIdx - opts.recentCount);
    else if (opts.range == ExportRange::EPOCH_RANGE) {
        startIdx = std::max(0, opts.rangeStart);
        endIdx   = std::min(endIdx, opts.rangeEnd + 1);
    }

    QString ts  = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString tag = opts.sessionTag.isEmpty() ? ts : opts.sessionTag;
    QStringList saved;

    SessionRecord rec;
    rec.id         = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    rec.timestamp  = QDateTime::currentDateTime();
    rec.tag        = tag;
    rec.note       = opts.sessionNote;
    rec.epochCount = endIdx - startIdx;

    if (opts.exportCSV && !epochData_.empty()) {
        statusLabel_->setText("📊 生成 CSV...");
        progressBar_->setValue(20);
        QString path = QFileDialog::getSaveFileName(
            this, "保存 CSV", QString("cxl_%1.csv").arg(tag), "CSV (*.csv)");
        if (!path.isEmpty() && saveTextFile(generateCSV(epochData_, startIdx, endIdx), path)) {
            saved << path; rec.csvPath = path;
        }
    }
    progressBar_->setValue(50);

    if (opts.exportJSON && !configJson_.isEmpty()) {
        statusLabel_->setText("⚙️ 生成 JSON...");
        QString path = QFileDialog::getSaveFileName(
            this, "保存配置快照", QString("cxl_%1.json").arg(tag), "JSON (*.json)");
        if (!path.isEmpty() && saveTextFile(configJson_, path)) {
            saved << path; rec.jsonPath = path;
        }
    }
    progressBar_->setValue(75);

    if (opts.exportMarkdown && !epochData_.empty()) {
        statusLabel_->setText("📝 生成 Markdown 报告...");
        QString path = QFileDialog::getSaveFileName(
            this, "保存报告", QString("cxl_%1.md").arg(tag), "Markdown (*.md)");
        if (!path.isEmpty() && saveTextFile(generateMarkdown(epochData_, startIdx, endIdx), path))
            saved << path;
    }
    progressBar_->setValue(100);

    if (!epochData_.empty() && startIdx < endIdx) {
        double sl = 0, sb = 0;
        for (int i = startIdx; i < endIdx && i < (int)epochData_.size(); ++i) {
            sl += epochData_[i].avg_latency_ns;
            sb += (epochData_[i].total_accesses * 64.0) / (0.01 * 1e9);
        }
        int n = endIdx - startIdx;
        rec.avgLatencyNs     = sl / n;
        rec.avgBandwidthGbps = sb / n;
    }

    if (saved.isEmpty()) {
        statusLabel_->setText("⚠️ 未保存任何文件（可能已取消）");
        exportBtn_->setEnabled(true);
        exportBtn_->setText("⬇  立即导出");
        return;
    }

    statusLabel_->setStyleSheet(
        "color:#4ADE80; font-size:11px; padding:5px 10px; "
        "background:#0A0A0A; border:1px solid #166534; border-radius:4px;");
    statusLabel_->setText(QString("✓ 已保存 %1 个文件").arg(saved.size()));
    exportBtn_->setEnabled(true);
    exportBtn_->setText("⬇  立即导出");

    history_.prepend(rec);
    refreshHistoryList();
    emit exportDone(rec);
}

// ─────────────────────────────────────────────────────────
void ExportPageWidget::refreshHistoryList() {
    if (!historyList_) return;
    historyList_->clear();
    for (int i = 0; i < history_.size(); ++i) {
        const auto& r = history_[i];
        QString line = QString("[%1]  %2  |  %3 epochs  |  avg %.1f ns")
            .arg(r.timestamp.toString("MM-dd HH:mm"))
            .arg(r.tag.isEmpty() ? "(无标签)" : r.tag)
            .arg(r.epochCount)
            .arg(r.avgLatencyNs);
        if (!r.note.isEmpty()) line += "  |  " + r.note.left(30) + "…";
        auto* item = new QListWidgetItem(line, historyList_);
        item->setData(Qt::UserRole, i);
        if (i == 0) item->setForeground(QColor(0x4A, 0xDE, 0x80));
    }
    if (history_.isEmpty()) {
        auto* item = new QListWidgetItem("（暂无历史记录）", historyList_);
        item->setForeground(QColor(0x44, 0x44, 0x44));
    }
}

void ExportPageWidget::onHistoryItemDoubleClicked(QListWidgetItem* item) {
    int idx = item->data(Qt::UserRole).toInt();
    if (idx < 0 || idx >= history_.size()) return;
    const auto& r = history_[idx];
    QMessageBox::information(this, "历史会话详情",
        QString("ID: %1\n时间: %2\n标签: %3\nEpochs: %4\n"
                "平均延迟: %.2f ns\n平均带宽: %.3f GB/s\n\n"
                "CSV: %5\nJSON: %6\n\n备注:\n%7")
        .arg(r.id)
        .arg(r.timestamp.toString("yyyy-MM-dd HH:mm:ss"))
        .arg(r.tag.isEmpty() ? "(无)" : r.tag)
        .arg(r.epochCount)
        .arg(r.avgLatencyNs)
        .arg(r.avgBandwidthGbps)
        .arg(r.csvPath.isEmpty()  ? "(未导出)" : r.csvPath)
        .arg(r.jsonPath.isEmpty() ? "(未导出)" : r.jsonPath)
        .arg(r.note.isEmpty() ? "(无)" : r.note));
}

void ExportPageWidget::onDeleteHistory() {
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

// ─────────────────────────────────────────────────────────
QString ExportPageWidget::generateCSV(const std::vector<cxlsim::EpochStats>& data,
                                       int start, int end) const {
    QString csv;
    QTextStream s(&csv);
    s << "epoch_id,total_accesses,l3_misses,l3_miss_rate_pct,"
      << "local_dram_accesses,cxl_accesses,tiering_ratio_pct,"
      << "avg_latency_ns,p95_latency_ns,p99_latency_ns,queuing_delay_ns,"
      << "local_dram_latency_ns,link_utilization_pct,bandwidth_gbps\n";
    for (int i = start; i < end && i < (int)data.size(); ++i) {
        const auto& st = data[i];
        double l3r = st.total_accesses > 0 ? 100.0 * st.l3_misses / st.total_accesses : 0;
        double bw  = st.total_accesses > 0 ? (st.total_accesses * 64.0) / (0.01 * 1e9) : 0;
        s << st.epoch_number << "," << st.total_accesses << "," << st.l3_misses << ","
          << QString::number(l3r,                  'f', 2) << ","
          << st.local_dram_accesses << "," << st.cxl_accesses << ","
          << QString::number(st.tiering_ratio,      'f', 2) << ","
          << QString::number(st.avg_latency_ns,     'f', 2) << ","
          << QString::number(st.p95_latency_ns,     'f', 2) << ","
          << QString::number(st.p99_latency_ns,     'f', 2) << ","
          << QString::number(st.queuing_delay_ns,   'f', 2) << ","
          << QString::number(st.local_dram_latency_ns,'f', 1) << ","
          << QString::number(st.link_utilization_pct,'f', 2) << ","
          << QString::number(bw,                    'f', 3) << "\n";
    }
    return csv;
}

QString ExportPageWidget::generateMarkdown(const std::vector<cxlsim::EpochStats>& data,
                                            int start, int end) const {
    QString md;
    QTextStream s(&md);
    int n = std::min(end, (int)data.size()) - start;
    if (n <= 0) { s << "# 无数据\n"; return md; }

    double sl = 0, sb = 0, st2 = 0, mx = 0;
    for (int i = start; i < start + n; ++i) {
        sl  += data[i].avg_latency_ns;
        sb  += (data[i].total_accesses * 64.0) / (0.01 * 1e9);
        st2 += data[i].tiering_ratio;
        mx   = std::max(mx, data[i].p99_latency_ns);
    }
    s << "# CXLMemSim 实验报告\n\n";
    s << "**导出时间：** " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "  \n";
    s << "**数据区间：** Epoch " << start << " – " << (start+n-1) << "（共 " << n << " 个）\n\n";
    s << "## 关键指标摘要\n\n| 指标 | 值 |\n|---|---|\n";
    s << "| 平均访问延迟 | " << QString::number(sl/n,'f',2) << " ns |\n";
    s << "| P99 延迟峰值 | " << QString::number(mx,  'f',2) << " ns |\n";
    s << "| 平均带宽     | " << QString::number(sb/n,'f',3) << " GB/s |\n";
    s << "| CXL Tiering Ratio | " << QString::number(st2/n,'f',1) << "% |\n\n";
    s << "## Epoch 详情（前 20 条）\n\n";
    s << "| Epoch | 总访问 | CXL访问 | 均延迟(ns) | P99(ns) | Tiering% | 带宽(GB/s) |\n";
    s << "|---|---|---|---|---|---|---|\n";
    int lim = std::min(n, 20);
    for (int i = start; i < start+lim; ++i) {
        const auto& e = data[i];
        double bw = (e.total_accesses * 64.0) / (0.01 * 1e9);
        s << "| " << e.epoch_number << " | " << e.total_accesses
          << " | " << e.cxl_accesses
          << " | " << QString::number(e.avg_latency_ns,'f',1)
          << " | " << QString::number(e.p99_latency_ns,'f',1)
          << " | " << QString::number(e.tiering_ratio,'f',1)
          << " | " << QString::number(bw,'f',2) << " |\n";
    }
    if (n > 20) s << "\n*（仅显示前 20 条）*\n";
    s << "\n---\n*由 CXLMemSim 自动生成*\n";
    return md;
}

bool ExportPageWidget::saveTextFile(const QString& content, const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&f); out << content; f.close(); return true;
}
