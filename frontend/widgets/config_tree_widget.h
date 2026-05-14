/**
 * @file config_tree_widget.h
 * @brief Configuration tree view widget
 */

#ifndef CONFIG_TREE_WIDGET_H
#define CONFIG_TREE_WIDGET_H

#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QPushButton>
#include <QComboBox>
#include <QMap>
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
    // 用户点击「应用配置」后发出，携带已应用的配置
    void configApplied(const cxlsim::CXLSimConfig& config);
    // 任何参数被修改（尚未应用），供主窗口变灰提示
    void configDirty(bool dirty);

public slots:
    void onApplyConfig();
    void onResetConfig();

private slots:
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onComboChanged(QTreeWidgetItem* item, const QString& value);
    void markDirty();

private:
    void setupUI();
    void populateTree();
    void addRootComplexItem();
    void addSwitchesItem();
    void addDevicesItem();
    void addConnectionsItem();
    void addSimulationItem();
    void updateApplyButtonState();

    QTreeWidget* tree_;
    QPushButton* applyButton_;
    QPushButton* resetButton_;

    cxlsim::CXLSimConfig config_;        // 已应用的基准配置
    cxlsim::CXLSimConfig pendingConfig_; // 编辑中、尚未应用的配置
    bool isDirty_{false};               // 是否有未应用的变更

    // 记录每个树节点对应的「属性key」，便于combo回调时定位
    QMap<QTreeWidgetItem*, QString> itemKeyMap_;

    // 创建嵌入树节点的下拉框
    QComboBox* makeCombo(QTreeWidgetItem* parent,
                         const QString& label,
                         const QStringList& options,
                         const QString& current,
                         const QString& key);
};

#endif // CONFIG_TREE_WIDGET_H
