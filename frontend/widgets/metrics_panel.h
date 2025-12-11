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

private:
    void setupUI();

    // Display widgets
    QLCDNumber* epochNumber_;
    QLabel* totalAccesses_;
    QLabel* l3Misses_;
    QLabel* cxlAccesses_;
    QLabel* avgLatency_;
    QLabel* totalDelay_;
    QProgressBar* missRate_;

    // Charts
    QTabWidget* chartTabs_;
    RealTimeChartWidget* latencyChart_;
    RealTimeChartWidget* bandwidthChart_;
    RealTimeChartWidget* missRateChart_;

    // Group boxes
    QGroupBox* createEpochGroup();
    QGroupBox* createAccessGroup();
    QGroupBox* createLatencyGroup();
    QGroupBox* createChartGroup();
};

#endif // METRICS_PANEL_H
