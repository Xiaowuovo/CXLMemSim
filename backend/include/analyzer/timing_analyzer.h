/**
 * @file timing_analyzer.h
 * @brief Core timing analyzer that integrates all components
 *
 * The TimingAnalyzer is the central component that:
 * - Collects memory access events from Tracer
 * - Maps addresses to CXL devices
 * - Calculates latencies using LatencyModel
 * - Updates topology load statistics
 * - Coordinates with Injector for delay injection
 */

#pragma once

#include "tracer/tracer_interface.h"
#include "topology/topology_graph.h"
#include "analyzer/latency_model.h"
#include "config_parser.h"
#include <memory>
#include <map>
#include <atomic>
#include <thread>

namespace cxlsim {

/**
 * @brief Address range mapping to CXL device
 */
struct AddressMapping {
    uint64_t start_addr;
    uint64_t end_addr;
    std::string device_id;

    bool contains(uint64_t addr) const {
        return addr >= start_addr && addr < end_addr;
    }
};

/**
 * @brief Per-epoch statistics
 */
struct EpochStats {
    uint64_t total_accesses;
    uint64_t l3_misses;
    uint64_t cxl_accesses;
    uint64_t local_dram_accesses;    ///< 本地DRAM访问次数（科研关键）
    double total_injected_delay_ns;
    double avg_latency_ns;
    double p95_latency_ns;           ///< P95尾延迟（抖动评估）
    double p99_latency_ns;           ///< P99尾延迟（最坏情况）
    double queuing_delay_ns;         ///< 拥塞排队延迟
    double link_utilization_pct;     ///< 链路平均利用率 0-100%
    double tiering_ratio;            ///< CXL访问占总L3 miss的比例 0-100%
    double local_dram_latency_ns;    ///< 本地DRAM参考延迟（用于对比）
    uint64_t epoch_number;

    EpochStats()
        : total_accesses(0), l3_misses(0), cxl_accesses(0), local_dram_accesses(0),
          total_injected_delay_ns(0), avg_latency_ns(0), 
          p95_latency_ns(0), p99_latency_ns(0), 
          queuing_delay_ns(0), link_utilization_pct(0),
          tiering_ratio(0), local_dram_latency_ns(90.0), epoch_number(0) {}
};

/**
 * @brief Timing analyzer configuration
 */
struct TimingAnalyzerConfig {
    int epoch_duration_ms;          ///< Epoch duration in milliseconds
    bool enable_injection;          ///< Enable actual delay injection
    bool verbose_logging;           ///< Enable detailed logging
    double bandwidth_sample_rate;   ///< How often to update bandwidth (0.0-1.0)
    double local_dram_latency_ns;   ///< Local DRAM latency for tiering comparison
    uint64_t total_local_dram_gb;   ///< Local DRAM capacity
    uint64_t total_cxl_gb;          ///< Total CXL capacity

    TimingAnalyzerConfig()
        : epoch_duration_ms(10), enable_injection(true),
          verbose_logging(false), bandwidth_sample_rate(0.1),
          local_dram_latency_ns(90.0), total_local_dram_gb(64), total_cxl_gb(0) {}
};

/**
 * @brief Core timing analyzer
 */
class TimingAnalyzer {
public:
    TimingAnalyzer();
    ~TimingAnalyzer();

    /**
     * @brief Initialize analyzer with configuration
     * @param config Simulation configuration
     * @return true on success
     */
    bool initialize(const CXLSimConfig& config);

    /**
     * @brief Set tracer instance
     * @param tracer Tracer to use for sampling
     */
    void set_tracer(std::shared_ptr<ITracer> tracer);

    /**
     * @brief Set analyzer configuration
     */
    void set_config(const TimingAnalyzerConfig& config) { config_ = config; }

    /**
     * @brief Add address range mapping
     * @param mapping Address to device mapping
     */
    void add_address_mapping(const AddressMapping& mapping);

    /**
     * @brief Start analysis
     * @return true on success
     */
    bool start();

    /**
     * @brief Stop analysis
     */
    void stop();

    /**
     * @brief Check if analyzer is running
     */
    bool is_running() const { return running_; }

    /**
     * @brief Run one epoch manually (for testing)
     * @return Epoch statistics
     */
    EpochStats run_one_epoch();

    /**
     * @brief Get current epoch statistics
     */
    const EpochStats& get_current_stats() const { return current_stats_; }

    /**
     * @brief Get topology graph (for inspection)
     */
    const TopologyGraph& get_topology() const { return topology_; }

    /**
     * @brief Get latency model (for configuration)
     */
    LatencyModel& get_latency_model() { return latency_model_; }

    /**
     * @brief Print current statistics
     */
    void print_stats() const;

private:
    // Components
    TopologyGraph topology_;
    LatencyModel latency_model_;
    std::shared_ptr<ITracer> tracer_;

    // Configuration
    TimingAnalyzerConfig config_;

    // Address mappings
    std::vector<AddressMapping> address_mappings_;

    // Runtime state
    std::atomic<bool> running_{false};
    std::thread analyzer_thread_;

    // Statistics
    EpochStats current_stats_;
    uint64_t total_epochs_{0};

    // Private methods
    void analyzer_loop();
    void process_epoch();
    void process_sample(const MemoryAccessEvent& event);
    std::string find_device_for_address(uint64_t addr) const;
    void update_bandwidth_stats(const std::vector<MemoryAccessEvent>& samples);
    void inject_delay(uint64_t delay_ns);
};

} // namespace cxlsim
