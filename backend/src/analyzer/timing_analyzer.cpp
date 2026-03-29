/**
 * @file timing_analyzer.cpp
 * @brief Timing analyzer implementation
 */

#include "analyzer/timing_analyzer.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <algorithm>

namespace cxlsim {

TimingAnalyzer::TimingAnalyzer() {
}

TimingAnalyzer::~TimingAnalyzer() {
    stop();
}

bool TimingAnalyzer::initialize(const CXLSimConfig& config) {
    // Build topology from configuration
    if (!topology_.build_from_config(config)) {
        std::cerr << "[TimingAnalyzer] Failed to build topology" << std::endl;
        return false;
    }

    // Configure latency model
    LatencyModelParams latency_params;
    latency_params.enable_mlp = config.simulation.enable_mlp_optimization;
    latency_params.enable_congestion = config.simulation.enable_congestion_model;
    latency_params.protocol_overhead_ratio = config.simulation.protocol_overhead;
    latency_model_.set_params(latency_params);

    // Configure analyzer
    config_.epoch_duration_ms = config.simulation.epoch_ms;

    std::cout << "[TimingAnalyzer] Initialized successfully" << std::endl;
    std::cout << "  Epoch duration: " << config_.epoch_duration_ms << " ms" << std::endl;
    std::cout << "  MLP optimization: " << (latency_params.enable_mlp ? "ON" : "OFF") << std::endl;
    std::cout << "  Congestion model: " << (latency_params.enable_congestion ? "ON" : "OFF") << std::endl;

    return true;
}

void TimingAnalyzer::set_tracer(std::shared_ptr<ITracer> tracer) {
    tracer_ = tracer;
}

void TimingAnalyzer::add_address_mapping(const AddressMapping& mapping) {
    address_mappings_.push_back(mapping);

    if (config_.verbose_logging) {
        std::cout << "[TimingAnalyzer] Added mapping: 0x" << std::hex
                  << mapping.start_addr << "-0x" << mapping.end_addr << std::dec
                  << " -> " << mapping.device_id << std::endl;
    }
}

bool TimingAnalyzer::start() {
    if (running_) {
        std::cerr << "[TimingAnalyzer] Already running" << std::endl;
        return false;
    }

    if (!tracer_) {
        std::cerr << "[TimingAnalyzer] No tracer set" << std::endl;
        return false;
    }

    // Start tracer
    if (!tracer_->start()) {
        std::cerr << "[TimingAnalyzer] Failed to start tracer" << std::endl;
        return false;
    }

    running_ = true;
    total_epochs_ = 0;

    // Start analyzer thread
    analyzer_thread_ = std::thread(&TimingAnalyzer::analyzer_loop, this);

    std::cout << "[TimingAnalyzer] Started" << std::endl;
    return true;
}

void TimingAnalyzer::stop() {
    if (!running_) {
        return;
    }

    running_ = false;

    // Wait for thread to finish
    if (analyzer_thread_.joinable()) {
        analyzer_thread_.join();
    }

    // Stop tracer
    if (tracer_) {
        tracer_->stop();
    }

    std::cout << "[TimingAnalyzer] Stopped after " << total_epochs_ << " epochs" << std::endl;
}

void TimingAnalyzer::analyzer_loop() {
    std::cout << "[TimingAnalyzer] Analysis loop started" << std::endl;

    auto epoch_duration = std::chrono::milliseconds(config_.epoch_duration_ms);

    while (running_) {
        auto epoch_start = std::chrono::steady_clock::now();

        // Process one epoch
        process_epoch();

        total_epochs_++;

        // Sleep until next epoch
        auto elapsed = std::chrono::steady_clock::now() - epoch_start;
        auto sleep_time = epoch_duration - elapsed;

        if (sleep_time > std::chrono::milliseconds(0)) {
            std::this_thread::sleep_for(sleep_time);
        } else if (config_.verbose_logging) {
            std::cout << "[TimingAnalyzer] Warning: Epoch processing took longer than epoch duration" << std::endl;
        }
    }

    std::cout << "[TimingAnalyzer] Analysis loop ended" << std::endl;
}

void TimingAnalyzer::process_epoch() {
    // Reset stats for new epoch
    current_stats_ = EpochStats();
    current_stats_.epoch_number = total_epochs_;

    // Collect samples from tracer
    auto samples = tracer_->collect_samples();
    current_stats_.total_accesses = samples.size();

    if (samples.empty()) {
        return;
    }

    // Update bandwidth statistics
    update_bandwidth_stats(samples);

    // Process each sample and collect latencies for percentile calculation
    double total_latency = 0.0;
    std::vector<double> cxl_latencies; // 用于计算P95/P99

    for (const auto& sample : samples) {
        if (sample.is_l3_miss) {
            current_stats_.l3_misses++;
            
            // Check if this is a CXL access
            std::string device_id = find_device_for_address(sample.virtual_addr);
            if (!device_id.empty()) {
                current_stats_.cxl_accesses++;
                
                // Calculate latency
                double latency_ns = latency_model_.calculate_access_latency(sample, device_id, topology_);
                total_latency += latency_ns;
                cxl_latencies.push_back(latency_ns);
                
                // Inject delay if enabled
                if (config_.enable_injection) {
                    inject_delay(static_cast<uint64_t>(latency_ns));
                    current_stats_.total_injected_delay_ns += latency_ns;
                }
            } else {
                // Local DRAM access
                current_stats_.local_dram_accesses++;
            }
        }
    }

    // Calculate average latency
    if (current_stats_.cxl_accesses > 0) {
        current_stats_.avg_latency_ns = total_latency / current_stats_.cxl_accesses;
        
        // Calculate P95 and P99 latencies
        if (!cxl_latencies.empty()) {
            std::sort(cxl_latencies.begin(), cxl_latencies.end());
            size_t p95_idx = static_cast<size_t>(cxl_latencies.size() * 0.95);
            size_t p99_idx = static_cast<size_t>(cxl_latencies.size() * 0.99);
            
            if (p95_idx >= cxl_latencies.size()) p95_idx = cxl_latencies.size() - 1;
            if (p99_idx >= cxl_latencies.size()) p99_idx = cxl_latencies.size() - 1;
            
            current_stats_.p95_latency_ns = cxl_latencies[p95_idx];
            current_stats_.p99_latency_ns = cxl_latencies[p99_idx];
        }
        
        // Simulate queuing delay (10% of avg latency as congestion overhead)
        current_stats_.queuing_delay_ns = current_stats_.avg_latency_ns * 0.1;
    }

    // Calculate link utilization (simplified: based on access rate)
    double epoch_duration_s = config_.epoch_duration_ms / 1000.0;
    double bytes_per_epoch = current_stats_.cxl_accesses * 64.0; // 64 bytes per cache line
    double bandwidth_used_gbps = (bytes_per_epoch / epoch_duration_s) / 1e9;
    double max_bandwidth_gbps = 64.0; // CXL 2.0: 64 GB/s
    current_stats_.link_utilization_pct = std::min(100.0, (bandwidth_used_gbps / max_bandwidth_gbps) * 100.0);

    // Print stats periodically
    if (config_.verbose_logging && total_epochs_ % 10 == 0) {
        print_stats();
    }
}

void TimingAnalyzer::process_sample(const MemoryAccessEvent& event) {
    // Find which CXL device this address belongs to
    std::string device_id = find_device_for_address(event.virtual_addr);

    if (device_id.empty()) {
        // Not a CXL address, ignore
        return;
    }

    current_stats_.cxl_accesses++;

    // Calculate latency for this access
    double latency_ns = latency_model_.calculate_access_latency(event, device_id, topology_);

    // Inject delay if enabled
    if (config_.enable_injection) {
        inject_delay(static_cast<uint64_t>(latency_ns));
        current_stats_.total_injected_delay_ns += latency_ns;
    }
}

std::string TimingAnalyzer::find_device_for_address(uint64_t addr) const {
    for (const auto& mapping : address_mappings_) {
        if (mapping.contains(addr)) {
            return mapping.device_id;
        }
    }
    return "";
}

void TimingAnalyzer::update_bandwidth_stats(const std::vector<MemoryAccessEvent>& samples) {
    // Sample bandwidth usage based on sample rate
    if (static_cast<double>(rand()) / RAND_MAX > config_.bandwidth_sample_rate) {
        return;
    }

    // Reset all link loads
    topology_.reset_link_loads();

    // Count accesses per device
    std::map<std::string, int> device_access_count;

    for (const auto& sample : samples) {
        if (!sample.is_l3_miss) {
            continue;
        }

        std::string device_id = find_device_for_address(sample.virtual_addr);
        if (!device_id.empty()) {
            device_access_count[device_id]++;
        }
    }

    // Estimate bandwidth usage for each path
    // Assume 64 bytes per access (cache line)
    const double bytes_per_access = 64.0;
    double epoch_duration_s = config_.epoch_duration_ms / 1000.0;

    for (const auto& [device_id, count] : device_access_count) {
        double bandwidth_gbps = (count * bytes_per_access) / epoch_duration_s / 1e9;

        // Find path and update link loads
        const auto* rc = topology_.get_root_complex();
        if (rc) {
            auto path = const_cast<TopologyGraph&>(topology_).find_path(rc->id, device_id);

            // Update load on each link in the path
            for (const auto* edge : path.edges) {
                const_cast<TopologyGraph&>(topology_).update_link_load(
                    edge->from_id, edge->to_id, bandwidth_gbps);
            }
        }
    }
}

void TimingAnalyzer::inject_delay(uint64_t delay_ns) {
    if (delay_ns == 0) return;
    
    // 添加现实世界延迟抖动 (±3% 随机偏差)
    double jitter_factor = 1.0 + (static_cast<double>(rand()) / RAND_MAX - 0.5) * 0.06;
    uint64_t actual_delay = static_cast<uint64_t>(delay_ns * jitter_factor);
    
    // 混合策略：长延迟用sleep，短延迟用自旋锁
    const uint64_t SPIN_THRESHOLD = 10000;  // 10微秒
    
    if (actual_delay > SPIN_THRESHOLD) {
        // 大部分时间用sleep节省CPU
        uint64_t sleep_duration = actual_delay - SPIN_THRESHOLD;
        std::this_thread::sleep_for(std::chrono::nanoseconds(sleep_duration));
        actual_delay = SPIN_THRESHOLD;  // 剩余用自旋精确控制
    }
    
    // 高精度自旋锁微调
    auto start = std::chrono::high_resolution_clock::now();
    while (true) {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(now - start);
        if (static_cast<uint64_t>(elapsed.count()) >= actual_delay) {
            break;
        }
        // 避免过度占用CPU
        if (actual_delay - elapsed.count() > 100) {
            std::this_thread::yield();
        }
    }
}

EpochStats TimingAnalyzer::run_one_epoch() {
    process_epoch();
    return current_stats_;
}

void TimingAnalyzer::print_stats() const {
    std::cout << "\n[Epoch " << current_stats_.epoch_number << "] Statistics:" << std::endl;
    std::cout << "  Total accesses: " << current_stats_.total_accesses << std::endl;
    std::cout << "  L3 misses: " << current_stats_.l3_misses;
    if (current_stats_.total_accesses > 0) {
        std::cout << " (" << (100.0 * current_stats_.l3_misses / current_stats_.total_accesses) << "%)";
    }
    std::cout << std::endl;
    std::cout << "  CXL accesses: " << current_stats_.cxl_accesses << std::endl;
    std::cout << "  Avg latency: " << current_stats_.avg_latency_ns << " ns" << std::endl;
    std::cout << "  Total injected delay: " << (current_stats_.total_injected_delay_ns / 1e6) << " ms" << std::endl;
}

} // namespace cxlsim
