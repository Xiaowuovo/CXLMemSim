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

ExportDialog::ExportDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("导出报告 - Export Report");
    setMinimumSize(500, 400);
    setupUI();
}

void ExportDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);

    // 标题
    auto* titleLabel = new QLabel("导出 CXL 模拟数据", this);
    titleLabel->setStyleSheet(
        "font-size: 16px; font-weight: 700; color: #EDEDED; "
        "padding: 8px 0; border-bottom: 2px solid #333333;");
    mainLayout->addWidget(titleLabel);

    // ══════════════════════════════════════════════════════════
    // 模块一：导出范围选择
    // ══════════════════════════════════════════════════════════
    auto* rangeGroup = new QGroupBox("导出范围 (Range Selection)", this);
    rangeGroup->setStyleSheet(
        "QGroupBox { color: #AAAAAA; font-size: 12px; font-weight: bold; "
        "border: 1px solid #333333; border-radius: 6px; padding-top: 16px; "
        "background: #0A0A0A; margin-top: 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; }");
    
    auto* rangeLayout = new QVBoxLayout(rangeGroup);
    rangeLayout->setContentsMargins(16, 20, 16, 16);
    rangeLayout->setSpacing(10);

    allEpochsRadio_ = new QRadioButton("全部记录 (All Epochs)", this);
    allEpochsRadio_->setChecked(true);
    allEpochsRadio_->setStyleSheet("QRadioButton { color: #EDEDED; font-size: 12px; }");
    rangeLayout->addWidget(allEpochsRadio_);

    auto* recentLayout = new QHBoxLayout();
    recentEpochsRadio_ = new QRadioButton("阶段记录 (最后", this);
    recentEpochsRadio_->setStyleSheet("QRadioButton { color: #EDEDED; font-size: 12px; }");
    recentLayout->addWidget(recentEpochsRadio_);

    recentCountSpin_ = new QSpinBox(this);
    recentCountSpin_->setRange(100, 100000);
    recentCountSpin_->setValue(10000);
    recentCountSpin_->setSingleStep(1000);
    recentCountSpin_->setEnabled(false);
    recentCountSpin_->setStyleSheet(
        "QSpinBox { background: #0A0A0A; color: #EDEDED; border: 1px solid #333333; "
        "border-radius: 4px; padding: 4px 8px; font-size: 12px; }");
    recentLayout->addWidget(recentCountSpin_);

    auto* epochsLabel = new QLabel("Epochs)", this);
    epochsLabel->setStyleSheet("color: #EDEDED; font-size: 12px;");
    recentLayout->addWidget(epochsLabel);
    recentLayout->addStretch();
    rangeLayout->addLayout(recentLayout);

    connect(allEpochsRadio_, &QRadioButton::toggled, this, &ExportDialog::onRangeChanged);
    connect(recentEpochsRadio_, &QRadioButton::toggled, this, &ExportDialog::onRangeChanged);

    mainLayout->addWidget(rangeGroup);

    // ══════════════════════════════════════════════════════════
    // 模块二：导出内容勾选
    // ══════════════════════════════════════════════════════════
    auto* contentGroup = new QGroupBox("导出内容 (Content Selection)", this);
    contentGroup->setStyleSheet(
        "QGroupBox { color: #AAAAAA; font-size: 12px; font-weight: bold; "
        "border: 1px solid #333333; border-radius: 6px; padding-top: 16px; "
        "background: #0A0A0A; margin-top: 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; }");
    
    auto* contentLayout = new QVBoxLayout(contentGroup);
    contentLayout->setContentsMargins(16, 20, 16, 16);
    contentLayout->setSpacing(10);

    csvCheckbox_ = new QCheckBox("运行数据 (.csv) - 包含完整性能指标时间序列", this);
    csvCheckbox_->setChecked(true);
    csvCheckbox_->setStyleSheet("QCheckBox { color: #4ADE80; font-size: 12px; font-weight: 600; }");
    contentLayout->addWidget(csvCheckbox_);

    jsonCheckbox_ = new QCheckBox("实验配置快照 (.json) - 拓扑/负载/基准设定", this);
    jsonCheckbox_->setStyleSheet("QCheckBox { color: #60A5FA; font-size: 12px; font-weight: 600; }");
    contentLayout->addWidget(jsonCheckbox_);

    pngCheckbox_ = new QCheckBox("性能趋势图 (.png) - 延迟/带宽/Miss Rate曲线", this);
    pngCheckbox_->setStyleSheet("QCheckBox { color: #FBBF24; font-size: 12px; font-weight: 600; }");
    contentLayout->addWidget(pngCheckbox_);

    mainLayout->addWidget(contentGroup);

    // ══════════════════════════════════════════════════════════
    // 状态显示
    // ══════════════════════════════════════════════════════════
    statusLabel_ = new QLabel("", this);
    statusLabel_->setStyleSheet(
        "color: #888888; font-size: 11px; padding: 6px 10px; "
        "background: #0A0A0A; border: 1px solid #222222; border-radius: 4px;");
    statusLabel_->setVisible(false);
    mainLayout->addWidget(statusLabel_);

    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    progressBar_->setTextVisible(true);
    progressBar_->setStyleSheet(
        "QProgressBar { background: #0A0A0A; border: 1px solid #333333; "
        "border-radius: 4px; text-align: center; color: #EDEDED; height: 20px; }"
        "QProgressBar::chunk { background: #4ADE80; border-radius: 3px; }");
    progressBar_->setVisible(false);
    mainLayout->addWidget(progressBar_);

    // ══════════════════════════════════════════════════════════
    // 模块三：操作区
    // ══════════════════════════════════════════════════════════
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);
    btnLayout->addStretch();

    cancelBtn_ = new QPushButton("取消", this);
    cancelBtn_->setMinimumHeight(36);
    cancelBtn_->setCursor(Qt::PointingHandCursor);
    cancelBtn_->setStyleSheet(
        "QPushButton { background: #0A0A0A; color: #888888; "
        "border: 1px solid #333333; border-radius: 6px; "
        "padding: 8px 24px; font-size: 13px; }"
        "QPushButton:hover { background: #1A1A1A; color: #AAAAAA; }");
    connect(cancelBtn_, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn_);

    exportBtn_ = new QPushButton("📦 打包导出", this);
    exportBtn_->setMinimumHeight(36);
    exportBtn_->setCursor(Qt::PointingHandCursor);
    exportBtn_->setStyleSheet(
        "QPushButton { background: #0A0A0A; color: #4ADE80; "
        "border: 1px solid #166534; border-radius: 6px; "
        "padding: 8px 24px; font-weight: 600; font-size: 13px; }"
        "QPushButton:hover { background: #0A3D2C; color: #86EFAC; border-color: #22C55E; }"
        "QPushButton:disabled { background: #0A0A0A; color: #444444; border-color: #222222; }");
    connect(exportBtn_, &QPushButton::clicked, this, &ExportDialog::onExport);
    btnLayout->addWidget(exportBtn_);

    mainLayout->addLayout(btnLayout);
    mainLayout->addStretch();
}

void ExportDialog::onRangeChanged() {
    recentCountSpin_->setEnabled(recentEpochsRadio_->isChecked());
}

void ExportDialog::setEpochData(const std::vector<cxlsim::EpochStats>& data) {
    epochData_ = data;
}

void ExportDialog::setConfigData(const QString& configJson) {
    configJson_ = configJson;
}

ExportDialog::ExportOptions ExportDialog::getExportOptions() const {
    ExportOptions opts;
    opts.exportCSV = csvCheckbox_->isChecked();
    opts.exportJSON = jsonCheckbox_->isChecked();
    opts.exportPNG = pngCheckbox_->isChecked();
    opts.range = allEpochsRadio_->isChecked() ? ALL_EPOCHS : RECENT_EPOCHS;
    opts.recentCount = recentCountSpin_->value();
    return opts;
}

void ExportDialog::onExport() {
    auto opts = getExportOptions();
    
    if (!opts.exportCSV && !opts.exportJSON && !opts.exportPNG) {
        QMessageBox::warning(this, "警告", "请至少选择一项导出内容");
        return;
    }

    // 显示进度
    exportBtn_->setEnabled(false);
    exportBtn_->setText("导出中...");
    statusLabel_->setText("⏳ 正在准备导出数据...");
    statusLabel_->setVisible(true);
    progressBar_->setValue(0);
    progressBar_->setVisible(true);

    QStringList exportedFiles;
    int totalSteps = (opts.exportCSV ? 1 : 0) + (opts.exportJSON ? 1 : 0) + (opts.exportPNG ? 1 : 0);
    int currentStep = 0;

    // 导出CSV
    if (opts.exportCSV && !epochData_.empty()) {
        statusLabel_->setText("📊 正在生成 CSV 数据...");
        progressBar_->setValue(20);
        
        int startIdx = 0;
        if (opts.range == RECENT_EPOCHS) {
            startIdx = std::max(0, static_cast<int>(epochData_.size()) - opts.recentCount);
        }
        
        QString csv = generateCSV(epochData_, startIdx);
        
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        QString defaultName = QString("cxl_simulation_%1.csv").arg(timestamp);
        
        QString fileName = QFileDialog::getSaveFileName(
            this, "保存 CSV 数据", defaultName, "CSV Files (*.csv)");
        
        if (!fileName.isEmpty()) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out.setCodec("UTF-8");
                out << csv;
                file.close();
                exportedFiles << fileName;
            }
        }
        
        currentStep++;
        progressBar_->setValue(20 + (currentStep * 60 / totalSteps));
    }

    // 导出JSON
    if (opts.exportJSON && !configJson_.isEmpty()) {
        statusLabel_->setText("⚙️ 正在保存配置快照...");
        
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        QString defaultName = QString("cxl_config_%1.json").arg(timestamp);
        
        QString fileName = QFileDialog::getSaveFileName(
            this, "保存配置快照", defaultName, "JSON Files (*.json)");
        
        if (!fileName.isEmpty()) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << configJson_;
                file.close();
                exportedFiles << fileName;
            }
        }
        
        currentStep++;
        progressBar_->setValue(20 + (currentStep * 60 / totalSteps));
    }

    // 导出PNG (暂时跳过，需要从图表widget获取)
    if (opts.exportPNG) {
        statusLabel_->setText("📈 图表导出功能将在后续版本支持...");
        currentStep++;
        progressBar_->setValue(20 + (currentStep * 60 / totalSteps));
    }

    // 完成
    progressBar_->setValue(100);
    statusLabel_->setText(QString("✓ 导出完成！已保存 %1 个文件").arg(exportedFiles.size()));
    statusLabel_->setStyleSheet(
        "color: #4ADE80; font-size: 11px; padding: 6px 10px; "
        "background: #0A0A0A; border: 1px solid #166534; border-radius: 4px;");
    
    exportBtn_->setText("✓ 导出成功");
    
    QTimer::singleShot(2000, this, [this]() {
        accept();
    });
}

QString ExportDialog::generateCSV(const std::vector<cxlsim::EpochStats>& data, int startIdx) const {
    QString csv;
    QTextStream stream(&csv);

    // CSV表头（完整数据字典）
    stream << "epoch_id,total_accesses,l3_miss_rate,ratio_local,ratio_cxl,"
           << "avg_latency_ns,p95_latency_ns,p99_latency_ns,queuing_delay_ns,"
           << "bandwidth_gbps,link_utilization_pct\n";

    // 数据行
    for (size_t i = startIdx; i < data.size(); ++i) {
        const auto& stats = data[i];
        
        // 计算L3 Miss Rate
        double l3MissRate = 0.0;
        if (stats.total_accesses > 0) {
            l3MissRate = (static_cast<double>(stats.l3_misses) / stats.total_accesses) * 100.0;
        }
        
        // 计算带宽
        double bandwidth = 0.0;
        if (stats.total_accesses > 0) {
            bandwidth = (stats.total_accesses * 64.0) / 0.01 / 1e9; // GB/s
        }

        stream << stats.epoch_number << ","
               << stats.total_accesses << ","
               << QString::number(l3MissRate, 'f', 2) << ","
               << stats.local_dram_accesses << ","
               << stats.cxl_accesses << ","
               << QString::number(stats.avg_latency_ns, 'f', 2) << ","
               << QString::number(stats.p95_latency_ns, 'f', 2) << ","
               << QString::number(stats.p99_latency_ns, 'f', 2) << ","
               << QString::number(stats.queuing_delay_ns, 'f', 2) << ","
               << QString::number(bandwidth, 'f', 3) << ","
               << QString::number(stats.link_utilization_pct, 'f', 2) << "\n";
    }

    return csv;
}
