/**
 * @file config_tree_widget.cpp
 * @brief Configuration tree widget implementation
 */

#include "config_tree_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>

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

ConfigTreeWidget::~ConfigTreeWidget() {
}

void ConfigTreeWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Tree widget
    tree_ = new QTreeWidget(this);
    tree_->setHeaderLabels({"Property", "Value"});
    tree_->setColumnWidth(0, 200);
    tree_->setAlternatingRowColors(true);

    connect(tree_, &QTreeWidget::itemDoubleClicked,
            this, &ConfigTreeWidget::onItemDoubleClicked);

    mainLayout->addWidget(tree_);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    addDeviceButton_ = new QPushButton("Add Device", this);
    connect(addDeviceButton_, &QPushButton::clicked, this, &ConfigTreeWidget::onAddDevice);
    buttonLayout->addWidget(addDeviceButton_);

    addSwitchButton_ = new QPushButton("Add Switch", this);
    connect(addSwitchButton_, &QPushButton::clicked, this, &ConfigTreeWidget::onAddSwitch);
    buttonLayout->addWidget(addSwitchButton_);

    removeButton_ = new QPushButton("Remove", this);
    connect(removeButton_, &QPushButton::clicked, this, &ConfigTreeWidget::onRemoveSelected);
    buttonLayout->addWidget(removeButton_);

    mainLayout->addLayout(buttonLayout);
}

void ConfigTreeWidget::setConfig(const cxlsim::CXLSimConfig& config) {
    config_ = config;
    populateTree();
    emit configChanged(config_);
}

cxlsim::CXLSimConfig ConfigTreeWidget::getConfig() const {
    return config_;
}

void ConfigTreeWidget::populateTree() {
    tree_->clear();

    // Configuration name
    QTreeWidgetItem* nameItem = new QTreeWidgetItem(tree_);
    nameItem->setText(0, "Configuration Name");
    nameItem->setText(1, QString::fromStdString(config_.name));
    nameItem->setFlags(nameItem->flags() | Qt::ItemIsEditable);

    addRootComplexItem();
    addSwitchesItem();
    addDevicesItem();
    addConnectionsItem();
    addSimulationItem();

    tree_->expandAll();
}

void ConfigTreeWidget::addRootComplexItem() {
    QTreeWidgetItem* rcRoot = new QTreeWidgetItem(tree_);
    rcRoot->setText(0, "Root Complex");
    rcRoot->setExpanded(true);

    QTreeWidgetItem* idItem = new QTreeWidgetItem(rcRoot);
    idItem->setText(0, "ID");
    idItem->setText(1, QString::fromStdString(config_.root_complex.id));

    QTreeWidgetItem* dramItem = new QTreeWidgetItem(rcRoot);
    dramItem->setText(0, "Local DRAM (GB)");
    dramItem->setText(1, QString::number(config_.root_complex.local_dram_size_gb));
}

void ConfigTreeWidget::addSwitchesItem() {
    QTreeWidgetItem* switchRoot = new QTreeWidgetItem(tree_);
    switchRoot->setText(0, "Switches");
    switchRoot->setText(1, QString("(%1)").arg(config_.switches.size()));

    for (const auto& sw : config_.switches) {
        QTreeWidgetItem* swItem = new QTreeWidgetItem(switchRoot);
        swItem->setText(0, QString::fromStdString(sw.id));
        swItem->setExpanded(false);

        QTreeWidgetItem* portsItem = new QTreeWidgetItem(swItem);
        portsItem->setText(0, "Ports");
        portsItem->setText(1, QString::number(sw.num_ports));

        QTreeWidgetItem* latencyItem = new QTreeWidgetItem(swItem);
        latencyItem->setText(0, "Latency (ns)");
        latencyItem->setText(1, QString::number(sw.latency_ns));
    }
}

void ConfigTreeWidget::addDevicesItem() {
    QTreeWidgetItem* devRoot = new QTreeWidgetItem(tree_);
    devRoot->setText(0, "CXL Devices");
    devRoot->setText(1, QString("(%1)").arg(config_.cxl_devices.size()));
    devRoot->setExpanded(true);

    for (const auto& dev : config_.cxl_devices) {
        QTreeWidgetItem* devItem = new QTreeWidgetItem(devRoot);
        devItem->setText(0, QString::fromStdString(dev.id));
        devItem->setExpanded(false);

        QTreeWidgetItem* typeItem = new QTreeWidgetItem(devItem);
        typeItem->setText(0, "Type");
        typeItem->setText(1, QString::fromStdString(dev.type));

        QTreeWidgetItem* capacityItem = new QTreeWidgetItem(devItem);
        capacityItem->setText(0, "Capacity (GB)");
        capacityItem->setText(1, QString::number(dev.capacity_gb));

        QTreeWidgetItem* bwItem = new QTreeWidgetItem(devItem);
        bwItem->setText(0, "Bandwidth (GB/s)");
        bwItem->setText(1, QString::number(dev.bandwidth_gbps));

        QTreeWidgetItem* latItem = new QTreeWidgetItem(devItem);
        latItem->setText(0, "Base Latency (ns)");
        latItem->setText(1, QString::number(dev.base_latency_ns));
    }
}

void ConfigTreeWidget::addConnectionsItem() {
    QTreeWidgetItem* connRoot = new QTreeWidgetItem(tree_);
    connRoot->setText(0, "Connections");
    connRoot->setText(1, QString("(%1)").arg(config_.connections.size()));

    for (const auto& conn : config_.connections) {
        QTreeWidgetItem* connItem = new QTreeWidgetItem(connRoot);
        QString label = QString("%1 → %2")
            .arg(QString::fromStdString(conn.from))
            .arg(QString::fromStdString(conn.to));
        connItem->setText(0, label);
        connItem->setText(1, QString::fromStdString(conn.link));
    }
}

void ConfigTreeWidget::addSimulationItem() {
    QTreeWidgetItem* simRoot = new QTreeWidgetItem(tree_);
    simRoot->setText(0, "Simulation Parameters");
    simRoot->setExpanded(true);

    QTreeWidgetItem* epochItem = new QTreeWidgetItem(simRoot);
    epochItem->setText(0, "Epoch Duration (ms)");
    epochItem->setText(1, QString::number(config_.simulation.epoch_ms));

    QTreeWidgetItem* mlpItem = new QTreeWidgetItem(simRoot);
    mlpItem->setText(0, "MLP Optimization");
    mlpItem->setText(1, config_.simulation.enable_mlp_optimization ? "Enabled" : "Disabled");

    QTreeWidgetItem* congItem = new QTreeWidgetItem(simRoot);
    congItem->setText(0, "Congestion Model");
    congItem->setText(1, config_.simulation.enable_congestion_model ? "Enabled" : "Disabled");
}

void ConfigTreeWidget::onItemDoubleClicked(QTreeWidgetItem* item, int column) {
    if (column != 1) {
        return;
    }

    // Allow editing of certain values
    QString property = item->text(0);
    QString currentValue = item->text(1);

    bool ok;
    QString newValue = QInputDialog::getText(
        this,
        "Edit Property",
        property + ":",
        QLineEdit::Normal,
        currentValue,
        &ok
    );

    if (ok && !newValue.isEmpty()) {
        item->setText(1, newValue);
        // TODO: Update config_ based on changes
        emit configChanged(config_);
    }
}

void ConfigTreeWidget::onAddDevice() {
    bool ok;
    QString id = QInputDialog::getText(
        this,
        "Add CXL Device",
        "Device ID:",
        QLineEdit::Normal,
        QString("CXL%1").arg(config_.cxl_devices.size()),
        &ok
    );

    if (ok && !id.isEmpty()) {
        cxlsim::CXLDeviceConfig dev;
        dev.id = id.toStdString();
        dev.type = "Type3";
        dev.capacity_gb = 128;
        dev.bandwidth_gbps = 64.0;
        dev.base_latency_ns = 170.0;

        config_.cxl_devices.push_back(dev);
        populateTree();
        emit configChanged(config_);
    }
}

void ConfigTreeWidget::onAddSwitch() {
    bool ok;
    QString id = QInputDialog::getText(
        this,
        "Add Switch",
        "Switch ID:",
        QLineEdit::Normal,
        QString("SW%1").arg(config_.switches.size()),
        &ok
    );

    if (ok && !id.isEmpty()) {
        cxlsim::SwitchConfig sw;
        sw.id = id.toStdString();
        sw.num_ports = 8;
        sw.latency_ns = 40.0;

        config_.switches.push_back(sw);
        populateTree();
        emit configChanged(config_);
    }
}

void ConfigTreeWidget::onRemoveSelected() {
    QTreeWidgetItem* item = tree_->currentItem();
    if (!item) {
        return;
    }

    // TODO: Implement removal logic
    QMessageBox::information(this, "Remove", "Remove functionality not yet implemented");
}
