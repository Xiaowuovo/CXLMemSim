#ifndef COMPARISON_WIDGET_H
#define COMPARISON_WIDGET_H

#include <QWidget>
#include <QLabel>
#include "benchmark_widget.h"

/**
 * @brief 性能对比视图Widget
 * 
 * 显示Current vs Baseline的多维度对比
 * - 柱状图对比（平均延迟、P99延迟、带宽）
 * - 差异百分比显示
 */
class ComparisonWidget : public QWidget {
    Q_OBJECT

public:
    explicit ComparisonWidget(QWidget *parent = nullptr);
    ~ComparisonWidget() = default;

    void updateComparison(const BenchmarkWidget::BenchmarkStats& current,
                         const BenchmarkWidget::BenchmarkStats& baseline);
    void clear();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    struct ComparisonData {
        double currentAvgLatency;
        double baselineAvgLatency;
        double currentP99Latency;
        double baselineP99Latency;
        double currentBandwidth;
        double baselineBandwidth;
        double currentUtilization;
        double baselineUtilization;
        
        ComparisonData() : currentAvgLatency(0), baselineAvgLatency(0),
                          currentP99Latency(0), baselineP99Latency(0),
                          currentBandwidth(0), baselineBandwidth(0),
                          currentUtilization(0), baselineUtilization(0) {}
    };

    ComparisonData data_;
    bool hasData_;
    
    void drawMetricBar(QPainter& painter, const QString& label,
                      double currentVal, double baselineVal,
                      const QString& unit, int y, int barHeight);
    QColor getPerformanceColor(double current, double baseline, bool lowerIsBetter) const;
};

#endif // COMPARISON_WIDGET_H
