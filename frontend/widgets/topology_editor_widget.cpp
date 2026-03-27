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
    , connectionMode_(false)
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
    rcBtn->setText("➕ 根复合体");
    rcBtn->setToolTip("添加根复合体");
    connect(rcBtn, &QToolButton::clicked, this, &TopologyEditorWidget::onAddRootComplex);
    toolbar_->addWidget(rcBtn);

    auto* swBtn = new QToolButton();
    swBtn->setText("➕ CXL交换机");
    swBtn->setToolTip("添加CXL交换机");
    connect(swBtn, &QToolButton::clicked, this, &TopologyEditorWidget::onAddSwitch);
    toolbar_->addWidget(swBtn);

    auto* devBtn = new QToolButton();
    devBtn->setText("➕ CXL设备");
    devBtn->setToolTip("添加CXL内存设备");
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

    // 连接模式切换按钮
    auto* connectModeBtn = new QToolButton();
    connectModeBtn->setText("🔗 连接模式");
    connectModeBtn->setCheckable(true);
    connectModeBtn->setToolTip("开启后点击两个节点即可连接");
    connect(connectModeBtn, &QToolButton::toggled, this, [this](bool checked) {
        connectionMode_ = checked;
        if (!checked) {
            connectionSource_ = nullptr;
            scene_->update();
        }
    });
    toolbar_->addWidget(connectModeBtn);

    auto* infoLabel = new QLabel("  💡 点击'连接模式'后，依次点击两个节点即可连接  ", toolbar_);
    infoLabel->setStyleSheet("color: #888888; font-size: 12px; font-weight: 500;");
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
            ComponentItem::RootComplex,
            this);
        rc->setPos(0, -220);
        scene_->addItem(rc);
        components_[QString::fromStdString(config.root_complex.id)] = rc;
    }

    int swIdx = 0;
    for (const auto& sw : config.switches) {
        auto* item = new ComponentItem(
            QString::fromStdString(sw.id),
            ComponentItem::Switch,
            this);
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
            ComponentItem::CXLDevice,
            this);
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
    
    auto* item = new ComponentItem(id, type, this);
    item->setPos(pos);
    item->setFlag(QGraphicsItem::ItemIsMovable);
    item->setFlag(QGraphicsItem::ItemIsSelectable);
    scene_->addItem(item);
    components_[id] = item;
}

void TopologyEditorWidget::onDeleteSelected() {
    auto selected = scene_->selectedItems();
    if (selected.isEmpty()) return;

    QList<ComponentItem*> compsToDelete;
    QList<LinkItem*> linksToDelete;

    // 收集要删除的项
    for (auto* item : selected) {
        if (auto* comp = dynamic_cast<ComponentItem*>(item)) {
            compsToDelete.append(comp);
        } else if (auto* link = dynamic_cast<LinkItem*>(item)) {
            linksToDelete.append(link);
        }
    }

    // 先删除组件关联的所有链接
    for (auto* comp : compsToDelete) {
        // 复制链接列表，避免在遍历时修改
        QList<LinkItem*> compLinks = comp->links();
        for (auto* link : compLinks) {
            // 从两端组件移除链接引用
            if (link->fromComponent()) {
                link->fromComponent()->removeLink(link);
            }
            if (link->toComponent()) {
                link->toComponent()->removeLink(link);
            }
            // 从场景和列表中移除
            links_.removeOne(link);
            scene_->removeItem(link);
            delete link;
        }
    }

    // 删除单独选中的链接
    for (auto* link : linksToDelete) {
        if (links_.contains(link)) {
            // 从两端组件移除链接引用
            if (link->fromComponent()) {
                link->fromComponent()->removeLink(link);
            }
            if (link->toComponent()) {
                link->toComponent()->removeLink(link);
            }
            links_.removeOne(link);
            scene_->removeItem(link);
            delete link;
        }
    }

    // 最后删除组件
    for (auto* comp : compsToDelete) {
        components_.remove(comp->id());
        scene_->removeItem(comp);
        delete comp;
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

    // 改进的布局算法：更好的间距和对齐
    const int RC_Y = -250;
    const int SW_Y = 0;
    const int DEV_Y = 250;
    const int RC_SPACING = 220;
    const int SW_SPACING = 180;
    const int DEV_SPACING = 160;

    // RC层居中
    if (!rcs.isEmpty()) {
        double totalWidth = (rcs.size() - 1) * RC_SPACING;
        double startX = -totalWidth / 2.0;
        for (int i = 0; i < rcs.size(); ++i) {
            rcs[i]->setPos(startX + i * RC_SPACING, RC_Y);
        }
    }
    
    // 交换机层居中
    if (!switches.isEmpty()) {
        double totalWidth = (switches.size() - 1) * SW_SPACING;
        double startX = -totalWidth / 2.0;
        for (int i = 0; i < switches.size(); ++i) {
            switches[i]->setPos(startX + i * SW_SPACING, SW_Y);
        }
    }
    
    // 设备层居中
    if (!devices.isEmpty()) {
        double totalWidth = (devices.size() - 1) * DEV_SPACING;
        double startX = -totalWidth / 2.0;
        for (int i = 0; i < devices.size(); ++i) {
            devices[i]->setPos(startX + i * DEV_SPACING, DEV_Y);
        }
    }
    
    // 更新所有连接线
    for (auto* link : links_)
        link->updatePosition();
    
    // 调整视图以适应所有内容
    view_->fitInView(scene_->itemsBoundingRect().adjusted(-100, -100, 100, 100), Qt::KeepAspectRatio);
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
    connectionSource_->setHighlight(true);
    connectionPreview_ = new QGraphicsLineItem();
    connectionPreview_->setPen(QPen(QColor(0x4F, 0xC3, 0xF7), 3, Qt::DashLine));
    connectionPreview_->setZValue(100);
    scene_->addItem(connectionPreview_);
}

void TopologyEditorWidget::finishConnection(ComponentItem* to) {
    if (connectionPreview_) {
        scene_->removeItem(connectionPreview_);
        delete connectionPreview_;
        connectionPreview_ = nullptr;
    }
    
    if (!connectionSource_ || connectionSource_ == to) {
        if (connectionSource_) {
            connectionSource_->setHighlight(false);
        }
        connectionSource_ = nullptr;
        return;
    }
    
    // 检查是否已存在连接
    for (auto* link : links_) {
        if ((link->fromComponent() == connectionSource_ && link->toComponent() == to) ||
            (link->fromComponent() == to && link->toComponent() == connectionSource_)) {
            connectionSource_->setHighlight(false);
            connectionSource_ = nullptr;
            return; // 已存在连接，忽略
        }
    }
    
    auto* link = new LinkItem(connectionSource_, to);
    link->setBandwidth(64.0);
    link->setLatency(40.0);
    scene_->addItem(link);
    links_.append(link);
    connectionSource_->addLink(link);
    to->addLink(link);
    
    connectionSource_->setHighlight(false);
    connectionSource_ = nullptr;
    emit topologyModified();
}

// ══════════════════════════════════════════════════════════════════════════════
// ComponentItem - 设备图形项实现
// ══════════════════════════════════════════════════════════════════════════════

ComponentItem::ComponentItem(const QString& id, ComponentType type, TopologyEditorWidget* editor, QGraphicsItem* parent)
    : QGraphicsItem(parent), id_(id), type_(type), highlighted_(false), editor_(editor)
{
    setFlag(ItemIsMovable);
    setFlag(ItemIsSelectable);
    setFlag(ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
}

QRectF ComponentItem::boundingRect() const {
    // 显著放大组件尺寸，从 110x64 增加到 180x100，便于查看和交互
    return QRectF(-90, -50, 180, 100);
}

void ComponentItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option); Q_UNUSED(widget);

    QColor baseColor, accentColor;
    QString label;

    // 采用更克制的色彩系统
    switch (type_) {
        case RootComplex:
            baseColor = QColor(0x1E, 0x3A, 0x8A); // 深邃蓝
            accentColor = QColor(0x3B, 0x82, 0xF6); // 强调蓝
            label = "RC";
            break;
        case Switch:
            baseColor = QColor(0x14, 0x53, 0x2D); // 深邃绿
            accentColor = QColor(0x22, 0xC5, 0x5E); // 强调绿
            label = "SW";
            break;
        case CXLDevice:
            baseColor = QColor(0x7C, 0x2D, 0x12); // 深邃橙
            accentColor = QColor(0xF9, 0x73, 0x16); // 强调橙
            label = "CXL";
            break;
    }

    bool isActive = highlighted_ || isSelected();
    QRectF rect = boundingRect();
    
    // 渲染外层光晕 (发光阴影特效)
    if (isActive) {
        painter->setPen(Qt::NoPen);
        for (int i = 1; i <= 6; ++i) {
            painter->setBrush(QColor(accentColor.red(), accentColor.green(), accentColor.blue(), 60 / i));
            painter->drawRoundedRect(rect.adjusted(-i*2, -i*2, i*2, i*2), 12, 12);
        }
    } else {
        // 常规微弱的投影
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 0, 0, 100));
        painter->drawRoundedRect(rect.adjusted(4, 6, 4, 6), 10, 10);
    }
    
    // 绘制主体卡片背景 (暗黑质感渐变)
    auto bgGrad = QLinearGradient(rect.topLeft(), rect.bottomLeft());
    bgGrad.setColorAt(0.0, QColor(0x1A, 0x1A, 0x1A));
    bgGrad.setColorAt(1.0, QColor(0x0A, 0x0A, 0x0A));
    painter->setBrush(bgGrad);
    
    // 绘制边框
    if (isActive) {
        painter->setPen(QPen(accentColor, 2.0));
    } else {
        painter->setPen(QPen(QColor(0x44, 0x44, 0x44), 1)); // 细腻的细边框
    }
    painter->drawRoundedRect(rect, 10, 10);

    // 绘制顶部彩色装饰条 (Vercel风格特色)
    painter->setPen(Qt::NoPen);
    painter->setBrush(isActive ? accentColor : baseColor);
    // 顶边圆角，底边直角
    QPainterPath topBar;
    topBar.moveTo(rect.left(), rect.top() + 12);
    topBar.arcTo(rect.left(), rect.top(), 20, 20, 180, -90);
    topBar.arcTo(rect.right() - 20, rect.top(), 20, 20, 90, -90);
    topBar.lineTo(rect.right(), rect.top() + 6);
    topBar.lineTo(rect.left(), rect.top() + 6);
    painter->drawPath(topBar);

    // 绘制主标签
    painter->setPen(QColor(0xED, 0xED, 0xED));
    QFont font("Inter, -apple-system, sans-serif", 14, QFont::Bold);
    painter->setFont(font);
    painter->drawText(QRectF(rect.left(), rect.top() + 8, rect.width(), rect.height() / 2),
                      Qt::AlignCenter, label);

    // 绘制ID (次级文本)
    painter->setPen(QColor(0x88, 0x88, 0x88));
    font.setBold(false);
    font.setPointSize(12);
    painter->setFont(font);
    painter->drawText(QRectF(rect.left(), rect.top() + rect.height() / 2, rect.width(), rect.height() / 2 - 8),
                      Qt::AlignCenter, id_);
}

void ComponentItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton && editor_ && editor_->isConnectionMode()) {
        // 连接模式下，左键点击处理连接
        if (!editor_->getConnectionSource()) {
            editor_->startConnection(this);
        } else {
            editor_->finishConnection(this);
        }
        event->accept();
        return;
    }
    dragStartPos_ = pos();
    QGraphicsItem::mousePressEvent(event);
}

void ComponentItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    // 在连接模式下显示预览线（无需Ctrl检查）
    if (editor_ && editor_->property("connectionSource").value<ComponentItem*>()) {
        event->accept();
        return;
    }
    QGraphicsItem::mouseMoveEvent(event);
    for (auto* link : links_)
        link->updatePosition();
}

void ComponentItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
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
    
    // 基础线条绘制
    bool active = isSelected() || (from_->isSelected() || to_->isSelected());
    QColor lineColor = active ? QColor(0x60, 0xA5, 0xFA) : QColor(0x33, 0x33, 0x33);
    painter->setPen(QPen(lineColor, active ? 3 : 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->drawLine(p1, p2);

    // 绘制箭头
    QLineF line(p1, p2);
    // 将箭头退后一点，避免挡住组件
    QPointF arrowBase = p2 - QPointF(line.dx(), line.dy()) * (40.0 / line.length());
    
    double angle = std::atan2(-line.dy(), line.dx());
    QPointF arrowP1 = arrowBase - QPointF(sin(angle + M_PI / 3) * 10,
                                          cos(angle + M_PI / 3) * 10);
    QPointF arrowP2 = arrowBase - QPointF(sin(angle + M_PI - M_PI / 3) * 10,
                                          cos(angle + M_PI - M_PI / 3) * 10);
    QPolygonF arrowHead;
    arrowHead << arrowBase << arrowP1 << arrowP2;
    painter->setPen(Qt::NoPen);
    painter->setBrush(lineColor);
    painter->drawPolygon(arrowHead);

    // 悬浮式胶囊标签 (Pill Badge)
    if (bandwidth_gbps_ > 0) {
        QPointF midPoint = (p1 + p2) / 2;
        QString label = QString("%1 GB/s • %2 ns")
            .arg(bandwidth_gbps_, 0, 'f', 0)
            .arg(latency_ns_, 0, 'f', 0);
        
        QFont font("JetBrains Mono, Consolas", 8);
        painter->setFont(font);
        
        QFontMetrics fm(font);
        int textWidth = fm.horizontalAdvance(label);
        int textHeight = fm.height();
        
        // 标签边框
        QRectF labelRect(-textWidth/2 - 8, -textHeight/2 - 4, textWidth + 16, textHeight + 8);
        labelRect.moveCenter(midPoint);
        
        // 绘制毛玻璃质感底色
        painter->setPen(QPen(QColor(0x33, 0x33, 0x33), 1));
        painter->setBrush(QColor(0x0A, 0x0A, 0x0A, 220)); // 半透明黑
        painter->drawRoundedRect(labelRect, labelRect.height()/2, labelRect.height()/2);
        
        // 绘制文本
        painter->setPen(active ? QColor(0x60, 0xA5, 0xFA) : QColor(0x88, 0x88, 0x88));
        painter->drawText(labelRect, Qt::AlignCenter, label);
    }
}

void LinkItem::updatePosition() {
    prepareGeometryChange();
}
