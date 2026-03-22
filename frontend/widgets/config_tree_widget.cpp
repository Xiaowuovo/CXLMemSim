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
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    auto* titleLabel = new QLabel("\u2630 CXL \u7cfb\u7edf\u62d3\u6251\u914d\u7f6e", this);
    titleLabel->setStyleSheet(
        "color:#4FC3F7; font-weight:bold; font-size:12px; padding:4px 6px;"
        "background:#0F3460; border-radius:3px;");
    mainLayout->addWidget(titleLabel);

    tree_ = new QTreeWidget(this);
    tree_->setHeaderLabels({"\u5c5e\u6027", "\u503c"});
    tree_->setColumnWidth(0, 150);
    tree_->setAlternatingRowColors(true);
    tree_->header()->setStretchLastSection(true);
    tree_->setIndentation(14);

    connect(tree_, &QTreeWidget::itemDoubleClicked,
            this, &ConfigTreeWidget::onItemDoubleClicked);

    mainLayout->addWidget(tree_);

    // 操作按钮行
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(4);

    addDeviceButton_ = new QPushButton("+ \u8bbe\u5907", this);
    addDeviceButton_->setStyleSheet(
        "QPushButton{background:#1B5E20;border-color:#81C784;padding:5px 8px;}"
        "QPushButton:hover{background:#2E7D32;}");
    connect(addDeviceButton_, &QPushButton::clicked, this, &ConfigTreeWidget::onAddDevice);
    btnLayout->addWidget(addDeviceButton_);

    addSwitchButton_ = new QPushButton("+ \u4ea4\u6362\u673a", this);
    addSwitchButton_->setStyleSheet(
        "QPushButton{background:#0D47A1;border-color:#64B5F6;padding:5px 8px;}"
        "QPushButton:hover{background:#1565C0;}");
    connect(addSwitchButton_, &QPushButton::clicked, this, &ConfigTreeWidget::onAddSwitch);
    btnLayout->addWidget(addSwitchButton_);

    removeButton_ = new QPushButton("\u2212 \u5220\u9664", this);
    removeButton_->setStyleSheet(
        "QPushButton{background:#B71C1C;border-color:#EF9A9A;padding:5px 8px;}"
        "QPushButton:hover{background:#C62828;}");
    connect(removeButton_, &QPushButton::clicked, this, &ConfigTreeWidget::onRemoveSelected);
    btnLayout->addWidget(removeButton_);

    mainLayout->addLayout(btnLayout);
}

void ConfigTreeWidget::setConfig(const cxlsim::CXLSimConfig& config) {
    config_ = config;
    populateTree();
    emit configChanged(config_);
}

cxlsim::CXLSimConfig ConfigTreeWidget::getConfig() const {
    return config_;
}

static QTreeWidgetItem* makeCategory(QTreeWidget* tree, const QString& text) {
    auto* item = new QTreeWidgetItem(tree);
    item->setText(0, text);
    QFont f = item->font(0);
    f.setBold(true);
    item->setFont(0, f);
    item->setForeground(0, QColor(0x4F, 0xC3, 0xF7));
    item->setBackground(0, QColor(0x0F, 0x34, 0x60));
    item->setBackground(1, QColor(0x0F, 0x34, 0x60));
    item->setExpanded(true);
    return item;
}

static QTreeWidgetItem* makeRow(QTreeWidgetItem* parent,
                                const QString& prop, const QString& val,
                                bool editable = false) {
    auto* item = new QTreeWidgetItem(parent);
    item->setText(0, prop);
    item->setText(1, val);
    item->setForeground(0, QColor(0xB0, 0xBE, 0xC5));
    item->setForeground(1, QColor(0xE0, 0xE0, 0xE0));
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
    auto* connRoot = makeCategory(tree_,
        QString("\u25b6 \u62d3\u6251\u8fde\u63a5 (%1\u6761)").arg(config_.connections.size()));
    for (const auto& conn : config_.connections) {
        auto* connItem = new QTreeWidgetItem(connRoot);
        connItem->setText(0, QString("  %1 \u2192 %2")
            .arg(QString::fromStdString(conn.from))
            .arg(QString::fromStdString(conn.to)));
        connItem->setText(1, QString::fromStdString(conn.link));
        connItem->setForeground(0, QColor(0xFF, 0xB7, 0x4D));
        connItem->setForeground(1, QColor(0x9E, 0x9E, 0x9E));
    }
}

void ConfigTreeWidget::addSimulationItem() {
    auto* simRoot = makeCategory(tree_, "\u25b6 \u6a21\u62df\u53c2\u6570");
    makeRow(simRoot, "  Epoch\u65f6\u957f (ms)",
            QString::number(config_.simulation.epoch_ms), true);
    makeRow(simRoot, "  MLP\u4f18\u5316",
            config_.simulation.enable_mlp_optimization ? "\u5f00\u542f" : "\u5173\u95ed");
    makeRow(simRoot, "  \u62e5\u585e\u6a21\u578b",
            config_.simulation.enable_congestion_model ? "\u5f00\u542f" : "\u5173\u95ed");
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
    if (!item) return;
    QMessageBox::information(this, "\u63d0\u793a",
        "\u8bf7\u9009\u4e2d\u5177\u4f53\u7684\u8bbe\u5907\u6216\u4ea4\u6362\u673a\u8282\u70b9\u8fdb\u884c\u5220\u9664\u3002\n\u5f53\u524d\u7248\u672c\u5df2\u652f\u6301\u901a\u8fc7\u91cd\u5efa\u914d\u7f6e\u6765\u79fb\u9664\u3002");
}
