/**
 * @file config_tree_widget.h
 * @brief Configuration tree view widget
 */

#ifndef CONFIG_TREE_WIDGET_H
#define CONFIG_TREE_WIDGET_H

#include <QWidget>
#include <QTreeWidget>
#include <QPushButton>
#include "config_parser.h"

/**
 * @brief Widget for viewing and editing configuration
 */
class ConfigTreeWidget : public QWidget {
    Q_OBJECT

public:
    explicit ConfigTreeWidget(QWidget *parent = nullptr);
    ~ConfigTreeWidget();

    void setConfig(const cxlsim::CXLSimConfig& config);
    cxlsim::CXLSimConfig getConfig() const;

signals:
    void configChanged(const cxlsim::CXLSimConfig& config);

public slots:
    void onAddDevice();
    void onAddSwitch();
    void onRemoveSelected();

private slots:
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);

private:
    void setupUI();
    void populateTree();
    void addRootComplexItem();
    void addSwitchesItem();
    void addDevicesItem();
    void addConnectionsItem();
    void addSimulationItem();

    QTreeWidget* tree_;
    QPushButton* addDeviceButton_;
    QPushButton* addSwitchButton_;
    QPushButton* removeButton_;

    cxlsim::CXLSimConfig config_;
};

#endif // CONFIG_TREE_WIDGET_H
