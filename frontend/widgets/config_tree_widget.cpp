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

ConfigTreeWidget::ConfigTreeWidget(QWidget *parent)
    : QWidget(parent)
    , tree_(nullptr)
    , addDeviceButton_(nullptr)
    , addSwitchButton_(nullptr)
    , removeButton_(nullptr)
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

    auto* titleLabel = new QLabel("CONFIGURATION", this);
    titleLabel->setStyleSheet(
        "color: #888888; font-weight: 700; font-size: 11px; letter-spacing: 1px; padding: 4px 2px;"
    );
    mainLayout->addWidget(titleLabel);

    tree_ = new QTreeWidget(this);
    tree_->setHeaderHidden(true); // 隐藏表头，采用无边框设计
    tree_->setColumnCount(2);
    tree_->setColumnWidth(0, 160);
    tree_->setIndentation(16);
    tree_->setFocusPolicy(Qt::NoFocus);
    tree_->setStyleSheet(
        "QTreeWidget { background-color: transparent; color: #EDEDED; border: none; outline: none; }"
        "QTreeWidget::item { padding: 6px; border-radius: 6px; margin: 2px 0; }"
        "QTreeWidget::item:selected { background-color: #222222; }"
        "QTreeWidget::item:hover:!selected { background-color: #111111; }"
        "QTreeWidget::branch:has-siblings:!adjoins-item { border-image: none; }"
        "QTreeWidget::branch:has-siblings:adjoins-item { border-image: none; }"
        "QTreeWidget::branch:!has-children:!has-siblings:adjoins-item { border-image: none; }"
        "QTreeWidget::branch:has-children:!has-siblings:closed, QTreeWidget::branch:closed:has-children:has-siblings { border-image: none; image: none; }"
        "QTreeWidget::branch:open:has-children:!has-siblings, QTreeWidget::branch:open:has-children:has-siblings { border-image: none; image: none; }"
    );

    connect(tree_, &QTreeWidget::itemDoubleClicked,
            this, &ConfigTreeWidget::onItemDoubleClicked);

    mainLayout->addWidget(tree_);

    // 操作按钮行 (现代化极简按钮)
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(8);

    QString ghostBtnStyle = 
        "QPushButton { background: #111111; color: #EDEDED; border: 1px solid #333333; border-radius: 6px; padding: 8px; font-size: 12px; font-weight: 500; }"
        "QPushButton:hover { background: #1A1A1A; border-color: #555555; }"
        "QPushButton:pressed { background: #222222; }";

    addDeviceButton_ = new QPushButton("+ Device", this);
    addDeviceButton_->setStyleSheet(ghostBtnStyle);
    connect(addDeviceButton_, &QPushButton::clicked, this, &ConfigTreeWidget::onAddDevice);
    btnLayout->addWidget(addDeviceButton_);

    addSwitchButton_ = new QPushButton("+ Switch", this);
    addSwitchButton_->setStyleSheet(ghostBtnStyle);
    connect(addSwitchButton_, &QPushButton::clicked, this, &ConfigTreeWidget::onAddSwitch);
    btnLayout->addWidget(addSwitchButton_);

    removeButton_ = new QPushButton("Remove", this);
    removeButton_->setStyleSheet(
        "QPushButton { background: transparent; color: #888888; border: 1px dashed #333333; border-radius: 6px; padding: 8px; font-size: 12px; font-weight: 500; }"
        "QPushButton:hover { background: #2A0808; color: #F87171; border: 1px solid #7F1D1D; }"
        "QPushButton:pressed { background: #450A0A; }"
    );
    connect(removeButton_, &QPushButton::clicked, this, &ConfigTreeWidget::onRemoveSelected);
    btnLayout->addWidget(removeButton_);

    mainLayout->addLayout(btnLayout);
}

void ConfigTreeWidget::setConfig(const cxlsim::CXLSimConfig& config) {
    config_ = config;
    populateTree();
    // 不发出 configChanged 信号，避免循环更新
}

cxlsim::CXLSimConfig ConfigTreeWidget::getConfig() const {
    return config_;
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
}

void ConfigTreeWidget::addSwitchesItem() {
    auto* swRoot = makeCategory(tree_,
        QString("\u25b6 CXL \u4ea4\u6362\u673a (%1\u4e2a)").arg(config_.switches.size()));
    for (const auto& sw : config_.switches) {
        auto* swItem = new QTreeWidgetItem(swRoot);
        swItem->setText(0, QString("  \u25cf %1").arg(QString::fromStdString(sw.id)));
        swItem->setForeground(0, QColor(0x81, 0xC7, 0x84));
        makeRow(swItem, "    \u7aef\u53e3\u6570", QString::number(sw.num_ports));
        makeRow(swItem, "    \u5ef6\u8fdf (ns)", QString::number(sw.latency_ns), true);
    }
}

void ConfigTreeWidget::addDevicesItem() {
    auto* devRoot = makeCategory(tree_,
        QString("\u25b6 CXL \u5185\u5b58\u8bbe\u5907 (%1\u4e2a)").arg(config_.cxl_devices.size()));
    devRoot->setExpanded(true);

    for (const auto& dev : config_.cxl_devices) {
        auto* devItem = new QTreeWidgetItem(devRoot);
        devItem->setText(0, QString("  \u25cf %1").arg(QString::fromStdString(dev.id)));
        devItem->setForeground(0, QColor(0x4F, 0xC3, 0xF7));
        makeRow(devItem, "    \u7c7b\u578b",         QString::fromStdString(dev.type));
        makeRow(devItem, "    \u5bb9\u91cf (GB)",    QString::number(dev.capacity_gb),    true);
        makeRow(devItem, "    \u5e26\u5bbd (GB/s)",  QString::number(dev.bandwidth_gbps), true);
        makeRow(devItem, "    \u57fa\u51c6\u5ef6\u8fdf (ns)", QString::number(dev.base_latency_ns), true);
    }
}

void ConfigTreeWidget::addConnectionsItem() {
    auto* connRoot = makeCategory(tree_, QString("▶ 拓扑连接 (%1条)").arg(config_.connections.size()));
    for (const auto& conn : config_.connections) {
        auto* connItem = new QTreeWidgetItem(connRoot);
        connItem->setText(0, QString("  %1 → %2")
            .arg(QString::fromStdString(conn.from))
            .arg(QString::fromStdString(conn.to)));
        connItem->setText(1, QString::fromStdString(conn.link));
        connItem->setForeground(0, QColor(0x88, 0x88, 0x88));
        connItem->setForeground(1, QColor(0x60, 0xA5, 0xFA));
        connItem->setBackground(0, Qt::NoBrush);
        connItem->setBackground(1, Qt::NoBrush);
    }
}

void ConfigTreeWidget::addSimulationItem() {
    auto* simRoot = makeCategory(tree_, "▶ 模拟参数");
    makeRow(simRoot, "  Epoch时长 (ms)", QString::number(config_.simulation.epoch_ms), true);
    makeRow(simRoot, "  MLP优化", config_.simulation.enable_mlp_optimization ? "开启" : "关闭");
    makeRow(simRoot, "  拥塞模型", config_.simulation.enable_congestion_model ? "开启" : "关闭");
}

void ConfigTreeWidget::onItemDoubleClicked(QTreeWidgetItem* item, int column) {
    if (column != 1) return;
    QString prop = item->text(0).trimmed();
    QString cur  = item->text(1);
    bool ok;
    QString nv = QInputDialog::getText(
        this, "\u7f16\u8f91\u5c5e\u6027", prop + ":", QLineEdit::Normal, cur, &ok);
    if (ok && !nv.isEmpty()) {
        item->setText(1, nv);
        emit configChanged(config_);
    }
}

void ConfigTreeWidget::onAddDevice() {
    bool ok;
    QString id = QInputDialog::getText(
        this, "\u6dfb\u52a0 CXL \u5185\u5b58\u8bbe\u5907",
        "\u8bbe\u5907\u6807\u8bc6\u7b26 (ID):",
        QLineEdit::Normal,
        QString("CXL_MEM%1").arg(config_.cxl_devices.size()), &ok);
    if (!ok || id.isEmpty()) return;

    cxlsim::CXLDeviceConfig dev;
    dev.id              = id.toStdString();
    dev.type            = "Type3";
    dev.capacity_gb     = 128;
    dev.bandwidth_gbps  = 64.0;
    dev.base_latency_ns = 170.0;

    config_.cxl_devices.push_back(dev);
    populateTree();
    emit configChanged(config_);
}

void ConfigTreeWidget::onAddSwitch() {
    bool ok;
    QString id = QInputDialog::getText(
        this, "\u6dfb\u52a0 CXL \u4ea4\u6362\u673a",
        "\u4ea4\u6362\u673a\u6807\u8bc6\u7b26 (ID):",
        QLineEdit::Normal,
        QString("SW%1").arg(config_.switches.size()), &ok);
    if (!ok || id.isEmpty()) return;

    cxlsim::SwitchConfig sw;
    sw.id        = id.toStdString();
    sw.num_ports = 8;
    sw.latency_ns = 40.0;

    config_.switches.push_back(sw);
    populateTree();
    emit configChanged(config_);
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
        for (auto it = config_.cxl_devices.begin(); it != config_.cxl_devices.end(); ++it) {
            if (QString::fromStdString(it->id) == id) {
                config_.cxl_devices.erase(it);
                populateTree();
                emit configChanged(config_);
                return;
            }
        }
        
        // 尝试删除交换机
        for (auto it = config_.switches.begin(); it != config_.switches.end(); ++it) {
            if (QString::fromStdString(it->id) == id) {
                config_.switches.erase(it);
                populateTree();
                emit configChanged(config_);
                return;
            }
        }
    }
    
    QMessageBox::information(this, "提示",
        "请选中具体的设备或交换机节点（带 ● 标记的项）进行删除");
}
