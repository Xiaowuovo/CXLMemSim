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
#include <QWheelEvent>
#include <QKeyEvent>
#include <QLabel>
#include "config_parser.h"

/**
 * @brief 支持滚轮缩放和平移的 GraphicsView
 */
class ZoomableGraphicsView : public QGraphicsView {
    Q_OBJECT
public:
    explicit ZoomableGraphicsView(QGraphicsScene* scene, QWidget* parent = nullptr);
protected:
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
private:
    double zoomFactor_;
};

// Forward declarations
class ComponentItem;
class LinkItem;

/**
 * @brief Graphical representation of a CXL component
 */
class TopologyEditorWidget;

/**
 * @brief 设备实时指标（模拟运行时填充）
 */
struct DeviceMetrics {
    double latency_ns    = -1.0;   ///< -1 表示无数据
    double bandwidth_gbps = -1.0;
    double load_pct       = -1.0;
    bool   active         = false;
};

class ComponentItem : public QGraphicsItem {
public:
    enum ComponentType {
        RootComplex,
        Switch,
        CXLDevice
    };

    ComponentItem(const QString& id, ComponentType type, TopologyEditorWidget* editor, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    QString id() const { return id_; }
    ComponentType componentType() const { return type_; }
    void setId(const QString& id) { id_ = id; update(); }

    void setHighlight(bool highlight) { highlighted_ = highlight; update(); }
    void addLink(LinkItem* link) { links_.append(link); }
    void removeLink(LinkItem* link) { links_.removeOne(link); }
    QList<LinkItem*> links() const { return links_; }

    void setMetrics(const DeviceMetrics& m) { metrics_ = m; update(); }
    const DeviceMetrics& metrics() const { return metrics_; }

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
    TopologyEditorWidget* editor_;
    DeviceMetrics metrics_;
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
    void setUtilization(double pct) { utilization_pct_ = pct; update(); }
    
    ComponentItem* fromComponent() const { return from_; }
    ComponentItem* toComponent() const { return to_; }

private:
    ComponentItem* from_;
    ComponentItem* to_;
    double bandwidth_gbps_;  ///< 物理最大带宽（静态配置）
    double latency_ns_;      ///< 链路延迟（静态配置）
    double utilization_pct_; ///< 实时利用率 0-100%
};

/**
 * @brief Main topology editor widget
 */
class TopologyEditorWidget : public QWidget {
    Q_OBJECT

public:
    explicit TopologyEditorWidget(QWidget *parent = nullptr);
    ~TopologyEditorWidget();

    cxlsim::CXLSimConfig getCurrentConfig() const;
    
    bool isConnectionMode() const { return connectionMode_; }
    ComponentItem* getConnectionSource() const { return connectionSource_; }
    void startConnection(ComponentItem* from);
    void finishConnection(ComponentItem* to);

    void updateDeviceMetrics(const QString& deviceId, const DeviceMetrics& m);
    void updateLinkUtilization(const QString& fromId, const QString& toId, double utilizPct);
    void clearAllMetrics();
    void setZoomLevel(double factor);
    double zoomLevel() const;

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

    QToolBar* toolbar_;
    ZoomableGraphicsView* view_;
    QGraphicsScene* scene_;
    QLabel* zoomLabel_;

    QMap<QString, ComponentItem*> components_;
    QList<LinkItem*> links_;
    
    bool connectionMode_;
    ComponentItem* connectionSource_;
    QGraphicsLineItem* connectionPreview_;
    int deviceCounter_;
    int switchCounter_;
};

#endif // TOPOLOGY_EDITOR_WIDGET_H
