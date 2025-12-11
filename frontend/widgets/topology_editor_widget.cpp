/**
 * @file topology_editor_widget.cpp
 * @brief Topology editor implementation
 */

#include "topology_editor_widget.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QPen>
#include <QBrush>
#include <QFileDialog>
#include <cmath>

// TopologyEditorWidget implementation

TopologyEditorWidget::TopologyEditorWidget(QWidget *parent)
    : QWidget(parent)
    , view_(nullptr)
    , scene_(nullptr)
{
    setupUI();
}

TopologyEditorWidget::~TopologyEditorWidget() {
}

void TopologyEditorWidget::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // Create scene and view
    scene_ = new QGraphicsScene(this);
    scene_->setSceneRect(-400, -300, 800, 600);

    view_ = new QGraphicsView(scene_, this);
    view_->setRenderHint(QPainter::Antialiasing);
    view_->setDragMode(QGraphicsView::ScrollHandDrag);
    view_->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    layout->addWidget(view_);
}

void TopologyEditorWidget::updateTopology(const cxlsim::CXLSimConfig& config) {
    clearTopology();

    // Create root complex
    if (!config.root_complex.id.empty()) {
        ComponentItem* rc = new ComponentItem(
            QString::fromStdString(config.root_complex.id),
            ComponentItem::RootComplex
        );
        rc->setPos(0, -200);
        scene_->addItem(rc);
        components_[QString::fromStdString(config.root_complex.id)] = rc;
    }

    // Create switches
    int switchX = -150;
    for (const auto& sw : config.switches) {
        ComponentItem* item = new ComponentItem(
            QString::fromStdString(sw.id),
            ComponentItem::Switch
        );
        item->setPos(switchX, 0);
        scene_->addItem(item);
        components_[QString::fromStdString(sw.id)] = item;
        switchX += 150;
    }

    // Create CXL devices
    int deviceX = -200;
    int deviceCount = config.cxl_devices.size();
    int spacing = 400 / std::max(1, deviceCount);

    for (const auto& dev : config.cxl_devices) {
        ComponentItem* item = new ComponentItem(
            QString::fromStdString(dev.id),
            ComponentItem::CXLDevice
        );
        item->setPos(deviceX, 200);
        scene_->addItem(item);
        components_[QString::fromStdString(dev.id)] = item;
        deviceX += spacing;
    }

    // Create connections
    for (const auto& conn : config.connections) {
        QString fromId = QString::fromStdString(conn.from);
        QString toId = QString::fromStdString(conn.to);

        if (components_.contains(fromId) && components_.contains(toId)) {
            LinkItem* link = new LinkItem(components_[fromId], components_[toId]);

            // Find link spec
            // auto it = config.link_specs.find(conn.link);
            // if (it != config.link_specs.end()) {
            //    link->setBandwidth(it->second.bandwidth_gbps);
            //    link->setLatency(it->second.latency_ns);
            // }

            scene_->addItem(link);
            links_.append(link);
        }
    }
}

void TopologyEditorWidget::clearTopology() {
    scene_->clear();
    components_.clear();
    links_.clear();
}

void TopologyEditorWidget::exportToImage(const QString& filename) {
    scene_->clearSelection();
    QRectF rect = scene_->itemsBoundingRect();
    rect.adjust(-50, -50, 50, 50);

    QImage image(rect.size().toSize(), QImage::Format_ARGB32);
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    scene_->render(&painter, QRectF(), rect);

    image.save(filename);
}

void TopologyEditorWidget::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);

    QAction* addDeviceAction = menu.addAction("Add CXL Device");
    connect(addDeviceAction, &QAction::triggered, this, &TopologyEditorWidget::addDeviceRequested);

    QAction* addSwitchAction = menu.addAction("Add Switch");
    connect(addSwitchAction, &QAction::triggered, this, &TopologyEditorWidget::addSwitchRequested);

    menu.addSeparator();

    QAction* exportAction = menu.addAction("Export to Image...");
    connect(exportAction, &QAction::triggered, [this]() {
        QString filename = QFileDialog::getSaveFileName(
            this, "Export Topology", "topology.png", "Images (*.png *.jpg)");
        if (!filename.isEmpty()) {
            exportToImage(filename);
        }
    });

    menu.addSeparator();

    // Check if an item is selected
    bool hasSelection = !scene_->selectedItems().isEmpty();
    QAction* removeAction = menu.addAction("Remove Selected");
    removeAction->setEnabled(hasSelection);
    connect(removeAction, &QAction::triggered, this, &TopologyEditorWidget::removeSelectedRequested);

    menu.exec(event->globalPos());
}

// ComponentItem implementation

ComponentItem::ComponentItem(const QString& id, ComponentType type, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , id_(id)
    , type_(type)
    , highlighted_(false)
{
    setFlag(ItemIsMovable);
    setFlag(ItemIsSelectable);
    setFlag(ItemSendsGeometryChanges);
}

QRectF ComponentItem::boundingRect() const {
    return QRectF(-50, -30, 100, 60);
}

void ComponentItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // Choose color based on type
    QColor fillColor;
    QString label;

    switch (type_) {
        case RootComplex:
            fillColor = QColor(100, 150, 255);
            label = "RC";
            break;
        case Switch:
            fillColor = QColor(150, 200, 150);
            label = "SW";
            break;
        case CXLDevice:
            fillColor = QColor(255, 200, 100);
            label = "CXL";
            break;
    }

    if (highlighted_ || isSelected()) {
        fillColor = fillColor.lighter(120);
    }

    // Draw component box
    painter->setPen(QPen(Qt::black, 2));
    painter->setBrush(fillColor);
    painter->drawRoundedRect(boundingRect(), 5, 5);

    // Draw label
    painter->setPen(Qt::black);
    QFont font = painter->font();
    font.setBold(true);
    painter->setFont(font);

    QRectF textRect = boundingRect();
    painter->drawText(textRect, Qt::AlignCenter, label + "\n" + id_);
}

void ComponentItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    dragStartPos_ = pos();
    QGraphicsItem::mousePressEvent(event);
}

void ComponentItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    QGraphicsItem::mouseMoveEvent(event);
}

void ComponentItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    QGraphicsItem::mouseReleaseEvent(event);
}

// LinkItem implementation

LinkItem::LinkItem(ComponentItem* from, ComponentItem* to, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , from_(from)
    , to_(to)
    , bandwidth_gbps_(0)
    , latency_ns_(0)
{
    setZValue(-1);  // Draw behind components
}

QRectF LinkItem::boundingRect() const {
    QPointF p1 = from_->pos();
    QPointF p2 = to_->pos();

    qreal minX = std::min(p1.x(), p2.x()) - 10;
    qreal minY = std::min(p1.y(), p2.y()) - 10;
    qreal maxX = std::max(p1.x(), p2.x()) + 10;
    qreal maxY = std::max(p1.y(), p2.y()) + 10;

    return QRectF(minX, minY, maxX - minX, maxY - minY);
}

void LinkItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QPointF p1 = from_->pos();
    QPointF p2 = to_->pos();

    // Draw line
    painter->setPen(QPen(Qt::darkGray, 2, Qt::SolidLine));
    painter->drawLine(p1, p2);

    // Draw arrow
    QLineF line(p1, p2);
    double angle = std::atan2(-line.dy(), line.dx());

    QPointF arrowP1 = p2 - QPointF(sin(angle + M_PI / 3) * 10,
                                    cos(angle + M_PI / 3) * 10);
    QPointF arrowP2 = p2 - QPointF(sin(angle + M_PI - M_PI / 3) * 10,
                                    cos(angle + M_PI - M_PI / 3) * 10);

    QPolygonF arrowHead;
    arrowHead << p2 << arrowP1 << arrowP2;
    painter->setBrush(Qt::darkGray);
    painter->drawPolygon(arrowHead);

    // Draw label with bandwidth and latency
    if (bandwidth_gbps_ > 0) {
        QPointF midPoint = (p1 + p2) / 2;
        QString label = QString("%1 GB/s\n%2 ns")
            .arg(bandwidth_gbps_, 0, 'f', 1)
            .arg(latency_ns_, 0, 'f', 1);

        painter->setPen(Qt::black);
        QFont font = painter->font();
        font.setPointSize(8);
        painter->setFont(font);
        painter->drawText(midPoint, label);
    }
}

void LinkItem::updatePosition() {
    prepareGeometryChange();
}
