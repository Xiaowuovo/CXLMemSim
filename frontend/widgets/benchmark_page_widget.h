#ifndef BENCHMARK_PAGE_WIDGET_H
#define BENCHMARK_PAGE_WIDGET_H

#include <QWidget>
#include "benchmark_widget.h"
#include "analyzer/timing_analyzer.h"
#include <vector>

class QVBoxLayout;
class QScrollArea;

/**
 * @brief 基准测试独立页面
 *
 * 包含固定基准 + 运行测试（最近 N epoch）+ 对比表格
 */
class BenchmarkPageWidget : public QWidget {
    Q_OBJECT

public:
    explicit BenchmarkPageWidget(QWidget *parent = nullptr);
    ~BenchmarkPageWidget();

    // 传入 epoch 历史指针（由 MainWindow 持有）
    void setEpochHistory(const std::vector<cxlsim::EpochStats>* history);

    // 重置
    void reset();

signals:
    void baselineFixed(const BenchmarkWidget::BenchmarkStats& baseline);
    void baselineCleared();

private:
    void setupUI();

    BenchmarkWidget* benchmarkWidget_;
};

#endif // BENCHMARK_PAGE_WIDGET_H
