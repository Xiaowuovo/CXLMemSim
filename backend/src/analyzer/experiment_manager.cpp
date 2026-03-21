/**
 * @file experiment_manager.cpp
 * @brief 实验管理器实现
 */

#include "analyzer/experiment_manager.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>

namespace cxlsim {

ExperimentManager::ExperimentManager() {}

ExperimentManager::~ExperimentManager() {}

bool ExperimentManager::load_experiment_configs(const std::string& config_file) {
    try {
        std::ifstream file(config_file);
        nlohmann::json j;
        file >> j;
        
        for (const auto& exp_json : j["experiments"]) {
            ExperimentConfig config;
            config.name = exp_json["name"];
            config.description = exp_json["description"];
            config.cxl_capacity_gb = exp_json["cxl_capacity_gb"];
            config.cxl_bandwidth_gbps = exp_json["cxl_bandwidth_gbps"];
            config.cxl_base_latency_ns = exp_json["cxl_base_latency_ns"];
            config.epoch_duration_ms = exp_json["epoch_duration_ms"];
            config.enable_congestion = exp_json["enable_congestion"];
            config.enable_mlp = exp_json["enable_mlp"];
            config.duration_seconds = exp_json["duration_seconds"];
            config.repeat_count = exp_json.value("repeat_count", 1);
            
            experiments_.push_back(config);
        }
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

void ExperimentManager::add_experiment(const ExperimentConfig& config) {
    experiments_.push_back(config);
}

ExperimentResult ExperimentManager::run_experiment(const ExperimentConfig& config,
                                                   TimingAnalyzer& analyzer) {
    ExperimentResult result;
    result.experiment_name = config.name;
    result.timestamp = format_timestamp();
    result.config = config;
    
    // TODO: 配置analyzer并运行实验
    // 这里需要根据config设置analyzer的参数
    // 然后运行指定时长，收集EpochStats
    
    // 聚合统计数据
    aggregate_stats(result);
    
    return result;
}

std::vector<ExperimentResult> ExperimentManager::run_all_experiments(TimingAnalyzer& analyzer) {
    std::vector<ExperimentResult> results;
    
    for (const auto& config : experiments_) {
        results.push_back(run_experiment(config, analyzer));
    }
    
    return results;
}

bool ExperimentManager::export_to_csv(const std::vector<ExperimentResult>& results,
                                     const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    
    // CSV Header
    file << "实验名称,时间戳,CXL容量(GB),带宽(GB/s),延迟(ns),"
         << "Epoch时长(ms),拥塞模型,MLP优化,"
         << "总访问次数,CXL访问次数,CXL访问比例(%),"
         << "平均延迟(ns),最大延迟(ns),最小延迟(ns),"
         << "总注入延迟(ms)\n";
    
    // Data rows
    for (const auto& result : results) {
        file << result.experiment_name << ","
             << result.timestamp << ","
             << result.config.cxl_capacity_gb << ","
             << result.config.cxl_bandwidth_gbps << ","
             << result.config.cxl_base_latency_ns << ","
             << result.config.epoch_duration_ms << ","
             << (result.config.enable_congestion ? "启用" : "禁用") << ","
             << (result.config.enable_mlp ? "启用" : "禁用") << ","
             << result.total_accesses << ","
             << result.total_cxl_accesses << ","
             << std::fixed << std::setprecision(2) << (result.cxl_access_ratio * 100) << ","
             << std::fixed << std::setprecision(2) << result.avg_latency_ns << ","
             << std::fixed << std::setprecision(2) << result.max_latency_ns << ","
             << std::fixed << std::setprecision(2) << result.min_latency_ns << ","
             << std::fixed << std::setprecision(3) << result.total_injected_delay_ms << "\n";
    }
    
    file.close();
    return true;
}

bool ExperimentManager::export_to_json(const std::vector<ExperimentResult>& results,
                                      const std::string& filename) {
    nlohmann::json j;
    j["experiment_set"] = {
        {"name", "CXL Memory Simulation Experiments"},
        {"timestamp", format_timestamp()},
        {"total_experiments", results.size()}
    };
    
    nlohmann::json experiments_json = nlohmann::json::array();
    
    for (const auto& result : results) {
        nlohmann::json exp;
        exp["name"] = result.experiment_name;
        exp["timestamp"] = result.timestamp;
        
        // Configuration
        exp["configuration"] = {
            {"cxl_capacity_gb", result.config.cxl_capacity_gb},
            {"bandwidth_gbps", result.config.cxl_bandwidth_gbps},
            {"base_latency_ns", result.config.cxl_base_latency_ns},
            {"epoch_duration_ms", result.config.epoch_duration_ms},
            {"congestion_enabled", result.config.enable_congestion},
            {"mlp_enabled", result.config.enable_mlp}
        };
        
        // Results
        exp["results"] = {
            {"total_accesses", result.total_accesses},
            {"cxl_accesses", result.total_cxl_accesses},
            {"cxl_access_ratio", result.cxl_access_ratio},
            {"avg_latency_ns", result.avg_latency_ns},
            {"max_latency_ns", result.max_latency_ns},
            {"min_latency_ns", result.min_latency_ns},
            {"total_injected_delay_ms", result.total_injected_delay_ms}
        };
        
        // Epoch data
        nlohmann::json epochs = nlohmann::json::array();
        for (const auto& epoch : result.epoch_data) {
            epochs.push_back({
                {"epoch_number", epoch.epoch_number},
                {"total_accesses", epoch.total_accesses},
                {"cxl_accesses", epoch.cxl_accesses},
                {"avg_latency_ns", epoch.avg_latency_ns}
            });
        }
        exp["epoch_details"] = epochs;
        
        experiments_json.push_back(exp);
    }
    
    j["experiments"] = experiments_json;
    
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    
    file << j.dump(2);
    file.close();
    
    return true;
}

std::string ExperimentManager::generate_comparison_report(
    const std::vector<ExperimentResult>& results) {
    
    std::ostringstream report;
    
    report << "=================================================\n";
    report << "         CXL 内存模拟实验对比报告\n";
    report << "=================================================\n\n";
    
    report << "实验时间: " << format_timestamp() << "\n";
    report << "实验总数: " << results.size() << "\n\n";
    
    // 表头
    report << std::left << std::setw(20) << "实验名称"
           << std::right << std::setw(12) << "延迟(ns)"
           << std::setw(12) << "带宽(GB/s)"
           << std::setw(15) << "平均延迟(ns)"
           << std::setw(15) << "CXL访问(%)"
           << std::setw(12) << "性能下降\n";
    report << std::string(86, '-') << "\n";
    
    // 找基准（假设第一个是基准）
    double baseline_latency = results.empty() ? 0 : results[0].avg_latency_ns;
    
    for (const auto& result : results) {
        double perf_degradation = baseline_latency > 0 
            ? ((result.avg_latency_ns - baseline_latency) / baseline_latency * 100)
            : 0;
        
        report << std::left << std::setw(20) << result.experiment_name
               << std::right << std::setw(12) << std::fixed << std::setprecision(0) 
               << result.config.cxl_base_latency_ns
               << std::setw(12) << std::fixed << std::setprecision(0)
               << result.config.cxl_bandwidth_gbps
               << std::setw(15) << std::fixed << std::setprecision(2)
               << result.avg_latency_ns
               << std::setw(14) << std::fixed << std::setprecision(1)
               << (result.cxl_access_ratio * 100) << "%"
               << std::setw(11) << std::fixed << std::setprecision(1)
               << perf_degradation << "%\n";
    }
    
    report << "\n=================================================\n";
    
    return report.str();
}

void ExperimentManager::aggregate_stats(ExperimentResult& result) {
    if (result.epoch_data.empty()) {
        result.total_accesses = 0;
        result.total_cxl_accesses = 0;
        result.avg_latency_ns = 0;
        result.max_latency_ns = 0;
        result.min_latency_ns = 0;
        result.cxl_access_ratio = 0;
        result.total_injected_delay_ms = 0;
        return;
    }
    
    result.total_accesses = 0;
    result.total_cxl_accesses = 0;
    double sum_latency = 0;
    result.max_latency_ns = 0;
    result.min_latency_ns = std::numeric_limits<double>::max();
    result.total_injected_delay_ms = 0;
    
    for (const auto& epoch : result.epoch_data) {
        result.total_accesses += epoch.total_accesses;
        result.total_cxl_accesses += epoch.cxl_accesses;
        sum_latency += epoch.avg_latency_ns * epoch.total_accesses;
        result.max_latency_ns = std::max(result.max_latency_ns, epoch.avg_latency_ns);
        result.min_latency_ns = std::min(result.min_latency_ns, epoch.avg_latency_ns);
        result.total_injected_delay_ms += epoch.total_injected_delay_ns / 1e6;
    }
    
    result.avg_latency_ns = result.total_accesses > 0 
        ? sum_latency / result.total_accesses 
        : 0;
    
    result.cxl_access_ratio = result.total_accesses > 0
        ? static_cast<double>(result.total_cxl_accesses) / result.total_accesses
        : 0;
}

std::string ExperimentManager::format_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::vector<ExperimentConfig> ExperimentManager::create_standard_experiments() {
    std::vector<ExperimentConfig> experiments;
    
    // 基准实验
    experiments.push_back({
        "基准-纯本地内存",
        "仅使用本地DDR内存，不使用CXL",
        0, 64, 90, 10, false, false, 60, 3
    });
    
    // 延迟敏感性
    experiments.push_back({
        "延迟-Gen5标准",
        "CXL Gen5 标准延迟170ns",
        128, 64, 170, 10, true, false, 60, 3
    });
    
    experiments.push_back({
        "延迟-Gen4模拟",
        "CXL Gen4 延迟250ns",
        128, 64, 250, 10, true, false, 60, 3
    });
    
    experiments.push_back({
        "延迟-远距离",
        "远距离或多跳场景350ns",
        128, 64, 350, 10, true, false, 60, 3
    });
    
    // 带宽瓶颈
    experiments.push_back({
        "带宽-64GB/s",
        "Gen5 x16 全带宽",
        128, 64, 170, 10, true, false, 60, 3
    });
    
    experiments.push_back({
        "带宽-32GB/s",
        "Gen5 x8 或 Gen4 x16",
        128, 32, 170, 10, true, false, 60, 3
    });
    
    experiments.push_back({
        "带宽-16GB/s",
        "Gen4 x8 低带宽",
        128, 16, 170, 10, true, false, 60, 3
    });
    
    return experiments;
}

namespace StandardExperiments {

std::vector<ExperimentConfig> create_latency_sensitivity_experiments() {
    return {
        {"延迟-90ns", "本地DDR基准", 0, 64, 90, 10, false, false, 30, 3},
        {"延迟-170ns", "CXL Gen5", 128, 64, 170, 10, true, false, 30, 3},
        {"延迟-250ns", "CXL Gen4", 128, 64, 250, 10, true, false, 30, 3},
        {"延迟-350ns", "远距离", 128, 64, 350, 10, true, false, 30, 3},
        {"延迟-500ns", "极端场景", 128, 64, 500, 10, true, false, 30, 3}
    };
}

std::vector<ExperimentConfig> create_bandwidth_bottleneck_experiments() {
    return {
        {"带宽-64GB/s", "Gen5 x16", 128, 64, 170, 10, true, false, 30, 3},
        {"带宽-48GB/s", "中等带宽", 128, 48, 170, 10, true, false, 30, 3},
        {"带宽-32GB/s", "Gen5 x8", 128, 32, 170, 10, true, false, 30, 3},
        {"带宽-16GB/s", "Gen4 x8", 128, 16, 170, 10, true, false, 30, 3},
        {"带宽-8GB/s", "低带宽", 128, 8, 170, 10, true, false, 30, 3}
    };
}

std::vector<ExperimentConfig> create_capacity_scaling_experiments() {
    return {
        {"容量-64GB", "小容量", 64, 64, 170, 10, true, false, 30, 3},
        {"容量-128GB", "标准容量", 128, 64, 170, 10, true, false, 30, 3},
        {"容量-256GB", "大容量", 256, 64, 170, 10, true, false, 30, 3},
        {"容量-512GB", "超大容量", 512, 64, 170, 10, true, false, 30, 3}
    };
}

std::vector<ExperimentConfig> create_congestion_model_experiments() {
    return {
        {"拥塞-禁用", "不考虑拥塞", 128, 32, 170, 10, false, false, 30, 3},
        {"拥塞-启用", "启用拥塞模型", 128, 32, 170, 10, true, false, 30, 3}
    };
}

std::vector<ExperimentConfig> create_mlp_optimization_experiments() {
    return {
        {"MLP-禁用", "无MLP优化", 128, 64, 170, 10, true, false, 30, 3},
        {"MLP-启用", "启用MLP优化", 128, 64, 170, 10, true, true, 30, 3}
    };
}

} // namespace StandardExperiments

} // namespace cxlsim
