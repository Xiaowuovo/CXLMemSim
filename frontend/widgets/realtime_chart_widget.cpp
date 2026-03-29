/**
 * @file realtime_chart_widget.cpp
 * @brief Real-time chart implementation using QPainter
 */

#include "realtime_chart_widget.h"
#include <QPainter>
#include <QPainterPath>
#include <algorithm>
#include <cmath>

RealTimeChartWidget::RealTimeChartWidget(QWidget *parent)
    : QWidget(parent)
    , title_("Chart")
    , yAxisLabel_("Value")
    , lineColor_(Qt::blue)
    , maxPoints_(60) // Default 60 points (e.g., 60 seconds)
    , maxValue_(100.0)
    , baselineLabel_("")
    , baselineColor_(QColor(0x88, 0x88, 0x88, 180))  // 半透明灰色
{
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
    setMinimumHeight(150);
}

void RealTimeChartWidget::addDataPoint(double value) {
    data_.append(value);
    if (data_.size() > maxPoints_) {
        data_.removeFirst();
    }

    // Update max value for auto-scaling (with some decay)
    if (value > maxValue_) {
        maxValue_ = value * 1.2; // Add 20% headroom
    } else {
        // Slowly decay max value if current values are low
        maxValue_ = std::max(value * 1.2, maxValue_ * 0.99);
    }

    // Ensure minimum scale
    if (maxValue_ < 10.0) maxValue_ = 10.0;

    update();
}

void RealTimeChartWidget::setTitle(const QString& title) {
    title_ = title;
    update();
}

void RealTimeChartWidget::setYAxisLabel(const QString& label) {
    yAxisLabel_ = label;
    update();
}

void RealTimeChartWidget::setLineColor(const QColor& color) {
    lineColor_ = color;
    update();
}

void RealTimeChartWidget::setMaxPoints(int count) {
    maxPoints_ = count;
    while (data_.size() > maxPoints_) {
        data_.removeFirst();
    }
    update();
}

void RealTimeChartWidget::clear() {
    data_.clear();
    maxValue_ = 100.0;
    update();
}

void RealTimeChartWidget::pinCurrentAsBaseline(const QString& label) {
    baseline_ = data_;
    baselineLabel_ = label.isEmpty() ? "Baseline" : label;
    update();
}

void RealTimeChartWidget::clearBaseline() {
    baseline_.clear();
    baselineLabel_.clear();
    update();
}

void RealTimeChartWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Dimensions
    int w = width();
    int h = height();
    int padding = 30;
    int graphX = padding + 10;
    int graphY = padding;
    int graphW = w - graphX - 10;
    int graphH = h - graphY - padding;

    // Draw background (深色主题)
    painter.fillRect(rect(), QColor(0x0A, 0x0A, 0x0A));
    painter.setPen(QColor(0x33, 0x33, 0x33));
    painter.drawRect(graphX, graphY, graphW, graphH);

    // Draw grid lines (horizontal)
    painter.setPen(QPen(QColor(0x1A, 0x1A, 0x1A), 1, Qt::DashLine));
    for (int i = 1; i < 5; ++i) {
        int y = graphY + (graphH * i / 5);
        painter.drawLine(graphX, y, graphX + graphW, y);
    }

    // Draw title
    painter.setPen(QColor(0xE8, 0xE8, 0xE8));
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(10);
    painter.setFont(titleFont);
    painter.drawText(rect(), Qt::AlignTop | Qt::AlignHCenter, title_);

    // Draw Y-axis labels
    QFont labelFont = font();
    labelFont.setPointSize(9);
    painter.setFont(labelFont);
    painter.setPen(QColor(0x88, 0x88, 0x88));
    painter.drawText(QRect(0, graphY - 10, padding, 20), Qt::AlignRight | Qt::AlignVCenter, QString::number(maxValue_, 'f', 0));
    painter.drawText(QRect(0, graphY + graphH - 10, padding, 20), Qt::AlignRight | Qt::AlignVCenter, "0");

    // ── 绘制基准曲线（科研对比关键）──
    if (!baseline_.isEmpty() && baseline_.size() > 1) {
        QPainterPath baselinePath;
        double xStep = (double)graphW / (maxPoints_ - 1);
        
        for (int i = 0; i < baseline_.size(); ++i) {
            double x = graphX + (i * xStep);
            if (baseline_.size() < maxPoints_) {
                x = graphX + graphW - ((baseline_.size() - 1 - i) * xStep);
            }
            
            double normalizedY = baseline_[i] / maxValue_;
            double y = graphY + graphH - (normalizedY * graphH);
            
            if (i == 0) {
                baselinePath.moveTo(x, y);
            } else {
                baselinePath.lineTo(x, y);
            }
        }
        
        // 虚线样式，灰色半透明
        painter.setPen(QPen(baselineColor_, 2, Qt::DashLine));
        painter.drawPath(baselinePath);
        
        // 基准标签
        if (!baselineLabel_.isEmpty()) {
            painter.setPen(baselineColor_);
            QFont labelFont = font();
            labelFont.setPointSize(9);
            painter.setFont(labelFont);
            painter.drawText(QRect(graphX + 10, graphY + 5, 100, 20), 
                           Qt::AlignLeft | Qt::AlignVCenter, 
                           "📌 " + baselineLabel_);
        }
    }

    // Draw data
    if (data_.size() > 1) {
        QPainterPath path;
        double xStep = (double)graphW / (maxPoints_ - 1);

        // Start from the right side
        int startIdx = 0;
        if (data_.size() < maxPoints_) {
            // If not full, start drawing from left but map correctly
        }

        for (int i = 0; i < data_.size(); ++i) {
            double x = graphX + (i * xStep);
            // If we have fewer points than max, we stretch them or align left?
            // Let's align right for scrolling effect
            if (data_.size() < maxPoints_) {
                x = graphX + graphW - ((data_.size() - 1 - i) * xStep);
            }

            double normalizedY = data_[i] / maxValue_;
            double y = graphY + graphH - (normalizedY * graphH);

            if (i == 0) {
                path.moveTo(x, y);
            } else {
                path.lineTo(x, y);
            }
        }

        // Draw line
        painter.setPen(QPen(lineColor_, 2));
        painter.drawPath(path);

        // Draw fill area
        path.lineTo(graphX + graphW, graphY + graphH);
        if (data_.size() < maxPoints_) {
             path.lineTo(graphX + graphW - ((data_.size() - 1) * xStep), graphY + graphH);
        } else {
             path.lineTo(graphX, graphY + graphH);
        }
        path.closeSubpath();

        QColor fillColor = lineColor_;
        fillColor.setAlpha(50);
        painter.setBrush(fillColor);
        painter.setPen(Qt::NoPen);
        painter.drawPath(path);
    }
}
