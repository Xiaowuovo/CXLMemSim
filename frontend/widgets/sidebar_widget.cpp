/**
 * @file sidebar_widget.cpp
 * @brief VSCode风格的侧边栏导航实现
 */

#include "sidebar_widget.h"
#include <QToolTip>

SidebarWidget::SidebarWidget(QWidget *parent)
    : QWidget(parent)
    , buttonGroup_(nullptr)
    , mainLayout_(nullptr)
    , currentPage_(TOPOLOGY)
{
    setupUI();
}

SidebarWidget::~SidebarWidget() {}

void SidebarWidget::setupUI() {
    setFixedWidth(48);  // VSCode风格的窄侧边栏
    setStyleSheet("QWidget { background-color: #0A0A0A; border-right: 1px solid #1A1A1A; }");

    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setContentsMargins(0, 8, 0, 8);
    mainLayout_->setSpacing(4);

    buttonGroup_ = new QButtonGroup(this);
    buttonGroup_->setExclusive(true);

    // 拓扑编辑
    auto* topoBtn = createIconButton("🗺", "拓扑编辑 (Topology)", TOPOLOGY);
    buttonGroup_->addButton(topoBtn, TOPOLOGY);
    mainLayout_->addWidget(topoBtn);

    // 配置树
    auto* cfgBtn = createIconButton("⚙", "系统配置 (Config)", CONFIG);
    buttonGroup_->addButton(cfgBtn, CONFIG);
    mainLayout_->addWidget(cfgBtn);

    // 负载配置
    auto* wlBtn = createIconButton("🚀", "负载配置 (Workload)", WORKLOAD);
    buttonGroup_->addButton(wlBtn, WORKLOAD);
    mainLayout_->addWidget(wlBtn);

    mainLayout_->addSpacing(12);  // 分隔线效果

    // 实验管理
    auto* expBtn = createIconButton("⭐", "实验管理 (Experiment)", EXPERIMENT);
    buttonGroup_->addButton(expBtn, EXPERIMENT);
    mainLayout_->addWidget(expBtn);

    // 性能指标
    auto* metricBtn = createIconButton("📊", "性能指标 (Metrics)", METRICS);
    buttonGroup_->addButton(metricBtn, METRICS);
    mainLayout_->addWidget(metricBtn);

    // 运行日志
    auto* logBtn = createIconButton("📝", "运行日志 (Log)", LOG);
    buttonGroup_->addButton(logBtn, LOG);
    mainLayout_->addWidget(logBtn);

    mainLayout_->addStretch();

    // 默认选中拓扑编辑
    topoBtn->setChecked(true);

    connect(buttonGroup_, QOverload<int>::of(&QButtonGroup::idClicked),
            this, &SidebarWidget::pageChanged);
}

QPushButton* SidebarWidget::createIconButton(const QString& icon, const QString& tooltip, PageType page) {
    (void)page;  // 参数由buttonGroup管理，此处标记为有意未使用
    auto* btn = new QPushButton(icon, this);
    btn->setCheckable(true);
    btn->setToolTip(tooltip);
    btn->setFixedSize(48, 48);
    btn->setCursor(Qt::PointingHandCursor);
    
    btn->setStyleSheet(
        "QPushButton {"
        "    background-color: transparent;"
        "    border: none;"
        "    border-left: 2px solid transparent;"
        "    color: #666666;"
        "    font-size: 20px;"
        "    text-align: center;"
        "}"
        "QPushButton:hover {"
        "    background-color: #111111;"
        "    color: #EDEDED;"
        "}"
        "QPushButton:checked {"
        "    background-color: #1A1A1A;"
        "    border-left: 2px solid #3B82F6;"
        "    color: #FFFFFF;"
        "}"
    );

    return btn;
}

void SidebarWidget::setActivePage(PageType page) {
    if (currentPage_ != page) {
        currentPage_ = page;
        auto* btn = qobject_cast<QPushButton*>(buttonGroup_->button(page));
        if (btn) {
            btn->setChecked(true);
        }
    }
}
