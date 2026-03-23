/**
 * @file topology_editor_widget.cpp
 * @brief 可交互式 CXL 拓扑编辑器 - 支持拖拽、连线、自动布局
 */

#include "topology_editor_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QPen>
#include <QBrush>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QToolButton>
#include <QLabel>
#include <cmath>

// ══════════════════════════════════════════════════════════════════════════════
// TopologyEditorWidget - 主编辑器实现
// ══════════════════════════════════════════════════════════════════════════════

TopologyEditorWidget::TopologyEditorWidget(QWidget *parent)
    : QWidget(parent)
    , toolbar_(nullptr)
    , view_(nullptr)
    , scene_(nullptr)
    , connectionSource_(nullptr)
    , connectionPreview_(nullptr)
    , deviceCounter_(0)
    , switchCounter_(0)
{
    setupUI();
    setupToolBar();
}

TopologyEditorWidget::~TopologyEditorWidget() {}

void TopologyEditorWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    scene_ = new QGraphicsScene(this);
    scene_->setSceneRect(-500, -400, 1000, 800);
    scene_->setBackgroundBrush(QColor(0x1A, 0x1A, 0x2E));

    view_ = new QGraphicsView(scene_, this);
    view_->setRenderHint(QPainter::Antialiasing);
    view_->setRenderHint(QPainter::TextAntialiasing);
    view_->setDragMode(QGraphicsView::RubberBandDrag);
    view_->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    view_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    view_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    view_->setStyleSheet(
        "QGraphicsView{border:none;background:#1A1A2E;}"
        "QScrollBar:vertical{background:#16213E;width:10px;}"
        "QScrollBar::handle:vertical{background:#0F3460;border-radius:5px;}"
        "QScrollBar:horizontal{background:#16213E;height:10px;}"
        "QScrollBar::handle:horizontal{background:#0F3460;border-radius:5px;}");

    layout->addWidget(view_);
}

void TopologyEditorWidget::setupToolBar() {
    toolbar_ = new QToolBar(this);
    toolbar_->setMovable(false);
    toolbar_->setIconSize(QSize(20, 20));
    toolbar_->setStyleSheet(
        "QToolBar{background:#16213E;border-bottom:2px solid #0F3460;padding:4px 8px;spacing:6px;}"
        "QToolButton{background:#0F3460;color:#E0E0E0;border:1px solid #4FC3F7;"
        "border-radius:4px;padding:6px 12px;font-size:11px;min-width:60px;}"
        "QToolButton:hover{background:#1A4A7A;border-color:#81D4FA;}"
        "QToolButton:pressed{background:#4FC3F7;color:#1A1A2E;}");

    auto* rcBtn = new QToolButton();
    rcBtn->setText("+ RC");
    rcBtn->setToolTip("添加根复合体 (Root Complex)");
    connect(rcBtn, &QToolButton::clicked, this, &TopologyEditorWidget::onAddRootComplex);
    toolbar_->addWidget(rcBtn);

    auto* swBtn = new QToolButton();
    swBtn->setText("+ 交换机");
    swBtn->setToolTip("添加 CXL 交换机");
    connect(swBtn, &QToolButton::clicked, this, &TopologyEditorWidget::onAddSwitch);
    toolbar_->addWidget(swBtn);

    auto* devBtn = new QToolButton();
    devBtn->setText("+ CXL设备");
    devBtn->setToolTip("添加 CXL 内存设备");
    connect(devBtn, &QToolButton::clicked, this, &TopologyEditorWidget::onAddCXLDevice);
    toolbar_->addWidget(devBtn);

    toolbar_->addSeparator();

    auto* delBtn = new QToolButton();
    delBtn->setText("✕ 删除");
    delBtn->setToolTip("删除选中组件");
    delBtn->setStyleSheet(
        "QToolButton{background:#B71C1C;border-color:#EF9A9A;}"
        "QToolButton:hover{background:#C62828;}");
    connect(delBtn, &QToolButton::clicked, this, &TopologyEditorWidget::onDeleteSelected);
    toolbar_->addWidget(delBtn);

    auto* layoutBtn = new QToolButton();
    layoutBtn->setText("⚡ 自动布局");
    layoutBtn->setToolTip("自动排列组件");
    connect(layoutBtn, &QToolButton::clicked, this, &TopologyEditorWidget::autoLayout);
    toolbar_->addWidget(layoutBtn);

    toolbar_->addSeparator();

    auto* infoLabel = new QLabel("  💡 提示: 按住Ctrl点击设备可建立连接  ", toolbar_);
    infoLabel->setStyleSheet("color:#4FC3F7;font-size:11px;padding:4px;");
    toolbar_->addWidget(infoLabel);

    qobject_cast<QVBoxLayout*>(layout())->insertWidget(0, toolbar_);
}

void TopologyEditorWidget::updateTopology(const cxlsim::CXLSimConfig& config) {
    clearTopology();
    deviceCounter_ = 0;
    switchCounter_ = 0;

    if (!config.root_complex.id.empty()) {
        auto* rc = new ComponentItem(
            QString::fromStdString(config.root_complex.id),
            ComponentItem::RootComplex);
        rc->setPos(0, -220);
        scene_->addItem(rc);
        components_[QString::fromStdString(config.root_complex.id)] = rc;
    }

    int swIdx = 0;
    for (const auto& sw : config.switches) {
        auto* item = new ComponentItem(
            QString::fromStdString(sw.id),
            ComponentItem::Switch);
        item->setPos(-200 + swIdx * 200, 0);
        scene_->addItem(item);
        components_[QString::fromStdString(sw.id)] = item;
        swIdx++;
        switchCounter_++;
    }

    int devIdx = 0;
    for (const auto& dev : config.cxl_devices) {
        auto* item = new ComponentItem(
            QString::fromStdString(dev.id),
            ComponentItem::CXLDevice);
        item->setPos(-250 + devIdx * 150, 220);
        scene_->addItem(item);
        components_[QString::fromStdString(dev.id)] = item;
        devIdx++;
        deviceCounter_++;
    }

    for (const auto& conn : config.connections) {
        QString fromId = QString::fromStdString(conn.from);
        QString toId = QString::fromStdString(conn.to);

        if (components_.contains(fromId) && components_.contains(toId)) {
            auto* link = new LinkItem(components_[fromId], components_[toId]);
            scene_->addItem(link);
            links_.append(link);
            components_[fromId]->addLink(link);
            components_[toId]->addLink(link);
        }
    }
}

void TopologyEditorWidget::onAddRootComplex() {
    if (components_.contains("RC0")) {
        QMessageBox::warning(this, "提示", "根复合体已存在，每个拓扑只能有一个RC");
        return;
    }
    bool ok;
    QString id = QInputDialog::getText(this, "添加根复合体", "ID:", 
                                       QLineEdit::Normal, "RC0", &ok);
    if (ok && !id.isEmpty()) {
        addComponent(ComponentItem::RootComplex, QPointF(0, -220));
        components_[id]->setId(id);
        emit topologyModified();
    }
}

void TopologyEditorWidget::onAddSwitch() {
    QString id = QString("SW%1").arg(switchCounter_++);
    addComponent(ComponentItem::Switch, QPointF(-100 + switchCounter_ * 80, 0));
    emit topologyModified();
}

void TopologyEditorWidget::onAddCXLDevice() {
    QString id = QString("CXL_MEM%1").arg(deviceCounter_++);
    addComponent(ComponentItem::CXLDevice, QPointF(-150 + deviceCounter_ * 100, 220));
    emit topologyModified();
}

void TopologyEditorWidget::addComponent(ComponentItem::ComponentType type, const QPointF& pos) {
    QString id;
    switch (type) {
        case ComponentItem::RootComplex: id = "RC0"; break;
        case ComponentItem::Switch:       id = QString("SW%1").arg(switchCounter_); break;
        case ComponentItem::CXLDevice:    id = QString("CXL_MEM%1").arg(deviceCounter_); break;
    }
    
    auto* item = new ComponentItem(id, type);
    item->setPos(pos);
    scene_->addItem(item);
    components_[id] = item;
}

void TopologyEditorWidget::onDeleteSelected() {
    auto selected = scene_->selectedItems();
    if (selected.isEmpty()) return;

    for (auto* item : selected) {
        if (auto* comp = dynamic_cast<ComponentItem*>(item)) {
            for (auto* link : comp->links()) {
                links_.removeOne(link);
                scene_->removeItem(link);
                delete link;
            }
            components_.remove(comp->id());
            scene_->removeItem(comp);
            delete comp;
        } else if (auto* link = dynamic_cast<LinkItem*>(item)) {
            links_.removeOne(link);
            scene_->removeItem(link);
            delete link;
        }
    }
    emit topologyModified();
}

void TopologyEditorWidget::autoLayout() {
    QList<ComponentItem*> rcs, switches, devices;
    for (auto* comp : components_) {
        switch (comp->componentType()) {
            case ComponentItem::RootComplex: rcs.append(comp); break;
            case ComponentItem::Switch:      switches.append(comp); break;
            case ComponentItem::CXLDevice:   devices.append(comp); break;
        }
    }

    for (int i = 0; i < rcs.size(); ++i)
        rcs[i]->setPos(-100 * (rcs.size() - 1) + i * 200, -220);
    
    for (int i = 0; i < switches.size(); ++i)
        switches[i]->setPos(-150 * (switches.size() - 1) / 2 + i * 150, 0);
    
    for (int i = 0; i < devices.size(); ++i)
        devices[i]->setPos(-120 * (devices.size() - 1) / 2 + i * 120, 220);
    
    for (auto* link : links_)
        link->updatePosition();
}

cxlsim::CXLSimConfig TopologyEditorWidget::getCurrentConfig() const {
    cxlsim::CXLSimConfig config;
    config.name = "GUI_Generated_Topology";
    
    for (auto* comp : components_) {
        if (comp->componentType() == ComponentItem::RootComplex) {
            config.root_complex.id = comp->id().toStdString();
            config.root_complex.local_dram_size_gb = 64;
        } else if (comp->componentType() == ComponentItem::Switch) {
            cxlsim::SwitchConfig sw;
            sw.id = comp->id().toStdString();
            sw.num_ports = 8;
            sw.latency_ns = 40.0;
            config.switches.push_back(sw);
        } else if (comp->componentType() == ComponentItem::CXLDevice) {
            cxlsim::CXLDeviceConfig dev;
            dev.id = comp->id().toStdString();
            dev.type = "Type3";
            dev.capacity_gb = 128;
            dev.bandwidth_gbps = 64.0;
            dev.base_latency_ns = 170.0;
            config.cxl_devices.push_back(dev);
        }
    }
    
    for (auto* link : links_) {
        cxlsim::ConnectionConfig conn;
        conn.from = link->fromComponent()->id().toStdString();
        conn.to = link->toComponent()->id().toStdString();
        conn.link = "PCIe_Gen5";
        config.connections.push_back(conn);
    }
    
    return config;
}

void TopologyEditorWidget::clearTopology() {
    scene_->clear();
    components_.clear();
    links_.clear();
    connectionSource_ = nullptr;
    if (connectionPreview_) {
        scene_->removeItem(connectionPreview_);
        delete connectionPreview_;
        connectionPreview_ = nullptr;
    }
}

void TopologyEditorWidget::exportToImage(const QString& filename) {
    scene_->clearSelection();
    QRectF rect = scene_->itemsBoundingRect();
    rect.adjust(-50, -50, 50, 50);
    QImage image(rect.size().toSize(), QImage::Format_ARGB32);
    image.fill(QColor(0x1A, 0x1A, 0x2E));
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    scene_->render(&painter, QRectF(), rect);
    image.save(filename);
}

void TopologyEditorWidget::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);
    menu.setStyleSheet("QMenu{background:#16213E;color:#E0E0E0;border:1px solid #0F3460;}"
                       "QMenu::item:selected{background:#0F3460;}");
    
    menu.addAction("添加 Root Complex", this, &TopologyEditorWidget::onAddRootComplex);
    menu.addAction("添加交换机", this, &TopologyEditorWidget::onAddSwitch);
    menu.addAction("添加 CXL 设备", this, &TopologyEditorWidget::onAddCXLDevice);
    menu.addSeparator();
    
    menu.addAction("导出拓扑图...", [this]() {
        QString fn = QFileDialog::getSaveFileName(this, "导出拓扑图", "topology.png", 
                                                   "图片 (*.png *.jpg)");
        if (!fn.isEmpty()) exportToImage(fn);
    });
    
    menu.addSeparator();
    auto* delAction = menu.addAction("删除选中", this, &TopologyEditorWidget::onDeleteSelected);
    delAction->setEnabled(!scene_->selectedItems().isEmpty());
    
    menu.exec(event->globalPos());
}

void TopologyEditorWidget::onComponentMoved() {
    for (auto* link : links_)
        link->updatePosition();
}

void TopologyEditorWidget::startConnection(ComponentItem* from) {
    connectionSource_ = from;
    connectionPreview_ = new QGraphicsLineItem();
    connectionPreview_->setPen(QPen(QColor(0x4F, 0xC3, 0xF7), 2, Qt::DashLine));
    scene_->addItem(connectionPreview_);
}

void TopologyEditorWidget::finishConnection(ComponentItem* to) {
    if (!connectionSource_ || connectionSource_ == to) {
        if (connectionPreview_) {
            scene_->removeItem(connectionPreview_);
            delete connectionPreview_;
            connectionPreview_ = nullptr;
        }
        connectionSource_ = nullptr;
        return;
    }
    
    auto* link = new LinkItem(connectionSource_, to);
    link->setBandwidth(64.0);
    link->setLatency(40.0);
    scene_->addItem(link);
    links_.append(link);
    connectionSource_->addLink(link);
    to->addLink(link);
    
    if (connectionPreview_) {
        scene_->removeItem(connectionPreview_);
        delete connectionPreview_;
        connectionPreview_ = nullptr;
    }
    connectionSource_ = nullptr;
    emit topologyModified();
}

// ══════════════════════════════════════════════════════════════════════════════
// ComponentItem - 设备图形项实现
// ══════════════════════════════════════════════════════════════════════════════

ComponentItem::ComponentItem(const QString& id, ComponentType type, QGraphicsItem* parent)
    : QGraphicsItem(parent), id_(id), type_(type), highlighted_(false)
{
    setFlag(ItemIsMovable);
    setFlag(ItemIsSelectable);
    setFlag(ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
}

QRectF ComponentItem::boundingRect() const {
    return QRectF(-55, -32, 110, 64);
}

void ComponentItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option); Q_UNUSED(widget);

    QColor fillColor, borderColor;
    QString label;

    switch (type_) {
        case RootComplex:
            fillColor = QColor(0x64, 0xB5, 0xF6);
            borderColor = QColor(0x42, 0xA5, 0xF5);
            label = "RC";
            break;
        case Switch:
            fillColor = QColor(0x81, 0xC7, 0x84);
            borderColor = QColor(0x66, 0xBB, 0x6A);
            label = "SW";
            break;
        case CXLDevice:
            fillColor = QColor(0xFF, 0xB7, 0x4D);
            borderColor = QColor(0xFF, 0xA7, 0x26);
            label = "CXL";
            break;
    }

    if (highlighted_ || isSelected()) {
        fillColor = fillColor.lighter(115);
        borderColor = borderColor.lighter(130);
    }

    QRectF rect = boundingRect();
    painter->setPen(QPen(borderColor, 2));
    painter->setBrush(QLinearGradient(rect.topLeft(), rect.bottomLeft()));
    auto grad = QLinearGradient(rect.topLeft(), rect.bottomLeft());
    grad.setColorAt(0.0, fillColor.lighter(110));
    grad.setColorAt(1.0, fillColor.darker(110));
    painter->setBrush(grad);
    painter->drawRoundedRect(rect, 8, 8);

    painter->setPen(QColor(0x1A, 0x1A, 0x2E));
    QFont font("Sans", 9, QFont::Bold);
    painter->setFont(font);
    painter->drawText(QRectF(rect.left(), rect.top(), rect.width(), rect.height() / 2),
                      Qt::AlignCenter, label);

    font.setBold(false);
    font.setPointSize(8);
    painter->setFont(font);
    painter->drawText(QRectF(rect.left(), rect.top() + rect.height() / 2, rect.width(), rect.height() / 2),
                      Qt::AlignCenter, id_);
}

void ComponentItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        auto* editor = dynamic_cast<TopologyEditorWidget*>(scene()->parent());
        if (editor) editor->startConnection(this);
        event->accept();
        return;
    }
    dragStartPos_ = pos();
    QGraphicsItem::mousePressEvent(event);
}

void ComponentItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    QGraphicsItem::mouseMoveEvent(event);
    for (auto* link : links_)
        link->updatePosition();
}

void ComponentItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        auto* editor = dynamic_cast<TopologyEditorWidget*>(scene()->parent());
        if (editor) editor->finishConnection(this);
        event->accept();
        return;
    }
    QGraphicsItem::mouseReleaseEvent(event);
}

QVariant ComponentItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == ItemPositionHasChanged) {
        for (auto* link : links_)
            link->updatePosition();
    }
    return QGraphicsItem::itemChange(change, value);
}

// ══════════════════════════════════════════════════════════════════════════════
// LinkItem - 连接线图形项实现
// ══════════════════════════════════════════════════════════════════════════════

LinkItem::LinkItem(ComponentItem* from, ComponentItem* to, QGraphicsItem* parent)
    : QGraphicsItem(parent), from_(from), to_(to), bandwidth_gbps_(64.0), latency_ns_(40.0)
{
    setZValue(-1);
    setFlag(ItemIsSelectable);
}

QRectF LinkItem::boundingRect() const {
    QPointF p1 = from_->pos();
    QPointF p2 = to_->pos();
    qreal minX = std::min(p1.x(), p2.x()) - 20;
    qreal minY = std::min(p1.y(), p2.y()) - 20;
    qreal maxX = std::max(p1.x(), p2.x()) + 20;
    qreal maxY = std::max(p1.y(), p2.y()) + 20;
    return QRectF(minX, minY, maxX - minX, maxY - minY);
}

void LinkItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option); Q_UNUSED(widget);

    QPointF p1 = from_->pos();
    QPointF p2 = to_->pos();
    
    QColor lineColor = isSelected() ? QColor(0x4F, 0xC3, 0xF7) : QColor(0x5A, 0x5A, 0x6E);
    painter->setPen(QPen(lineColor, 2, Qt::SolidLine));
    painter->drawLine(p1, p2);

    QLineF line(p1, p2);
    double angle = std::atan2(-line.dy(), line.dx());
    QPointF arrowP1 = p2 - QPointF(sin(angle + M_PI / 3) * 12,
                                    cos(angle + M_PI / 3) * 12);
    QPointF arrowP2 = p2 - QPointF(sin(angle + M_PI - M_PI / 3) * 12,
                                    cos(angle + M_PI - M_PI / 3) * 12);
    QPolygonF arrowHead;
    arrowHead << p2 << arrowP1 << arrowP2;
    painter->setBrush(lineColor);
    painter->drawPolygon(arrowHead);

    if (bandwidth_gbps_ > 0) {
        QPointF midPoint = (p1 + p2) / 2;
        QString label = QString("%1GB/s\n%2ns")
            .arg(bandwidth_gbps_, 0, 'f', 0)
            .arg(latency_ns_, 0, 'f', 0);
        painter->setPen(QColor(0xE0, 0xE0, 0xE0));
        QFont font("Sans", 7);
        painter->setFont(font);
        
        QRectF labelRect(-25, -15, 50, 30);
        labelRect.moveCenter(midPoint);
        painter->fillRect(labelRect, QColor(0x16, 0x21, 0x3E, 200));
        painter->drawText(labelRect, Qt::AlignCenter, label);
    }
}

void LinkItem::updatePosition() {
    prepareGeometryChange();
}
