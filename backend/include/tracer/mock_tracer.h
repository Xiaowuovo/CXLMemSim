/**
 * @file mock_tracer.h
 * @brief Mock tracer for development in virtual machines
 *
 * This tracer generates simulated memory access events for testing
 * and development purposes when hardware sampling is not available.
 */

#pragma once

#include "tracer_interface.h"
#include <random>
#include <atomic>

namespace cxlsim {

/**
 * @brief Mock tracer implementation
 *
 * Supports two modes:
 * 1. Generate random simulated events
 * 2. Replay events from a pre-recorded trace file
 */
class MockTracer : public ITracer {
public:
    MockTracer();
    ~MockTracer() override = default;

    // ITracer interface
    bool initialize(pid_t target_pid) override;
    bool start() override;
    void stop() override;
    std::vector<MemoryAccessEvent> collect_samples() override;

    std::string name() const override { return "MockTracer"; }
    std::string capabilities() const override {
        return "Simulated data for VM development";
    }
    bool supports_precise_address() const override { return true; }
    bool is_running() const override { return running_; }

    // Mock-specific methods

    /**
     * @brief Load trace from JSON file
     * @param json_path Path to trace file
     * @return true on success
     */
    bool load_trace_file(const std::string& json_path);

    /**
     * @brief Configure simulation parameters
     * @param l3_miss_rate Probability of L3 miss (0.0 - 1.0)
     * @param avg_latency_ns Average latency in nanoseconds
     */
    void set_simulation_params(double l3_miss_rate, uint64_t avg_latency_ns);

    /**
     * @brief Set address range for generated events
     * @param start_addr Start of virtual address range
     * @param end_addr End of virtual address range
     */
    void set_address_range(uint64_t start_addr, uint64_t end_addr);

    /**
     * @brief Enable/disable loop mode for trace playback
     * @param enable If true, trace will loop when reaching end
     */
    void set_loop_mode(bool enable) { loop_mode_ = enable; }

private:
    // State
    std::atomic<bool> running_{false};
    pid_t target_pid_{-1};

    // Preloaded trace
    std::vector<MemoryAccessEvent> preloaded_trace_;
    size_t current_index_{0};
    bool loop_mode_{false};

    // Random generation parameters
    std::mt19937_64 rng_;
    double l3_miss_rate_{0.05};  // 5% miss rate by default
    uint64_t avg_latency_ns_{100};
    uint64_t addr_start_{0x1000000};
    uint64_t addr_end_{0x10000000};

    // Helper methods
    MemoryAccessEvent generate_random_event();
    uint64_t get_current_timestamp_ns() const;
};

} // namespace cxlsim
