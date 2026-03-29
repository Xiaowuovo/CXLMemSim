#include "comparison_widget.h"
#include <QPainter>
#include <QPainterPath>
#include <cmath>

ComparisonWidget::ComparisonWidget(QWidget *parent)
    : QWidget(parent)
    , hasData_(false)
{
    setMinimumSize(400, 300);
    setStyleSheet("background: #0A0A0A; border: 1px solid #222222; border-radius: 6px;");
}

void ComparisonWidget::updateComparison(const BenchmarkWidget::BenchmarkStats& current,
                                       const BenchmarkWidget::BenchmarkStats& baseline) {
    data_.currentAvgLatency = current.avg_latency_ns;
    data_.baselineAvgLatency = baseline.avg_latency_ns;
    data_.currentP99Latency = current.p99_latency_ns;
    data_.baselineP99Latency = baseline.p99_latency_ns;
    data_.currentBandwidth = current.bandwidth_gbps;
    data_.baselineBandwidth = baseline.bandwidth_gbps;
    data_.currentUtilization = current.link_utilization_pct;
    data_.baselineUtilization = baseline.link_utilization_pct;
    
    hasData_ = true;
    update();
}

void ComparisonWidget::clear() {
    data_ = ComparisonData();
    hasData_ = false;
    update();
}

void ComparisonWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int padding = 20;
    int titleHeight = 40;

    // 标题
    painter.setPen(QColor(0x88, 0x88, 0x88));
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    painter.setFont(titleFont);
    painter.drawText(QRect(padding, 10, w - 2 * padding, titleHeight),
                    Qt::AlignLeft | Qt::AlignVCenter,
                    "CURRENT vs BASELINE");

    if (!hasData_) {
        painter.setPen(QColor(0x66, 0x66, 0x66));
        QFont hintFont = font();
        hintFont.setPointSize(10);
        painter.setFont(hintFont);
        painter.drawText(rect(), Qt::AlignCenter, "运行基准测试后显示对比");
        return;
    }

    // 图例
    int legendY = titleHeight + 5;
    painter.setFont(font());
    painter.setPen(QColor(0x60, 0xA5, 0xFA));
    painter.drawText(QRect(padding, legendY, 80, 20), Qt::AlignLeft | Qt::AlignVCenter, "■ Current");
    painter.setPen(QColor(0x88, 0x88, 0x88));
    painter.drawText(QRect(padding + 90, legendY, 80, 20), Qt::AlignLeft | Qt::AlignVCenter, "■ Baseline");

    // 绘制对比柱状图
    int startY = titleHeight + 35;
    int barHeight = 45;
    int spacing = 15;

    drawMetricBar(painter, "平均延迟", 
                 data_.currentAvgLatency, data_.baselineAvgLatency,
                 "ns", startY, barHeight);
    
    drawMetricBar(painter, "P99 延迟",
                 data_.currentP99Latency, data_.baselineP99Latency,
                 "ns", startY + barHeight + spacing, barHeight);
    
    drawMetricBar(painter, "带宽",
                 data_.currentBandwidth, data_.baselineBandwidth,
                 "GB/s", startY + (barHeight + spacing) * 2, barHeight);
    
    drawMetricBar(painter, "链路利用率",
                 data_.currentUtilization, data_.baselineUtilization,
                 "%", startY + (barHeight + spacing) * 3, barHeight);
}

void ComparisonWidget::drawMetricBar(QPainter& painter, const QString& label,
                                    double currentVal, double baselineVal,
                                    const QString& unit, int y, int barHeight) {
    int w = width();
    int padding = 20;
    int labelWidth = 100;
    int barStartX = padding + labelWidth;
    int maxBarWidth = w - barStartX - padding - 80; // 留出空间显示数值

    // 绘制标签
    painter.setPen(QColor(0xAA, 0xAA, 0xAA));
    QFont labelFont = font();
    labelFont.setPointSize(10);
    painter.setFont(labelFont);
    painter.drawText(QRect(padding, y, labelWidth, barHeight),
                    Qt::AlignLeft | Qt::AlignVCenter, label);

    // 计算柱状图比例（延迟越低越好，带宽越高越好）
    bool lowerIsBetter = (unit == "ns");
    double maxVal = std::max(currentVal, baselineVal);
    if (maxVal < 0.01) maxVal = 1.0; // 避免除零

    // Current柱
    int currentBarY = y + 5;
    int currentBarHeight = (barHeight - 15) / 2;
    int currentBarWidth = static_cast<int>((currentVal / maxVal) * maxBarWidth);
    
    QColor currentColor = getPerformanceColor(currentVal, baselineVal, lowerIsBetter);
    painter.fillRect(barStartX, currentBarY, currentBarWidth, currentBarHeight, currentColor);
    
    // Baseline柱
    int baselineBarY = currentBarY + currentBarHeight + 5;
    int baselineBarWidth = static_cast<int>((baselineVal / maxVal) * maxBarWidth);
    
    painter.fillRect(barStartX, baselineBarY, baselineBarWidth, currentBarHeight, QColor(0x50, 0x50, 0x50));

    // 绘制数值
    painter.setPen(QColor(0xED, 0xED, 0xED));
    QFont valueFont = font();
    valueFont.setPointSize(9);
    valueFont.setFamily("Roboto Mono, Consolas, monospace");
    painter.setFont(valueFont);
    
    QString currentText = QString("%1 %2").arg(currentVal, 0, 'f', 1).arg(unit);
    painter.drawText(QRect(barStartX + currentBarWidth + 5, currentBarY, 70, currentBarHeight),
                    Qt::AlignLeft | Qt::AlignVCenter, currentText);
    
    QString baselineText = QString("%1 %2").arg(baselineVal, 0, 'f', 1).arg(unit);
    painter.setPen(QColor(0x88, 0x88, 0x88));
    painter.drawText(QRect(barStartX + baselineBarWidth + 5, baselineBarY, 70, currentBarHeight),
                    Qt::AlignLeft | Qt::AlignVCenter, baselineText);

    // 绘制改进百分比
    double improvement = 0;
    if (lowerIsBetter) {
        improvement = ((baselineVal - currentVal) / baselineVal) * 100.0;
    } else {
        improvement = ((currentVal - baselineVal) / baselineVal) * 100.0;
    }
    
    if (std::abs(improvement) > 0.1) {
        QString improvementText = QString("%1%2%")
            .arg(improvement > 0 ? "+" : "")
            .arg(improvement, 0, 'f', 1);
        
        QColor improvementColor = improvement > 0 ? QColor(0x4A, 0xDE, 0x80) : QColor(0xF8, 0x71, 0x71);
        painter.setPen(improvementColor);
        QFont diffFont = font();
        diffFont.setPointSize(9);
        diffFont.setBold(true);
        painter.setFont(diffFont);
        painter.drawText(QRect(w - padding - 65, y, 65, barHeight),
                        Qt::AlignRight | Qt::AlignVCenter, improvementText);
    }
}

QColor ComparisonWidget::getPerformanceColor(double current, double baseline, bool lowerIsBetter) const {
    bool isBetter = lowerIsBetter ? (current < baseline) : (current > baseline);
    
    if (isBetter) {
        return QColor(0x4A, 0xDE, 0x80); // 绿色 - 性能更好
    } else if (std::abs(current - baseline) / baseline < 0.05) {
        return QColor(0x60, 0xA5, 0xFA); // 蓝色 - 基本持平
    } else {
        return QColor(0xFB, 0xBF, 0x24); // 黄色 - 性能下降
    }
}
