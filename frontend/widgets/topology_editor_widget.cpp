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
#include <QKeyEvent>
#include <cmath>

// ══════════════════════════════════════════════════════════════════════════════
// ZoomableGraphicsView
// ══════════════════════════════════════════════════════════════════════════════

ZoomableGraphicsView::ZoomableGraphicsView(QGraphicsScene* scene, QWidget* parent)
    : QGraphicsView(scene, parent), zoomFactor_(1.0)
{
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    setDragMode(QGraphicsView::RubberBandDrag);
}

void ZoomableGraphicsView::wheelEvent(QWheelEvent* event) {
    const double scaleFactor = 1.15;
    if (event->angleDelta().y() > 0) {
        if (zoomFactor_ < 5.0) {
            scale(scaleFactor, scaleFactor);
            zoomFactor_ *= scaleFactor;
        }
    } else {
        if (zoomFactor_ > 0.1) {
            scale(1.0 / scaleFactor, 1.0 / scaleFactor);
            zoomFactor_ /= scaleFactor;
        }
    }
    event->accept();
}

void ZoomableGraphicsView::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Space) {
        // 按空格切换拖拽模式
        if (dragMode() == QGraphicsView::RubberBandDrag) {
            setDragMode(QGraphicsView::ScrollHandDrag);
            setCursor(Qt::OpenHandCursor);
        } else {
            setDragMode(QGraphicsView::RubberBandDrag);
            setCursor(Qt::ArrowCursor);
        }
        event->accept();
    } else if (event->key() == Qt::Key_0 && event->modifiers() & Qt::ControlModifier) {
        // Ctrl+0 复位缩放
        resetTransform();
        zoomFactor_ = 1.0;
        event->accept();
    } else {
        QGraphicsView::keyPressEvent(event);
    }
}

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
    , zoomLabel_(nullptr)
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
    scene_->setSceneRect(-800, -600, 1600, 1200);
    scene_->setBackgroundBrush(QColor(0x0A, 0x0A, 0x0A));

    view_ = new ZoomableGraphicsView(scene_, this);
    view_->setRenderHint(QPainter::Antialiasing);
    view_->setRenderHint(QPainter::TextAntialiasing);
    view_->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    view_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    view_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    view_->setFocusPolicy(Qt::StrongFocus);
    view_->setStyleSheet(
        "QGraphicsView{border:none;background:#0A0A0A;}"
        "QScrollBar:vertical{background:#111111;width:8px;}"
        "QScrollBar::handle:vertical{background:#333333;border-radius:4px;min-height:20px;}"
        "QScrollBar:horizontal{background:#111111;height:8px;}"
        "QScrollBar::handle:horizontal{background:#333333;border-radius:4px;min-width:20px;}");

    layout->addWidget(view_);
}

void TopologyEditorWidget::updateDeviceMetrics(const QString& deviceId, const DeviceMetrics& m) {
    if (components_.contains(deviceId)) {
        components_[deviceId]->setMetrics(m);
    }
}

void TopologyEditorWidget::updateLinkUtilization(const QString& fromId, const QString& toId, double utilizPct) {
    // 查找连接 fromId -> toId 的链路
    for (auto* link : links_) {
        if (link->fromComponent()->id() == fromId && link->toComponent()->id() == toId) {
            link->setUtilization(utilizPct);
            return;
        }
        // 也检查反向连接（双向链路）
        if (link->fromComponent()->id() == toId && link->toComponent()->id() == fromId) {
            link->setUtilization(utilizPct);
            return;
        }
    }
}

void TopologyEditorWidget::clearAllMetrics() {
    DeviceMetrics empty;
    for (auto* comp : components_)
        comp->setMetrics(empty);
    // 清除链路利用率
    for (auto* link : links_)
        link->setUtilization(0.0);
}

void TopologyEditorWidget::setZoomLevel(double factor) {
    view_->resetTransform();
    view_->scale(factor, factor);
}

double TopologyEditorWidget::zoomLevel() const {
    return view_->transform().m11();
}

void TopologyEditorWidget::setupToolBar() {
    toolbar_ = new QToolBar(this);
    toolbar_->setMovable(false);
    toolbar_->setIconSize(QSize(16, 16));

    // ── 统一工具栏底色与全局主题一致 ──────────────────────────────────────────
    toolbar_->setStyleSheet(
        "QToolBar {"
        "  background: #0D0D0D;"
        "  border-bottom: 1px solid #1E1E1E;"
        "  padding: 3px 10px;"
        "  spacing: 4px;"
        "}"
        "QToolBar::separator {"
        "  background: #2A2A2A;"
        "  width: 1px;"
        "  margin: 5px 8px;"
        "}"
        // 默认编辑类按钮：低调暗蓝灰
        "QToolButton {"
        "  background: #161616;"
        "  color: #AAAAAA;"
        "  border: 1px solid #2A2A2A;"
        "  border-radius: 5px;"
        "  padding: 5px 11px;"
        "  font-size: 11px;"
        "  min-width: 56px;"
        "}"
        "QToolButton:hover {"
        "  background: #1E1E1E;"
        "  color: #EDEDED;"
        "  border-color: #3A3A3A;"
        "}"
        "QToolButton:pressed { background: #252525; }"
        "QToolButton:checked {"
        "  background: #0D2035;"
        "  color: #4FC3F7;"
        "  border-color: #1E4D6B;"
        "}");

    // ═══════════════════════════════════════════════════════════
    // 分组一：拓扑编辑操作
    // ═══════════════════════════════════════════════════════════
    auto* editLabel = new QLabel("  拓扑编辑  ", this);
    editLabel->setStyleSheet(
        "color:#444444; font-size:10px; font-weight:700; "
        "letter-spacing:1px; background:transparent; padding:0 4px;");
    toolbar_->addWidget(editLabel);

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

    auto* delBtn = new QToolButton();
    delBtn->setText("删除");
    delBtn->setToolTip("删除选中的组件 (Del)");
    delBtn->setStyleSheet(
        "QToolButton{"
        "  background:#1A0A0A; color:#F87171;"
        "  border:1px solid #3A1A1A; border-radius:5px;"
        "  padding:5px 11px; font-size:11px; min-width:44px;"
        "}"
        "QToolButton:hover{ background:#2A1010; border-color:#DC2626; color:#FCA5A5; }");
    connect(delBtn, &QToolButton::clicked, this, &TopologyEditorWidget::onDeleteSelected);
    toolbar_->addWidget(delBtn);

    auto* layoutBtn = new QToolButton();
    layoutBtn->setText("自动布局");
    layoutBtn->setToolTip("自动排列所有组件");
    connect(layoutBtn, &QToolButton::clicked, this, &TopologyEditorWidget::autoLayout);
    toolbar_->addWidget(layoutBtn);

    auto* connectModeBtn = new QToolButton();
    connectModeBtn->setText("连接模式");
    connectModeBtn->setCheckable(true);
    connectModeBtn->setToolTip("开启后：点击起点节点，再点击终点节点，即可添加链路");
    connect(connectModeBtn, &QToolButton::toggled, this, [this](bool checked) {
        connectionMode_ = checked;
        if (!checked) {
            connectionSource_ = nullptr;
            scene_->update();
        }
    });
    toolbar_->addWidget(connectModeBtn);

    toolbar_->addSeparator();

    // ═══════════════════════════════════════════════════════════
    // 分组二：模拟控制
    // ═══════════════════════════════════════════════════════════
    auto* simLabel = new QLabel("  模拟控制  ", this);
    simLabel->setStyleSheet(
        "color:#444444; font-size:10px; font-weight:700; "
        "letter-spacing:1px; background:transparent; padding:0 4px;");
    toolbar_->addWidget(simLabel);

    auto* startBtn = new QToolButton();
    startBtn->setText("▶  开始模拟");
    startBtn->setToolTip("启动 CXL 内存模拟  [F5]");
    startBtn->setStyleSheet(
        "QToolButton{"
        "  background:#0A2E1C; color:#6EE7B7;"
        "  border:1px solid #166534; border-radius:5px;"
        "  padding:5px 14px; font-size:11px; font-weight:600; min-width:84px;"
        "}"
        "QToolButton:hover{ background:#0F3D25; border-color:#22C55E; color:#86EFAC; }");
    connect(startBtn, &QToolButton::clicked, this, &TopologyEditorWidget::startSimulationRequested);
    toolbar_->addWidget(startBtn);

    auto* stopBtn = new QToolButton();
    stopBtn->setText("■  停止");
    stopBtn->setToolTip("停止模拟  [F6]");
    stopBtn->setStyleSheet(
        "QToolButton{"
        "  background:#1A0A0A; color:#FCA5A5;"
        "  border:1px solid #7F1D1D; border-radius:5px;"
        "  padding:5px 11px; font-size:11px; font-weight:600; min-width:60px;"
        "}"
        "QToolButton:hover{ background:#2A0F0F; border-color:#DC2626; }");
    connect(stopBtn, &QToolButton::clicked, this, &TopologyEditorWidget::stopSimulationRequested);
    toolbar_->addWidget(stopBtn);

    auto* resetBtn = new QToolButton();
    resetBtn->setText("↺  重置");
    resetBtn->setToolTip("重置模拟统计数据");
    connect(resetBtn, &QToolButton::clicked, this, &TopologyEditorWidget::resetSimulationRequested);
    toolbar_->addWidget(resetBtn);

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
    // 若有实时指标，向下扩展26px
    double extraH = metrics_.active ? 26.0 : 0.0;
    return QRectF(-90, -50, 180, 100 + extraH);
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
    // 卡片主体固定100px高度；boundingRect()在有指标时会额外高26px
    QRectF cardRect(-90, -50, 180, 100);
    
    // 渲染外层光晕 (发光阴影特效)
    if (isActive) {
        painter->setPen(Qt::NoPen);
        for (int i = 1; i <= 6; ++i) {
            painter->setBrush(QColor(accentColor.red(), accentColor.green(), accentColor.blue(), 60 / i));
            painter->drawRoundedRect(cardRect.adjusted(-i*2, -i*2, i*2, i*2), 12, 12);
        }
    } else {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 0, 0, 100));
        painter->drawRoundedRect(cardRect.adjusted(4, 6, 4, 6), 10, 10);
    }

    // 绘制主体卡片背景 (暗黑质感渐变)
    auto bgGrad = QLinearGradient(cardRect.topLeft(), cardRect.bottomLeft());
    bgGrad.setColorAt(0.0, QColor(0x1A, 0x1A, 0x1A));
    bgGrad.setColorAt(1.0, QColor(0x0A, 0x0A, 0x0A));
    painter->setBrush(bgGrad);
    if (isActive)
        painter->setPen(QPen(accentColor, 2.0));
    else
        painter->setPen(QPen(QColor(0x44, 0x44, 0x44), 1));
    painter->drawRoundedRect(cardRect, 10, 10);

    // 绘制顶部彩色装饰条 (Vercel风格特色)
    painter->setPen(Qt::NoPen);
    painter->setBrush(isActive ? accentColor : baseColor);
    QPainterPath topBar;
    topBar.moveTo(cardRect.left(), cardRect.top() + 12);
    topBar.arcTo(cardRect.left(), cardRect.top(), 20, 20, 180, -90);
    topBar.arcTo(cardRect.right() - 20, cardRect.top(), 20, 20, 90, -90);
    topBar.lineTo(cardRect.right(), cardRect.top() + 6);
    topBar.lineTo(cardRect.left(), cardRect.top() + 6);
    painter->drawPath(topBar);

    // 绘制主标签
    painter->setPen(QColor(0xED, 0xED, 0xED));
    QFont font("Inter, -apple-system, sans-serif", 14, QFont::Bold);
    painter->setFont(font);
    painter->drawText(QRectF(cardRect.left(), cardRect.top() + 8, cardRect.width(), 50),
                      Qt::AlignCenter, label);

    // 绘制ID (次级文本)
    painter->setPen(QColor(0x88, 0x88, 0x88));
    font.setBold(false);
    font.setPointSize(12);
    painter->setFont(font);
    painter->drawText(QRectF(cardRect.left(), cardRect.top() + 50, cardRect.width(), 42),
                      Qt::AlignCenter, id_);

    // ── 实时指标覆盖层 ─────────────────────────────────────────────
    if (metrics_.active) {
        // 卡片主体固定高度 100px: top=-50, bottom=50
        const double cardBottom = cardRect.bottom(); // = 50
        QRectF metricsRect(cardRect.left(), cardBottom, cardRect.width(), 26);
        painter->setBrush(QColor(0x05, 0x05, 0x05, 230));
        painter->setPen(QPen(accentColor.darker(150), 1));
        painter->drawRoundedRect(metricsRect, 4, 4);

        QFont mf("JetBrains Mono, Consolas", 7);
        painter->setFont(mf);

        // 延迟
        if (metrics_.latency_ns >= 0) {
            painter->setPen(QColor(0xF9, 0x73, 0x16));
            painter->drawText(QRectF(cardRect.left() + 4, cardBottom + 2, 56, 11),
                              Qt::AlignLeft, QString("%1ns").arg(metrics_.latency_ns, 0, 'f', 0));
        }
        // 带宽
        if (metrics_.bandwidth_gbps >= 0) {
            painter->setPen(QColor(0x22, 0xC5, 0x5E));
            painter->drawText(QRectF(cardRect.left() + 62, cardBottom + 2, 56, 11),
                              Qt::AlignCenter, QString("%1GB/s").arg(metrics_.bandwidth_gbps, 0, 'f', 1));
        }
        // 负载百分比 + 进度条
        if (metrics_.load_pct >= 0) {
            QColor loadColor = metrics_.load_pct > 80 ? QColor(0xEF, 0x44, 0x44)
                             : metrics_.load_pct > 50 ? QColor(0xFB, 0xBF, 0x24)
                             : QColor(0x22, 0xC5, 0x5E);
            painter->setPen(loadColor);
            painter->drawText(QRectF(cardRect.right() - 46, cardBottom + 2, 42, 11),
                              Qt::AlignRight, QString("%1%").arg(metrics_.load_pct, 0, 'f', 0));

            QRectF barBg(cardRect.left() + 4, cardBottom + 15, cardRect.width() - 8, 6);
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(0x22, 0x22, 0x22));
            painter->drawRoundedRect(barBg, 3, 3);
            QRectF barFg(barBg.left(), barBg.top(),
                         barBg.width() * metrics_.load_pct / 100.0, barBg.height());
            painter->setBrush(loadColor);
            painter->drawRoundedRect(barFg, 3, 3);
        }
    }
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
    : QGraphicsItem(parent), from_(from), to_(to), 
      bandwidth_gbps_(64.0), latency_ns_(40.0), utilization_pct_(0.0)
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

    // 悬浮式胶囊标签 (Pill Badge) - 双行显示
    if (bandwidth_gbps_ > 0) {
        QPointF midPoint = (p1 + p2) / 2;
        
        // 第一行：物理配置（静态）
        QString configLine = QString("%1 GB/s • %2 ns")
            .arg(bandwidth_gbps_, 0, 'f', 0).arg(latency_ns_, 0, 'f', 0);
        
        // 第二行：实时利用率（动态）
        QString utilizLine = utilization_pct_ > 0
            ? QString("▸ %1%").arg(utilization_pct_, 0, 'f', 0)
            : QString("idle");
        
        QFont configFont("JetBrains Mono, Consolas", 7);
        QFont utilizFont("Inter, -apple-system", 9, QFont::Bold);
        
        QFontMetrics configFm(configFont);
        QFontMetrics utilizFm(utilizFont);
        
        int w1 = configFm.horizontalAdvance(configLine);
        int w2 = utilizFm.horizontalAdvance(utilizLine);
        int maxWidth = std::max(w1, w2);
        int totalHeight = configFm.height() + utilizFm.height() + 4;
        
        QRectF labelRect(-maxWidth/2 - 10, -totalHeight/2 - 4, maxWidth + 20, totalHeight + 8);
        labelRect.moveCenter(midPoint);
        
        // 毛玻璃质感底色
        painter->setPen(QPen(QColor(0x33, 0x33, 0x33), 1));
        painter->setBrush(QColor(0x0A, 0x0A, 0x0A, 220));
        painter->drawRoundedRect(labelRect, labelRect.height()/2.5, labelRect.height()/2.5);
        
        // 绘制第一行：配置参数（灰色小字）
        painter->setFont(configFont);
        painter->setPen(QColor(0x66, 0x66, 0x66));
        QRectF line1Rect(labelRect.left(), labelRect.top() + 4, labelRect.width(), configFm.height());
        painter->drawText(line1Rect, Qt::AlignCenter, configLine);
        
        // 绘制第二行：实时利用率（彩色粗体）
        painter->setFont(utilizFont);
        QColor utilizColor = utilization_pct_ > 80 ? QColor(0xEF, 0x44, 0x44)  // 红色-拥塞
                           : utilization_pct_ > 50 ? QColor(0xFB, 0xBF, 0x24)  // 黄色-中等
                           : utilization_pct_ > 0  ? QColor(0x22, 0xC5, 0x5E)  // 绿色-正常
                           : QColor(0x44, 0x44, 0x44);                          // 灰色-空闲
        painter->setPen(active ? QColor(0x60, 0xA5, 0xFA) : utilizColor);
        QRectF line2Rect(labelRect.left(), labelRect.top() + configFm.height() + 4, 
                         labelRect.width(), utilizFm.height());
        painter->drawText(line2Rect, Qt::AlignCenter, utilizLine);
    }
}

void LinkItem::updatePosition() {
    prepareGeometryChange();
}
