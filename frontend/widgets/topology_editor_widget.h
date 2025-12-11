/**
 * @file topology_editor_widget.h
 * @brief Visual topology editor using QGraphicsView
 */

#ifndef TOPOLOGY_EDITOR_WIDGET_H
#define TOPOLOGY_EDITOR_WIDGET_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QMap>
#include "config_parser.h"

class ComponentItem;
class LinkItem;

/**
 * @brief Main topology editor widget
 */
class TopologyEditorWidget : public QWidget {
    Q_OBJECT

public:
    explicit TopologyEditorWidget(QWidget *parent = nullptr);
    ~TopologyEditorWidget();

public slots:
    void updateTopology(const cxlsim::CXLSimConfig& config);
    void clearTopology();
    void exportToImage(const QString& filename);

signals:
    void componentSelected(const QString& id);
    void topologyModified();
    void addDeviceRequested();
    void addSwitchRequested();
    void removeSelectedRequested();

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    void setupUI();
    void layoutComponents();

    QGraphicsView* view_;
    QGraphicsScene* scene_;

    QMap<QString, ComponentItem*> components_;
    QList<LinkItem*> links_;
};

/**
 * @brief Graphical representation of a CXL component
 */
class ComponentItem : public QGraphicsItem {
public:
    enum ComponentType {
        RootComplex,
        Switch,
        CXLDevice
    };

    ComponentItem(const QString& id, ComponentType type, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    QString id() const { return id_; }
    ComponentType componentType() const { return type_; }

    void setHighlight(bool highlight) { highlighted_ = highlight; update(); }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    QString id_;
    ComponentType type_;
    bool highlighted_;
    QPointF dragStartPos_;
};

/**
 * @brief Graphical representation of a link between components
 */
class LinkItem : public QGraphicsItem {
public:
    LinkItem(ComponentItem* from, ComponentItem* to, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    void updatePosition();

    void setBandwidth(double gbps) { bandwidth_gbps_ = gbps; }
    void setLatency(double ns) { latency_ns_ = ns; }

private:
    ComponentItem* from_;
    ComponentItem* to_;
    double bandwidth_gbps_;
    double latency_ns_;
};

#endif // TOPOLOGY_EDITOR_WIDGET_H
