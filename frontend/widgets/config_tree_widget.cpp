/**
 * @file config_tree_widget.cpp
 * @brief CXL 系统配置树控件实现
 */

#include "config_tree_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QLabel>
#include <QComboBox>

ConfigTreeWidget::ConfigTreeWidget(QWidget *parent)
    : QWidget(parent)
    , tree_(nullptr)
{
    setupUI();
    config_ = cxlsim::ConfigParser::create_default_config();
    populateTree();
}

ConfigTreeWidget::~ConfigTreeWidget() {}

void ConfigTreeWidget::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(12);

    // 专业级页面标题
    auto* headerFrame = new QWidget(this);
    auto* headerLayout = new QHBoxLayout(headerFrame);
    headerLayout->setContentsMargins(0, 0, 0, 8);
    headerLayout->setSpacing(12);
    
    auto* titleLabel = new QLabel("⚙ CONFIGURATION", this);
    titleLabel->setStyleSheet(
        "color: #E8E8E8; font-weight: 600; font-size: 16px; padding: 8px 0;"
    );
    headerLayout->addWidget(titleLabel);
    
    auto* subtitleLabel = new QLabel("系统参数配置", this);
    subtitleLabel->setStyleSheet(
        "color: #666666; font-size: 12px; padding: 8px 0;"
    );
    headerLayout->addWidget(subtitleLabel);
    headerLayout->addStretch();
    
    mainLayout->addWidget(headerFrame);

    tree_ = new QTreeWidget(this);
    tree_->setHeaderHidden(true);
    tree_->setColumnCount(2);
    tree_->setColumnWidth(0, 180);
    tree_->setIndentation(20);
    tree_->setFocusPolicy(Qt::NoFocus);
    tree_->setAnimated(true); // 启用展开/折叠动画
    tree_->setStyleSheet(
        "QTreeWidget { "
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 #050505, stop:1 #000000); "
        "    color: #E8E8E8; "
        "    border: 1px solid #1A1A1A; "
        "    border-radius: 10px; "
        "    padding: 8px; "
        "    outline: none; "
        "}"
        "QTreeWidget::item { "
        "    padding: 10px 12px; "
        "    border-radius: 6px; "
        "    margin: 2px 4px; "
        "    background: transparent; "
        "    min-height: 28px;"
        "}"
        "QTreeWidget::item:selected { "
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 #1E1E1E, stop:1 #141414); "
        "    color: #FFFFFF;"
        "    border-left: 3px solid #4FC3F7;"
        "}"
        "QTreeWidget::item:hover:!selected { "
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 #0F0F0F, stop:1 #0A0A0A); "
        "}"
        "QTreeWidget::branch { background: transparent; }"
        "QTreeWidget::branch:has-siblings:!adjoins-item { border-image: none; }"
        "QTreeWidget::branch:has-siblings:adjoins-item { border-image: none; }"
        "QTreeWidget::branch:!has-children:!has-siblings:adjoins-item { border-image: none; }"
        "QTreeWidget::branch:has-children:!has-siblings:closed, "
        "QTreeWidget::branch:closed:has-children:has-siblings { "
        "    border-image: none; "
        "    image: none; "
        "}"
        "QTreeWidget::branch:open:has-children:!has-siblings, "
        "QTreeWidget::branch:open:has-children:has-siblings { "
        "    border-image: none; "
        "    image: none; "
        "}"
    );

    connect(tree_, &QTreeWidget::itemDoubleClicked,
            this, &ConfigTreeWidget::onItemDoubleClicked);

    mainLayout->addWidget(tree_);

    // ── 操作按钮行：应用配置 / 重置配置 ──
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(8);

    resetButton_ = new QPushButton("↺ 重置配置", this);
    resetButton_->setCursor(Qt::PointingHandCursor);
    resetButton_->setStyleSheet(
        "QPushButton { background: #0F0F0F; color: #888888; "
        "border: 1px solid #2A2A2A; border-radius: 7px; "
        "padding: 10px 16px; font-size: 13px; font-weight: 500; }"
        "QPushButton:hover { background: #1A1A1A; color: #AAAAAA; border-color: #444; }"
        "QPushButton:disabled { color: #444; border-color: #1A1A1A; }");
    connect(resetButton_, &QPushButton::clicked, this, &ConfigTreeWidget::onResetConfig);
    btnLayout->addWidget(resetButton_);

    applyButton_ = new QPushButton("✓ 应用配置", this);
    applyButton_->setCursor(Qt::PointingHandCursor);
    applyButton_->setStyleSheet(
        "QPushButton { background: #0A2A1A; color: #4ADE80; "
        "border: 1px solid #166534; border-radius: 7px; "
        "padding: 10px 16px; font-size: 13px; font-weight: 600; }"
        "QPushButton:hover { background: #0D3D26; color: #86EFAC; border-color: #22C55E; }"
        "QPushButton:disabled { background: #0A0A0A; color: #444; border-color: #1A1A1A; }");
    connect(applyButton_, &QPushButton::clicked, this, &ConfigTreeWidget::onApplyConfig);
    btnLayout->addWidget(applyButton_);

    mainLayout->addLayout(btnLayout);
    updateApplyButtonState();
}

void ConfigTreeWidget::setConfig(const cxlsim::CXLSimConfig& config) {
    config_ = config;
    pendingConfig_ = config;
    isDirty_ = false;
    populateTree();
    updateApplyButtonState();
}

cxlsim::CXLSimConfig ConfigTreeWidget::getConfig() const {
    return isDirty_ ? pendingConfig_ : config_;
}

void ConfigTreeWidget::onApplyConfig() {
    config_ = pendingConfig_;
    isDirty_ = false;
    updateApplyButtonState();
    emit configApplied(config_);
}

void ConfigTreeWidget::onResetConfig() {
    pendingConfig_ = config_;
    isDirty_ = false;
    populateTree();
    updateApplyButtonState();
}

void ConfigTreeWidget::markDirty() {
    if (!isDirty_) {
        isDirty_ = true;
        updateApplyButtonState();
        emit configDirty(true);
    }
}

void ConfigTreeWidget::updateApplyButtonState() {
    applyButton_->setEnabled(isDirty_);
    resetButton_->setEnabled(isDirty_);
    applyButton_->setText(isDirty_ ? "✓ 应用配置 ●" : "✓ 应用配置");
}

static QTreeWidgetItem* makeCategory(QTreeWidget* tree, const QString& text) {
    auto* item = new QTreeWidgetItem(tree);
    item->setText(0, text);
    QFont f("Inter, -apple-system", 11, QFont::Bold);
    item->setFont(0, f);
    item->setForeground(0, QColor(0xAA, 0xAA, 0xAA));
    // 强制清除背景色，避免重叠
    item->setBackground(0, Qt::NoBrush);
    item->setBackground(1, Qt::NoBrush);
    item->setExpanded(true);
    item->setFlags(item->flags() & ~Qt::ItemIsSelectable); // 分类不可被选中
    return item;
}

static QTreeWidgetItem* makeRow(QTreeWidgetItem* parent,
                                const QString& prop, const QString& val,
                                bool editable = false) {
    auto* item = new QTreeWidgetItem(parent);
    item->setText(0, prop);
    item->setText(1, val);
    item->setForeground(0, QColor(0x88, 0x88, 0x88));
    item->setBackground(0, Qt::NoBrush);
    item->setBackground(1, Qt::NoBrush);
    
    // 值采用类似代码的高亮
    if (editable) {
        item->setForeground(1, QColor(0x60, 0xA5, 0xFA)); // 可编辑蓝
        item->setToolTip(1, "Double click to edit");
    } else {
        item->setForeground(1, QColor(0xED, 0xED, 0xED));
    }
    
    if (editable) item->setFlags(item->flags() | Qt::ItemIsEditable);
    return item;
}

void ConfigTreeWidget::populateTree() {
    tree_->clear();

    // 配置名称
    auto* nameItem = new QTreeWidgetItem(tree_);
    nameItem->setText(0, "\u914d\u7f6e\u540d\u79f0");
    nameItem->setText(1, QString::fromStdString(config_.name));
    nameItem->setFlags(nameItem->flags() | Qt::ItemIsEditable);
    nameItem->setForeground(0, QColor(0xF9, 0xA8, 0x25));
    nameItem->setForeground(1, QColor(0xE0, 0xE0, 0xE0));

    addRootComplexItem();
    addSwitchesItem();
    addDevicesItem();
    addConnectionsItem();
    addSimulationItem();

    tree_->expandAll();
}

void ConfigTreeWidget::addRootComplexItem() {
    auto* rcRoot = makeCategory(tree_, "\u25b6 \u6839\u590d\u5408\u4f53 (Root Complex)");
    makeRow(rcRoot, "  \u6807\u8bc6\u7b26", QString::fromStdString(config_.root_complex.id));
    makeRow(rcRoot, "  \u672c\u5730DRAM\u5927\u5c0f (GB)",
            QString::number(config_.root_complex.local_dram_size_gb), true);
    makeRow(rcRoot, "  \u672c\u5730DRAM\u5ef6\u8fdf (ns)", "90 (硬编码)");
}

void ConfigTreeWidget::addSwitchesItem() {
    auto* swRoot = makeCategory(tree_,
        QString("\u25b6 CXL \u4ea4\u6362\u673a (%1\u4e2a)").arg(config_.switches.size()));
    for (auto& sw : config_.switches) {
        auto* swItem = new QTreeWidgetItem(swRoot);
        swItem->setText(0, QString("  \u25cf %1").arg(QString::fromStdString(sw.id)));
        swItem->setForeground(0, QColor(0x81, 0xC7, 0x84));
        swItem->setFlags(Qt::ItemIsEnabled);
        makeRow(swItem, "    \u7aef\u53e3\u6570", QString::number(sw.num_ports));
        makeCombo(swItem, "    \u5ef6\u8fdf (ns)",
                  {"15", "20", "30", "40", "60"},
                  QString::number(static_cast<int>(sw.latency_ns)),
                  QString("sw_latency_%1").arg(QString::fromStdString(sw.id)));
        swItem->setExpanded(true);
    }
}

void ConfigTreeWidget::addDevicesItem() {
    auto* devRoot = makeCategory(tree_,
        QString("\u25b6 CXL \u5185\u5b58\u8bbe\u5907 (%1\u4e2a)").arg(config_.cxl_devices.size()));
    devRoot->setExpanded(true);

    for (int i = 0; i < static_cast<int>(config_.cxl_devices.size()); ++i) {
        const auto& dev = config_.cxl_devices[i];
        QString devKey = QString::fromStdString(dev.id);

        auto* devItem = new QTreeWidgetItem(devRoot);
        devItem->setText(0, QString("  \u25cf %1").arg(devKey));
        devItem->setForeground(0, QColor(0x4F, 0xC3, 0xF7));
        devItem->setFlags(Qt::ItemIsEnabled);

        makeCombo(devItem, "    \u7c7b\u578b",
                  {"Type1", "Type2", "Type3"},
                  QString::fromStdString(dev.type),
                  QString("dev_type_%1").arg(devKey));
        makeCombo(devItem, "    PCIe Gen",
                  {"Gen4", "Gen5"},
                  QString::fromStdString(dev.link_gen),
                  QString("dev_gen_%1").arg(devKey));
        makeCombo(devItem, "    \u94fe\u8def\u5bbd\u5ea6",
                  {"x4", "x8", "x16"},
                  QString::fromStdString(dev.link_width),
                  QString("dev_width_%1").arg(devKey));
        makeRow(devItem, "    \u5ef6\u8fdf (ns)",
                QString::number(dev.base_latency_ns), true);

        devItem->setExpanded(true);
    }
}

void ConfigTreeWidget::addConnectionsItem() {
    auto* connRoot = makeCategory(tree_,
        QString("\u25b6 \u62d3\u6251\u8fde\u63a5 (%1\u6761)").arg(config_.connections.size()));
    for (int i = 0; i < static_cast<int>(config_.connections.size()); ++i) {
        const auto& conn = config_.connections[i];
        QString connKey = QString("%1_to_%2")
            .arg(QString::fromStdString(conn.from))
            .arg(QString::fromStdString(conn.to));

        auto* connItem = new QTreeWidgetItem(connRoot);
        connItem->setText(0, QString("  %1 \u2192 %2")
            .arg(QString::fromStdString(conn.from))
            .arg(QString::fromStdString(conn.to)));
        connItem->setForeground(0, QColor(0x88, 0x88, 0x88));
        connItem->setFlags(Qt::ItemIsEnabled);

        QString linkStr  = QString::fromStdString(conn.link);
        QString curGen   = linkStr.contains("4") ? "Gen4" : "Gen5";
        QString curWidth = linkStr.contains("x4") ? "x4"
                         : linkStr.contains("x8") ? "x8" : "x16";

        makeCombo(connItem, "    PCIe Gen",
                  {"Gen4", "Gen5"}, curGen,
                  QString("conn_gen_%1").arg(connKey));
        makeCombo(connItem, "    \u94fe\u8def\u5bbd\u5ea6",
                  {"x4", "x8", "x16"}, curWidth,
                  QString("conn_width_%1").arg(connKey));

        connItem->setExpanded(true);
    }
}

void ConfigTreeWidget::addSimulationItem() {
    auto* simRoot = makeCategory(tree_, "▶ 模拟引擎参数");
    simRoot->setToolTip(0, "全局引擎级别的参数配置");

    makeCombo(simRoot, "  Epoch时长 (ms)",
              {"5", "10", "20", "50", "100"},
              QString::number(config_.simulation.epoch_ms),
              "sim_epoch_ms");
    makeCombo(simRoot, "  MLP优化",
              {"✓ 开启", "✗ 关闭"},
              config_.simulation.enable_mlp_optimization ? "✓ 开启" : "✗ 关闭",
              "sim_mlp");
    makeCombo(simRoot, "  拥塞模型",
              {"✓ 开启", "✗ 关闭"},
              config_.simulation.enable_congestion_model ? "✓ 开启" : "✗ 关闭",
              "sim_congestion");
    makeRow(simRoot, "  热点检测", "关闭 (未开发)");
}

QComboBox* ConfigTreeWidget::makeCombo(QTreeWidgetItem* parent,
                                       const QString& label,
                                       const QStringList& options,
                                       const QString& current,
                                       const QString& key) {
    auto* item = new QTreeWidgetItem(parent);
    item->setText(0, label);
    item->setForeground(0, QColor(0x88, 0x88, 0x88));
    item->setBackground(0, Qt::NoBrush);
    item->setBackground(1, Qt::NoBrush);
    item->setFlags(Qt::ItemIsEnabled);

    itemKeyMap_[item] = key;

    auto* combo = new QComboBox(tree_);
    combo->addItems(options);

    // 将当前值匹配到列表项，找不到就取第一项
    int idx = options.indexOf(current);
    if (idx < 0) idx = 0;
    combo->setCurrentIndex(idx);

    combo->setStyleSheet(
        "QComboBox { "
        "    background: #0D0D0D; "
        "    color: #60A5FA; "
        "    border: 1px solid #2A2A2A; "
        "    border-radius: 4px; "
        "    padding: 2px 8px; "
        "    font-size: 12px; "
        "}"
        "QComboBox::drop-down { border: none; width: 20px; }"
        "QComboBox QAbstractItemView { "
        "    background: #111111; "
        "    color: #E8E8E8; "
        "    selection-background-color: #1E3A5F; "
        "}");

    tree_->setItemWidget(item, 1, combo);

    connect(combo, &QComboBox::currentTextChanged,
            this, [this, item](const QString& val) {
                onComboChanged(item, val);
            });

    return combo;
}

void ConfigTreeWidget::onComboChanged(QTreeWidgetItem* item, const QString& value) {
    QString key = itemKeyMap_.value(item);
    if (key.isEmpty()) return;

    // 交换机延迟
    if (key.startsWith("sw_latency_")) {
        QString swId = key.mid(QString("sw_latency_").length());
        for (auto& sw : pendingConfig_.switches) {
            if (QString::fromStdString(sw.id) == swId) {
                sw.latency_ns = value.toDouble();
                break;
            }
        }

    // CXL设备类型
    } else if (key.startsWith("dev_type_")) {
        QString devId = key.mid(QString("dev_type_").length());
        for (auto& dev : pendingConfig_.cxl_devices) {
            if (QString::fromStdString(dev.id) == devId) {
                dev.type = value.toStdString();
                break;
            }
        }

    // CXL设备 PCIe Gen
    } else if (key.startsWith("dev_gen_")) {
        QString devId = key.mid(QString("dev_gen_").length());
        for (auto& dev : pendingConfig_.cxl_devices) {
            if (QString::fromStdString(dev.id) == devId) {
                dev.link_gen = value.toStdString();
                int gen = value.endsWith("4") ? 4 : 5;
                int w   = dev.link_width == "x4" ? 4 : (dev.link_width == "x8" ? 8 : 16);
                dev.bandwidth_gbps = (gen == 5 ? 64.0 : 32.0) * w / 16;
                QString genNum = (gen == 4) ? "4.0" : "5.0";
                for (auto& conn : pendingConfig_.connections) {
                    if (conn.to == dev.id) {
                        QString cur = QString::fromStdString(conn.link);
                        QString width = cur.contains("x4") ? "x4" : (cur.contains("x8") ? "x8" : "x16");
                        conn.link = QString("PCIe%1-%2").arg(genNum).arg(width).toStdString();
                    }
                }
                break;
            }
        }

    // CXL设备链路宽度
    } else if (key.startsWith("dev_width_")) {
        QString devId = key.mid(QString("dev_width_").length());
        for (auto& dev : pendingConfig_.cxl_devices) {
            if (QString::fromStdString(dev.id) == devId) {
                dev.link_width = value.toStdString();
                int gen = dev.link_gen == "Gen4" ? 4 : 5;
                int w   = value == "x4" ? 4 : (value == "x8" ? 8 : 16);
                dev.bandwidth_gbps = (gen == 5 ? 64.0 : 32.0) * w / 16;
                QString genNum = (gen == 4) ? "4.0" : "5.0";
                for (auto& conn : pendingConfig_.connections) {
                    if (conn.to == dev.id) {
                        conn.link = QString("PCIe%1-%2").arg(genNum).arg(value).toStdString();
                    }
                }
                break;
            }
        }

    // 连接 PCIe Gen（更新 link 字符串）
    } else if (key.startsWith("conn_gen_")) {
        QString connKey = key.mid(QString("conn_gen_").length());
        for (auto& conn : pendingConfig_.connections) {
            QString ck = QString("%1_to_%2")
                .arg(QString::fromStdString(conn.from))
                .arg(QString::fromStdString(conn.to));
            if (ck == connKey) {
                QString cur = QString::fromStdString(conn.link);
                QString width = cur.contains("x4") ? "x4" : (cur.contains("x8") ? "x8" : "x16");
                QString genNum = value == "Gen4" ? "4.0" : "5.0";
                conn.link = QString("PCIe%1-%2").arg(genNum).arg(width).toStdString();
                break;
            }
        }

    // 连接链路宽度
    } else if (key.startsWith("conn_width_")) {
        QString connKey = key.mid(QString("conn_width_").length());
        for (auto& conn : pendingConfig_.connections) {
            QString ck = QString("%1_to_%2")
                .arg(QString::fromStdString(conn.from))
                .arg(QString::fromStdString(conn.to));
            if (ck == connKey) {
                QString cur = QString::fromStdString(conn.link);
                QString genNum = cur.contains("4") ? "4.0" : "5.0";
                conn.link = QString("PCIe%1-%2").arg(genNum).arg(value).toStdString();
                break;
            }
        }

    // 模拟引擎参数
    } else if (key == "sim_epoch_ms") {
        pendingConfig_.simulation.epoch_ms = value.toInt();
    } else if (key == "sim_mlp") {
        pendingConfig_.simulation.enable_mlp_optimization = value.startsWith("✓");
    } else if (key == "sim_congestion") {
        pendingConfig_.simulation.enable_congestion_model = value.startsWith("✓");
    }

    markDirty();
}

void ConfigTreeWidget::onItemDoubleClicked(QTreeWidgetItem* item, int column) {
    if (column != 1) return;
    QString prop = item->text(0).trimmed();
    QString cur  = item->text(1);
    bool ok;
    QString nv = QInputDialog::getText(
        this, "编辑属性", prop + ":", QLineEdit::Normal, cur, &ok);
    if (ok && !nv.isEmpty()) {
        item->setText(1, nv);
        // 将可编辑行的文本变更写入 pendingConfig_
        if (prop.contains("DRAM大小")) {
            pendingConfig_.root_complex.local_dram_size_gb = nv.toULongLong();
        } else if (prop.contains("延迟") && prop.contains("ns")) {
            // 设备延迟行 -- 父节点文本格式: "  ● CXL_MEM1"，去掉●和空格提取ID
            auto* parent = item->parent();
            if (parent) {
                QString raw = parent->text(0).trimmed(); // "● CXL_MEM1"
                // 去掉开头的 ● 符号（单字节或多字节）及紧随的空格
                int dotPos = raw.indexOf(QChar(0x25CF)); // ●
                QString devId = (dotPos >= 0)
                    ? raw.mid(dotPos + 1).trimmed()
                    : raw.trimmed();
                for (auto& dev : pendingConfig_.cxl_devices) {
                    if (QString::fromStdString(dev.id) == devId) {
                        dev.base_latency_ns = nv.toDouble();
                        break;
                    }
                }
            }
        }
        markDirty();
    }
}

void ConfigTreeWidget::onAddDevice() {
    bool ok;
    QString id = QInputDialog::getText(
        this, "添加 CXL 内存设备",
        "设备标识符 (ID):",
        QLineEdit::Normal,
        QString("CXL_MEM%1").arg(pendingConfig_.cxl_devices.size()), &ok);
    if (!ok || id.isEmpty()) return;

    cxlsim::CXLDeviceConfig dev;
    dev.id              = id.toStdString();
    dev.type            = "Type3";
    dev.capacity_gb     = 128;
    dev.bandwidth_gbps  = 64.0;
    dev.base_latency_ns = 170.0;

    pendingConfig_.cxl_devices.push_back(dev);
    populateTree();
    onApplyConfig(); // 结构变更直接生效并发出 configApplied 信号
}

void ConfigTreeWidget::onAddSwitch() {
    bool ok;
    QString id = QInputDialog::getText(
        this, "\u6dfb\u52a0 CXL \u4ea4\u6362\u673a",
        "\u4ea4\u6362\u673a\u6807\u8bc6\u7b26 (ID):",
        QLineEdit::Normal,
        QString("SW%1").arg(pendingConfig_.switches.size()), &ok);
    if (!ok || id.isEmpty()) return;

    cxlsim::SwitchConfig sw;
    sw.id        = id.toStdString();
    sw.num_ports = 8;
    sw.latency_ns = 40.0;

    pendingConfig_.switches.push_back(sw);
    populateTree();
    onApplyConfig(); // 结构变更直接生效
}

void ConfigTreeWidget::onRemoveSelected() {
    QTreeWidgetItem* item = tree_->currentItem();
    if (!item) {
        QMessageBox::information(this, "\u63d0\u793a", "\u8bf7\u5148\u9009\u4e2d\u8981\u5220\u9664\u7684\u8bbe\u5907\u6216\u4ea4\u6362\u673a\u8282\u70b9");
        return;
    }

    QString itemText = item->text(0).trimmed();
    
    // 检查是否是设备节点（以 ● 开头）
    if (itemText.startsWith("●")) {
        QString id = itemText.mid(1).trimmed(); // 移除 ● 符号
        
        // 尝试删除设备
        for (auto it = pendingConfig_.cxl_devices.begin(); it != pendingConfig_.cxl_devices.end(); ++it) {
            if (QString::fromStdString(it->id) == id) {
                pendingConfig_.cxl_devices.erase(it);
                populateTree();
                onApplyConfig();
                return;
            }
        }
        
        // 尝试删除交换机
        for (auto it = pendingConfig_.switches.begin(); it != pendingConfig_.switches.end(); ++it) {
            if (QString::fromStdString(it->id) == id) {
                pendingConfig_.switches.erase(it);
                populateTree();
                onApplyConfig();
                return;
            }
        }
    }
    
    QMessageBox::information(this, "提示",
        "请选中具体的设备或交换机节点（带 ● 标记的项）进行删除");
}

