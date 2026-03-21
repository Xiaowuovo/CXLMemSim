/**
 * @file experiment_manager.h
 * @brief 实验管理器 - 支持标准实验配置和数据导出
 */

#pragma once

#include "timing_analyzer.h"
#include <vector>
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>

namespace cxlsim {

/**
 * @brief 单个实验配置
 */
struct ExperimentConfig {
    std::string name;               // 实验名称
    std::string description;        // 实验描述
    
    // CXL设备参数
    uint64_t cxl_capacity_gb;
    double cxl_bandwidth_gbps;
    double cxl_base_latency_ns;
    
    // 模拟参数
    int epoch_duration_ms;
    bool enable_congestion;
    bool enable_mlp;
    
    // 运行参数
    int duration_seconds;           // 实验持续时间
    int repeat_count;               // 重复次数
};

/**
 * @brief 实验结果数据
 */
struct ExperimentResult {
    std::string experiment_name;
    std::string timestamp;
    
    // 配置参数
    ExperimentConfig config;
    
    // 统计数据
    std::vector<EpochStats> epoch_data;
    
    // 聚合统计
    uint64_t total_accesses;
    uint64_t total_cxl_accesses;
    double avg_latency_ns;
    double max_latency_ns;
    double min_latency_ns;
    double cxl_access_ratio;
    double total_injected_delay_ms;
};

/**
 * @brief 实验管理器
 */
class ExperimentManager {
public:
    ExperimentManager();
    ~ExperimentManager();
    
    /**
     * @brief 加载实验配置文件
     */
    bool load_experiment_configs(const std::string& config_file);
    
    /**
     * @brief 添加实验配置
     */
    void add_experiment(const ExperimentConfig& config);
    
    /**
     * @brief 运行单个实验
     */
    ExperimentResult run_experiment(const ExperimentConfig& config,
                                   TimingAnalyzer& analyzer);
    
    /**
     * @brief 运行所有实验
     */
    std::vector<ExperimentResult> run_all_experiments(TimingAnalyzer& analyzer);
    
    /**
     * @brief 导出结果到CSV
     */
    bool export_to_csv(const std::vector<ExperimentResult>& results,
                      const std::string& filename);
    
    /**
     * @brief 导出结果到JSON
     */
    bool export_to_json(const std::vector<ExperimentResult>& results,
                       const std::string& filename);
    
    /**
     * @brief 生成对比报告
     */
    std::string generate_comparison_report(const std::vector<ExperimentResult>& results);
    
    /**
     * @brief 创建标准实验集（用于毕设）
     */
    static std::vector<ExperimentConfig> create_standard_experiments();
    
private:
    std::vector<ExperimentConfig> experiments_;
    
    // 辅助函数
    void aggregate_stats(ExperimentResult& result);
    std::string format_timestamp();
};

/**
 * @brief 标准实验场景（参考学术界常用实验设计）
 */
namespace StandardExperiments {
    // 实验1: 延迟敏感性分析
    std::vector<ExperimentConfig> create_latency_sensitivity_experiments();
    
    // 实验2: 带宽瓶颈分析
    std::vector<ExperimentConfig> create_bandwidth_bottleneck_experiments();
    
    // 实验3: 容量扩展效益分析
    std::vector<ExperimentConfig> create_capacity_scaling_experiments();
    
    // 实验4: 拥塞模型对比
    std::vector<ExperimentConfig> create_congestion_model_experiments();
    
    // 实验5: MLP优化效果
    std::vector<ExperimentConfig> create_mlp_optimization_experiments();
}

} // namespace cxlsim
