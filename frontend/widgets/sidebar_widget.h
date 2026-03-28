/**
 * @file sidebar_widget.h
 * @brief VSCode风格的侧边栏导航
 */

#pragma once

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QButtonGroup>

class SidebarWidget : public QWidget {
    Q_OBJECT

public:
    enum PageType {
        TOPOLOGY = 0,    // 拓扑编辑
        CONFIG,          // 配置树
        WORKLOAD,        // 负载配置
        EXPERIMENT,      // 实验管理
        METRICS,         // 性能指标
        LOG              // 运行日志
    };

    explicit SidebarWidget(QWidget *parent = nullptr);
    ~SidebarWidget();

    void setActivePage(PageType page);
    PageType currentPage() const { return currentPage_; }

signals:
    void pageChanged(int pageIndex);

private:
    void setupUI();
    QPushButton* createIconButton(const QString& icon, const QString& tooltip, PageType page);

    QButtonGroup* buttonGroup_;
    QVBoxLayout* mainLayout_;
    PageType currentPage_;
};
