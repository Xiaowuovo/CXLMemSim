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

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString title_;
    QString yAxisLabel_;
    QColor lineColor_;
    QVector<double> data_;
    int maxPoints_;
    double maxValue_;
};

#endif // REALTIME_CHART_WIDGET_H
