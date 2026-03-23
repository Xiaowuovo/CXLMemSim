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
#include <QToolBar>
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

    cxlsim::CXLSimConfig getCurrentConfig() const;

public slots:
    void updateTopology(const cxlsim::CXLSimConfig& config);
    void clearTopology();
    void exportToImage(const QString& filename);
    void autoLayout();
    
private slots:
    void onAddRootComplex();
    void onAddSwitch();
    void onAddCXLDevice();
    void onDeleteSelected();
    void onComponentMoved();

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
    void setupToolBar();
    void layoutComponents();
    void addComponent(ComponentItem::ComponentType type, const QPointF& pos);
    void startConnection(ComponentItem* from);
    void finishConnection(ComponentItem* to);

    QToolBar* toolbar_;
    QGraphicsView* view_;
    QGraphicsScene* scene_;

    QMap<QString, ComponentItem*> components_;
    QList<LinkItem*> links_;
    
    ComponentItem* connectionSource_;
    QGraphicsLineItem* connectionPreview_;
    int deviceCounter_;
    int switchCounter_;
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
    void setId(const QString& id) { id_ = id; update(); }

    void setHighlight(bool highlight) { highlighted_ = highlight; update(); }
    void addLink(LinkItem* link) { links_.append(link); }
    void removeLink(LinkItem* link) { links_.removeOne(link); }
    QList<LinkItem*> links() const { return links_; }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    QString id_;
    ComponentType type_;
    bool highlighted_;
    QPointF dragStartPos_;
    QList<LinkItem*> links_;
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
    
    ComponentItem* fromComponent() const { return from_; }
    ComponentItem* toComponent() const { return to_; }

private:
    ComponentItem* from_;
    ComponentItem* to_;
    double bandwidth_gbps_;
    double latency_ns_;
};

#endif // TOPOLOGY_EDITOR_WIDGET_H
