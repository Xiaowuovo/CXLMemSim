/**
 * @file realtime_chart_widget.h
 * @brief Custom widget for rendering real-time line charts
 */

#ifndef REALTIME_CHART_WIDGET_H
#define REALTIME_CHART_WIDGET_H

#include <QWidget>
#include <QVector>
#include <QColor>

class RealTimeChartWidget : public QWidget {
    Q_OBJECT

public:
    explicit RealTimeChartWidget(QWidget *parent = nullptr);

    void addDataPoint(double value);
    void setTitle(const QString& title);
    void setYAxisLabel(const QString& label);
    void setLineColor(const QColor& color);
    void setMaxPoints(int count);
    void clear();
    
    // ── 科研多组对比功能 ──
    void pinCurrentAsBaseline(const QString& label = "Baseline");  ///< 固定当前曲线为基准
    void clearBaseline();                                          ///< 清除基准曲线
    bool hasBaseline() const { return !baseline_.isEmpty(); }      ///< 是否有基准曲线

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString title_;
    QString yAxisLabel_;
    QColor lineColor_;
    QVector<double> data_;
    int maxPoints_;
    double maxValue_;
    
    // 基准曲线（用于对比）
    QVector<double> baseline_;
    QString baselineLabel_;
    QColor baselineColor_;
};

#endif // REALTIME_CHART_WIDGET_H
