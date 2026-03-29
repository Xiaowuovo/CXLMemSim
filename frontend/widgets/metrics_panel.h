/**
 * @file metrics_panel.h
 * @brief Performance metrics visualization panel
 */

#ifndef METRICS_PANEL_H
#define METRICS_PANEL_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QLCDNumber>
#include <QGroupBox>
#include <QTabWidget>
#include "analyzer/timing_analyzer.h"
#include "realtime_chart_widget.h"

/**
 * @brief Panel displaying real-time performance metrics
 */
class MetricsPanel : public QWidget {
    Q_OBJECT

public:
    explicit MetricsPanel(QWidget *parent = nullptr);
    ~MetricsPanel();

public slots:
    void updateStats(const cxlsim::EpochStats& stats);
    void reset();
    void pinCurrentAsBaseline();    ///< 固定当前数据为基准（科研对比）
    void clearBaseline();            ///< 清除基准曲线

private:
    void setupUI();

    // Display widgets
    QLCDNumber* epochNumber_;
    QLabel* bandwidthDisplay_;     ///< 实时带宽数字仪表盘
    QLabel* latencyDisplay_;       ///< 实时延迟数字仪表盘
    QLabel* totalAccesses_;
    QLabel* l3Misses_;
    QLabel* cxlAccesses_;
    QLabel* avgLatency_;
    QLabel* totalDelay_;
    QLabel* p95Latency_;          ///< P95尾延迟（抖动关键）
    QLabel* p99Latency_;          ///< P99尾延迟（最坏情况）
    QLabel* queuingDelay_;        ///< 排队延迟
    QLabel* tieringRatio_;        ///< 本地DRAM vs CXL比例
    QProgressBar* linkUtilBar_;   ///< 链路利用率进度条
    QProgressBar* missRate_;

    // Charts (顺序必须与构造函数初始化列表一致)
    RealTimeChartWidget* latencyChart_;
    RealTimeChartWidget* bandwidthChart_;
    QTabWidget* chartTabs_;
    RealTimeChartWidget* missRateChart_;
    
    // Current stats for baseline
    cxlsim::EpochStats currentStats_;

    // Group boxes
    QGroupBox* createEpochGroup();
    QGroupBox* createAccessGroup();
    QGroupBox* createLatencyGroup();
    QGroupBox* createChartGroup();
};

#endif // METRICS_PANEL_H
