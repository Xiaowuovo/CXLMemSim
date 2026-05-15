/**
 * @file workload_config_widget.cpp
 * @brief 负载配置面板实现
 */

#include "workload_config_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QButtonGroup>
#include <QFileDialog>
#include <QFileInfo>

WorkloadConfigWidget::WorkloadConfigWidget(QWidget *parent)
    : QWidget(parent)
    , syntheticModeBtn_(nullptr)
    , traceModeBtn_(nullptr)
    , traceGroup_(nullptr)
    , tracePathEdit_(nullptr)
    , browsBtn_(nullptr)
    , traceStatusLabel_(nullptr)
    , syntheticGroup_(nullptr)
    , patternCombo_(nullptr)
    , readRatioSpin_(nullptr)
    , injectionRateSpin_(nullptr)
    , workingSetSpin_(nullptr)
    , strideSpin_(nullptr)
    , durationSpin_(nullptr)
    , threadsSpin_(nullptr)
{
    setupUI();
    updateUIState();
}

WorkloadConfigWidget::~WorkloadConfigWidget() {}

void WorkloadConfigWidget::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(16);

    // ── 模式选择 ──
    auto* modeGroup = new QGroupBox("负载生成模式 (Workload Generation)", this);
    modeGroup->setStyleSheet(
        "QGroupBox { color: #888888; font-size: 11px; font-weight: bold; border: none; padding-top: 16px; }");
    
    auto* modeLayout = new QVBoxLayout(modeGroup);
    auto* btnGroup = new QButtonGroup(this);
    
    syntheticModeBtn_ = new QRadioButton("🔧 Synthetic Traffic (合成流量)", this);
    syntheticModeBtn_->setChecked(true);
    syntheticModeBtn_->setStyleSheet("color: #EDEDED; font-size: 12px;");
    btnGroup->addButton(syntheticModeBtn_);
    modeLayout->addWidget(syntheticModeBtn_);
    
    auto* synthDesc = new QLabel("  参数化配置访问模式、读写比例、注入速率", this);
    synthDesc->setStyleSheet("color: #666666; font-size: 10px; margin-left: 24px;");
    modeLayout->addWidget(synthDesc);
    
    modeLayout->addSpacing(8);
    
    traceModeBtn_ = new QRadioButton("📊 Trace-Driven (轨迹驱动)", this);
    traceModeBtn_->setStyleSheet("color: #EDEDED; font-size: 12px;");
    btnGroup->addButton(traceModeBtn_);
    modeLayout->addWidget(traceModeBtn_);
    
    auto* traceDesc = new QLabel("  加载真实应用内存访问轨迹 (YOLOv8/BERT/...)", this);
    traceDesc->setStyleSheet("color: #666666; font-size: 10px; margin-left: 24px;");
    modeLayout->addWidget(traceDesc);
    
    mainLayout->addWidget(modeGroup);

    // ── Trace-Driven 配置 ──
    traceGroup_ = new QGroupBox("Trace File Configuration", this);
    traceGroup_->setStyleSheet(
        "QGroupBox { color: #888888; font-size: 11px; font-weight: bold; border: 1px solid #333333; "
        "border-radius: 6px; padding: 12px; margin-top: 8px; background: #0A0A0A; }");
    
    auto* traceLayout = new QVBoxLayout(traceGroup_);
    
    auto* fileRow = new QHBoxLayout();
    auto* fileLabel = new QLabel("Trace File:", this);
    fileLabel->setStyleSheet("color: #888888; font-size: 11px;");
    fileRow->addWidget(fileLabel);
    
    tracePathEdit_ = new QLineEdit(this);
    tracePathEdit_->setPlaceholderText("选择 CSV/TXT 轨迹文件...");
    tracePathEdit_->setReadOnly(true);
    fileRow->addWidget(tracePathEdit_, 1);
    
    browsBtn_ = new QPushButton("浏览...", this);
    browsBtn_->setMinimumWidth(80);
    fileRow->addWidget(browsBtn_);
    traceLayout->addLayout(fileRow);
    
    traceStatusLabel_ = new QLabel("❌ 未加载轨迹文件", this);
    traceStatusLabel_->setStyleSheet("color: #EF4444; font-size: 10px; margin-top: 4px;");
    traceLayout->addWidget(traceStatusLabel_);
    
    mainLayout->addWidget(traceGroup_);

    // ── Synthetic Traffic 配置 ──
    syntheticGroup_ = new QGroupBox("Synthetic Traffic Parameters", this);
    syntheticGroup_->setStyleSheet(
        "QGroupBox { color: #888888; font-size: 11px; font-weight: bold; border: 1px solid #333333; "
        "border-radius: 6px; padding: 12px; margin-top: 8px; background: #0A0A0A; }");
    
    auto* synthLayout = new QFormLayout(syntheticGroup_);
    synthLayout->setHorizontalSpacing(12);
    synthLayout->setVerticalSpacing(10);
    synthLayout->setLabelAlignment(Qt::AlignRight);

    // Access Pattern
    patternCombo_ = new QComboBox(this);
    patternCombo_->addItem("🎲 Random (随机访问)", static_cast<int>(cxlsim::AccessPattern::RANDOM));
    patternCombo_->addItem("📏 Sequential (顺序访问)", static_cast<int>(cxlsim::AccessPattern::SEQUENTIAL));
    patternCombo_->addItem("🔀 Stride (跨步访问)", static_cast<int>(cxlsim::AccessPattern::STRIDE));
    patternCombo_->addItem("🌀 Mixed (混合模式)", static_cast<int>(cxlsim::AccessPattern::MIXED));
    synthLayout->addRow("Access Pattern:", patternCombo_);

    // Read Ratio
    readRatioSpin_ = new QDoubleSpinBox(this);
    readRatioSpin_->setRange(0.0, 1.0);
    readRatioSpin_->setSingleStep(0.1);
    readRatioSpin_->setValue(0.7);
    readRatioSpin_->setSuffix(" (70% Read)");
    synthLayout->addRow("Read/Write Ratio:", readRatioSpin_);

    // Injection Rate
    injectionRateSpin_ = new QDoubleSpinBox(this);
    injectionRateSpin_->setRange(0.1, 200.0);
    injectionRateSpin_->setSingleStep(1.0);
    injectionRateSpin_->setValue(10.0);
    injectionRateSpin_->setSuffix(" GB/s");
    injectionRateSpin_->setToolTip("流量注入速率，影响带宽利用率和拥塞");
    synthLayout->addRow("Injection Rate:", injectionRateSpin_);

    // Working Set
    workingSetSpin_ = new QSpinBox(this);
    workingSetSpin_->setRange(1, 1024);
    workingSetSpin_->setValue(32);
    workingSetSpin_->setSuffix(" GB");
    workingSetSpin_->setToolTip("工作集大小，影响Cache命中率");
    synthLayout->addRow("Working Set Size:", workingSetSpin_);

    // Stride
    strideSpin_ = new QSpinBox(this);
    strideSpin_->setRange(64, 65536);
    strideSpin_->setSingleStep(64);
    strideSpin_->setValue(4096);
    strideSpin_->setSuffix(" Bytes");
    strideSpin_->setEnabled(false); // 仅在Stride模式启用
    synthLayout->addRow("Stride Size:", strideSpin_);

    // Duration
    durationSpin_ = new QDoubleSpinBox(this);
    durationSpin_->setRange(0.1, 3600.0);
    durationSpin_->setSingleStep(1.0);
    durationSpin_->setValue(10.0);
    durationSpin_->setSuffix(" sec");
    synthLayout->addRow("Duration:", durationSpin_);

    // Threads
    threadsSpin_ = new QSpinBox(this);
    threadsSpin_->setRange(1, 128);
    threadsSpin_->setValue(1);
    threadsSpin_->setToolTip("并发线程数，模拟多核并发访问");
    synthLayout->addRow("Concurrent Threads:", threadsSpin_);

    mainLayout->addWidget(syntheticGroup_);
    mainLayout->addStretch();
    
    // ══════════════════════════════════════════════════════════
    // 操作按钮组
    // ══════════════════════════════════════════════════════════
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);
    
    auto* applyBtn = new QPushButton("✓ 应用负载配置", this);
    applyBtn->setMinimumHeight(40);
    applyBtn->setCursor(Qt::PointingHandCursor);
    applyBtn->setStyleSheet(
        "QPushButton { "
        "    background: #0A0A0A; "
        "    color: #6EE7B7; "
        "    border: 1px solid #166534; "
        "    border-radius: 6px; "
        "    padding: 10px 24px; "
        "    font-weight: 600; "
        "    font-size: 13px;"
        "}"
        "QPushButton:hover { "
        "    background: #0A3D2C; "
        "    color: #86EFAC; "
        "    border-color: #22C55E; "
        "}"
        "QPushButton:pressed { "
        "    background: #14532D; "
        "    border-color: #22C55E; "
        "}");
    connect(applyBtn, &QPushButton::clicked, this, &WorkloadConfigWidget::applyWorkloadRequested);
    btnLayout->addWidget(applyBtn);
    
    auto* cancelBtn = new QPushButton("✕ 取消负载", this);
    cancelBtn->setMinimumHeight(40);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setStyleSheet(
        "QPushButton { "
        "    background: #0A0A0A; "
        "    color: #FCA5A5; "
        "    border: 1px solid #7F1D1D; "
        "    border-radius: 6px; "
        "    padding: 10px 24px; "
        "    font-weight: 600; "
        "    font-size: 13px;"
        "}"
        "QPushButton:hover { "
        "    background: #3D0A0A; "
        "    color: #FEE2E2; "
        "    border-color: #DC2626; "
        "}"
        "QPushButton:pressed { "
        "    background: #532D14; "
        "    border-color: #DC2626; "
        "}");
    connect(cancelBtn, &QPushButton::clicked, this, &WorkloadConfigWidget::cancelWorkloadRequested);
    btnLayout->addWidget(cancelBtn);
    
    mainLayout->addLayout(btnLayout);

    // ── 信号连接 ──
    connect(syntheticModeBtn_, &QRadioButton::toggled, this, &WorkloadConfigWidget::onModeChanged);
    connect(traceModeBtn_, &QRadioButton::toggled, this, &WorkloadConfigWidget::onModeChanged);
    connect(browsBtn_, &QPushButton::clicked, this, &WorkloadConfigWidget::onBrowseTraceFile);
    
    connect(patternCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &WorkloadConfigWidget::onParameterChanged);
    connect(readRatioSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &WorkloadConfigWidget::onParameterChanged);
    connect(injectionRateSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &WorkloadConfigWidget::onParameterChanged);
    connect(workingSetSpin_, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &WorkloadConfigWidget::onParameterChanged);
    connect(strideSpin_, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &WorkloadConfigWidget::onParameterChanged);
    connect(durationSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &WorkloadConfigWidget::onParameterChanged);
    connect(threadsSpin_, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &WorkloadConfigWidget::onParameterChanged);
}

void WorkloadConfigWidget::updateUIState() {
    bool isSynthetic = syntheticModeBtn_->isChecked();
    
    // 强化互斥逻辑：禁用的组件添加半透明样式
    traceGroup_->setEnabled(!isSynthetic);
    syntheticGroup_->setEnabled(isSynthetic);
    
    if (isSynthetic) {
        // Synthetic模式：Trace组件置灰
        traceGroup_->setStyleSheet(
            "QGroupBox { color: #444444; font-size: 11px; font-weight: bold; "
            "border: 1px solid #222222; border-radius: 6px; padding: 12px; "
            "margin-top: 8px; background: #050505; opacity: 0.5; }"
            "QGroupBox * { color: #444444; }");
        syntheticGroup_->setStyleSheet(
            "QGroupBox { color: #888888; font-size: 11px; font-weight: bold; "
            "border: 1px solid #4FC3F7; border-radius: 6px; padding: 12px; "
            "margin-top: 8px; background: #0A0A0A; }");
        
        // Stride控件仅在Stride模式启用
        int pattern = patternCombo_->currentData().toInt();
        strideSpin_->setEnabled(pattern == static_cast<int>(cxlsim::AccessPattern::STRIDE));
    } else {
        // Trace模式：Synthetic组件置灰
        traceGroup_->setStyleSheet(
            "QGroupBox { color: #888888; font-size: 11px; font-weight: bold; "
            "border: 1px solid #4FC3F7; border-radius: 6px; padding: 12px; "
            "margin-top: 8px; background: #0A0A0A; }");
        syntheticGroup_->setStyleSheet(
            "QGroupBox { color: #444444; font-size: 11px; font-weight: bold; "
            "border: 1px solid #222222; border-radius: 6px; padding: 12px; "
            "margin-top: 8px; background: #050505; opacity: 0.5; }"
            "QGroupBox * { color: #444444; }");
    }
}

void WorkloadConfigWidget::onModeChanged() {
    updateUIState();
    validateAndEmit();
}

void WorkloadConfigWidget::onBrowseTraceFile() {
    QString filename = QFileDialog::getOpenFileName(
        this, "选择内存访问轨迹文件", "",
        "Trace Files (*.csv *.txt *.trace);;All Files (*)");
    
    if (!filename.isEmpty()) {
        tracePathEdit_->setText(filename);
        
        QFileInfo info(filename);
        if (info.exists() && info.isReadable()) {
            traceStatusLabel_->setText(QString("✅ 已加载: %1 (%2 KB)")
                .arg(info.fileName())
                .arg(info.size() / 1024));
            traceStatusLabel_->setStyleSheet("color: #10B981; font-size: 10px; margin-top: 4px;");
        } else {
            traceStatusLabel_->setText("❌ 文件不可读");
            traceStatusLabel_->setStyleSheet("color: #EF4444; font-size: 10px; margin-top: 4px;");
        }
        
        validateAndEmit();
    }
}

void WorkloadConfigWidget::onParameterChanged() {
    updateUIState();
    validateAndEmit();
}

void WorkloadConfigWidget::validateAndEmit() {
    bool valid = isWorkloadValid();
    emit validationStatusChanged(valid);
    emit workloadChanged();
}

cxlsim::WorkloadConfig WorkloadConfigWidget::getWorkloadConfig() const {
    cxlsim::WorkloadConfig config;
    
    config.trace_driven = traceModeBtn_->isChecked();
    
    if (config.trace_driven) {
        config.trace_file_path = tracePathEdit_->text().toStdString();
    } else {
        int pattern = patternCombo_->currentData().toInt();
        config.access_pattern = static_cast<cxlsim::AccessPattern>(pattern);
        config.read_ratio = readRatioSpin_->value();
        config.injection_rate_gbps = injectionRateSpin_->value();
        config.working_set_gb = workingSetSpin_->value();
        config.stride_bytes = strideSpin_->value();
        config.duration_sec = durationSpin_->value();
        config.num_threads = threadsSpin_->value();
    }
    
    return config;
}

void WorkloadConfigWidget::setWorkloadConfig(const cxlsim::WorkloadConfig& config) {
    if (config.trace_driven) {
        traceModeBtn_->setChecked(true);
        tracePathEdit_->setText(QString::fromStdString(config.trace_file_path));
    } else {
        syntheticModeBtn_->setChecked(true);
        // 按 userData 匹配，而非直接用 index（combo 顺序与枚举值不一致）
        int targetVal = static_cast<int>(config.access_pattern);
        for (int i = 0; i < patternCombo_->count(); ++i) {
            if (patternCombo_->itemData(i).toInt() == targetVal) {
                patternCombo_->setCurrentIndex(i);
                break;
            }
        }
        readRatioSpin_->setValue(config.read_ratio);
        injectionRateSpin_->setValue(config.injection_rate_gbps);
        workingSetSpin_->setValue(config.working_set_gb);
        strideSpin_->setValue(config.stride_bytes);
        durationSpin_->setValue(config.duration_sec);
        threadsSpin_->setValue(config.num_threads);
    }
    
    updateUIState();
}

bool WorkloadConfigWidget::isWorkloadValid() const {
    lastError_.clear();
    
    if (traceModeBtn_->isChecked()) {
        // Trace模式验证
        QString path = tracePathEdit_->text();
        if (path.isEmpty()) {
            lastError_ = "未选择轨迹文件";
            return false;
        }
        
        QFileInfo info(path);
        if (!info.exists()) {
            lastError_ = "轨迹文件不存在";
            return false;
        }
        
        if (!info.isReadable()) {
            lastError_ = "轨迹文件不可读";
            return false;
        }
    } else {
        // Synthetic模式验证
        if (injectionRateSpin_->value() < 0.1) {
            lastError_ = "注入速率过低";
            return false;
        }
        
        if (workingSetSpin_->value() < 1) {
            lastError_ = "工作集大小无效";
            return false;
        }
        
        if (durationSpin_->value() < 0.1) {
            lastError_ = "持续时间过短";
            return false;
        }
    }
    
    return true;
}

QString WorkloadConfigWidget::getValidationError() const {
    return lastError_;
}
