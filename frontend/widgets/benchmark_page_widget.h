#ifndef BENCHMARK_PAGE_WIDGET_H
#define BENCHMARK_PAGE_WIDGET_H

#include <QWidget>
#include "benchmark_widget.h"
#include "comparison_widget.h"
#include "common_types.h"

class QVBoxLayout;

/**
 * @brief 基准测试独立页面
 * 
 * 包含基准测试配置和性能对比视图
 */
class BenchmarkPageWidget : public QWidget {
    Q_OBJECT

public:
    explicit BenchmarkPageWidget(QWidget *parent = nullptr);
    ~BenchmarkPageWidget();

    // 更新实时数据
    void updateCurrentStats(const cxlsim::EpochStats& stats);
    
    // 重置
    void reset();

signals:
    void baselineFixed(const cxlsim::EpochStats& baseline);
    void baselineCleared();

private:
    void setupUI();

    // 组件
    BenchmarkWidget* benchmarkWidget_;
    ComparisonWidget* comparisonWidget_;
    
    // 当前数据
    cxlsim::EpochStats currentStats_;
};

#endif // BENCHMARK_PAGE_WIDGET_H
