/**
 * @file sidebar_widget.cpp
 * @brief VSCode风格的侧边栏导航实现
 */

#include "sidebar_widget.h"
#include <QToolTip>
#include <QLabel>
#include <QFrame>

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
    setFixedWidth(56);
    setStyleSheet(
        "QWidget { "
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0A0A0A, stop:1 #000000); "
        "    border-right: 1px solid #1A1A1A; "
        "}"
    );
    
    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setContentsMargins(4, 12, 4, 12);
    mainLayout_->setSpacing(2);
    
    buttonGroup_ = new QButtonGroup(this);
    buttonGroup_->setExclusive(true);
    
    // ══════════════════════════════════════════════════════════
    // 工作区分组 (WORKSPACE)
    // ══════════════════════════════════════════════════════════
    auto* workspaceLabel = new QLabel("WORK", this);
    workspaceLabel->setAlignment(Qt::AlignCenter);
    workspaceLabel->setStyleSheet(
        "color: #555555; font-size: 9px; font-weight: 700; "
        "letter-spacing: 1px; padding: 8px 0 4px 0;"
    );
    mainLayout_->addWidget(workspaceLabel);
    
    auto* topoBtn = createIconButton("⬢", "拓扑编辑\nTopology Editor", TOPOLOGY);
    auto* cfgBtn = createIconButton("⚙", "系统配置\nSystem Config", CONFIG);
    auto* wlBtn = createIconButton("⚡", "负载配置\nWorkload Config", WORKLOAD);
    
    buttonGroup_->addButton(topoBtn, TOPOLOGY);
    buttonGroup_->addButton(cfgBtn, CONFIG);
    buttonGroup_->addButton(wlBtn, WORKLOAD);
    
    mainLayout_->addWidget(topoBtn);
    mainLayout_->addWidget(cfgBtn);
    mainLayout_->addWidget(wlBtn);
    
    // 基准测试页面按钮
    auto* benchBtn = createIconButton("⚑", "基准测试\nBenchmark", BENCHMARK);
    buttonGroup_->addButton(benchBtn, BENCHMARK);
    mainLayout_->addWidget(benchBtn);
    
    // ══════════════════════════════════════════════════════════
    // 功能按钮组（快捷操作）- 导出数据
    // ══════════════════════════════════════════════════════════
    auto* divider1 = new QFrame(this);
    divider1->setFrameShape(QFrame::HLine);
    divider1->setStyleSheet("background: #1A1A1A; max-height: 1px; margin: 8px 8px;");
    mainLayout_->addWidget(divider1);
    
    auto* exportBtn = new QPushButton("⎘", this);
    exportBtn->setToolTip("导出数据\nExport Data");
    exportBtn->setFixedSize(48, 44);
    exportBtn->setCursor(Qt::PointingHandCursor);
    exportBtn->setStyleSheet(
        "QPushButton {"
        "    background: transparent;"
        "    border: none;"
        "    border-left: 2px solid transparent;"
        "    border-radius: 6px;"
        "    color: #666666;"
        "    font-size: 22px;"
        "    font-weight: 300;"
        "    padding: 0;"
        "    margin: 2px 0;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "        stop:0 rgba(79, 195, 247, 0.08), "
        "        stop:1 rgba(79, 195, 247, 0.02));"
        "    color: #EDEDED;"
        "    border-left-color: #4FC3F7;"
        "}"
        "QPushButton:checked {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "        stop:0 rgba(79, 195, 247, 0.15), "
        "        stop:1 rgba(79, 195, 247, 0.05));"
        "    border-left: 2px solid #4FC3F7;"
        "    color: #4FC3F7;"
        "    font-weight: 500;"
        "}"
        "QPushButton:checked:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "        stop:0 rgba(79, 195, 247, 0.20), "
        "        stop:1 rgba(79, 195, 247, 0.08));"
        "}");
    connect(exportBtn, &QPushButton::clicked, this, &SidebarWidget::exportDataRequested);
    mainLayout_->addWidget(exportBtn);
    
    // 分隔线
    auto* divider2 = new QFrame(this);
    divider2->setFrameShape(QFrame::HLine);
    divider2->setStyleSheet("background: #1A1A1A; max-height: 1px; margin: 8px 8px;");
    mainLayout_->addWidget(divider2);
    
    // ══════════════════════════════════════════════════════════
    // 分析分组 (ANALYSIS)
    // ══════════════════════════════════════════════════════════
    auto* analysisLabel = new QLabel("TOOLS", this);
    analysisLabel->setAlignment(Qt::AlignCenter);
    analysisLabel->setStyleSheet(
        "color: #555555; font-size: 9px; font-weight: 700; "
        "letter-spacing: 1px; padding: 4px 0;"
    );
    mainLayout_->addWidget(analysisLabel);
    
    auto* expBtn = createIconButton("◈", "实验管理\nExperiments", EXPERIMENT);
    auto* metBtn = createIconButton("◐", "性能监控\nMetrics", METRICS);
    auto* logBtn = createIconButton("◫", "运行日志\nLogs", LOG);
    
    buttonGroup_->addButton(expBtn, EXPERIMENT);
    buttonGroup_->addButton(metBtn, METRICS);
    buttonGroup_->addButton(logBtn, LOG);
    
    mainLayout_->addWidget(expBtn);
    mainLayout_->addWidget(metBtn);
    mainLayout_->addWidget(logBtn);
    
    mainLayout_->addStretch();

    // 默认选中拓扑编辑
    topoBtn->setChecked(true);

    connect(buttonGroup_, QOverload<int>::of(&QButtonGroup::idClicked),
            this, &SidebarWidget::pageChanged);
}

QPushButton* SidebarWidget::createIconButton(const QString& icon, const QString& tooltip, PageType page) {
    (void)page;
    auto* btn = new QPushButton(icon, this);
    btn->setCheckable(true);
    btn->setToolTip(tooltip);
    btn->setFixedSize(48, 44);
    btn->setCursor(Qt::PointingHandCursor);
    
    btn->setStyleSheet(
        "QPushButton {"
        "    background: transparent;"
        "    border: none;"
        "    border-left: 2px solid transparent;"
        "    border-radius: 6px;"
        "    color: #666666;"
        "    font-size: 22px;"
        "    font-weight: 300;"
        "    padding: 0;"
        "    margin: 2px 0;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "        stop:0 rgba(79, 195, 247, 0.08), "
        "        stop:1 rgba(79, 195, 247, 0.02));"
        "    color: #EDEDED;"
        "    border-left-color: #4FC3F7;"
        "}"
        "QPushButton:checked {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "        stop:0 rgba(79, 195, 247, 0.15), "
        "        stop:1 rgba(79, 195, 247, 0.05));"
        "    border-left: 2px solid #4FC3F7;"
        "    color: #4FC3F7;"
        "    font-weight: 500;"
        "}"
        "QPushButton:checked:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "        stop:0 rgba(79, 195, 247, 0.20), "
        "        stop:1 rgba(79, 195, 247, 0.08));"
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
