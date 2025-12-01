/**
 * @file pebs_tracer.h
 * @brief PEBS-based tracer for physical hardware
 *
 * This tracer uses Intel PEBS (Precise Event-Based Sampling)
 * Only available on physical hardware with Intel CPUs.
 *
 * @note This is a placeholder. Full implementation will be added
 *       when testing on physical hardware.
 */

#pragma once

#include "tracer_interface.h"

namespace cxlsim {

/**
 * @brief PEBS tracer implementation (physical hardware only)
 *
 * Uses Linux perf_event_open with PEBS to capture precise
 * memory access events with data linear addresses.
 */
class PEBSTracer : public ITracer {
public:
    PEBSTracer();
    ~PEBSTracer() override;

    bool initialize(pid_t target_pid) override;
    bool start() override;
    void stop() override;
    std::vector<MemoryAccessEvent> collect_samples() override;

    std::string name() const override { return "PEBSTracer"; }
    std::string capabilities() const override {
        return "Precise Event-Based Sampling with data address";
    }
    bool supports_precise_address() const override { return true; }
    bool is_running() const override { return running_; }

private:
    bool running_{false};
    pid_t target_pid_{-1};
    int perf_fd_{-1};
    void* mmap_buf_{nullptr};
    size_t mmap_size_{0};

    bool setup_perf_event();
    void cleanup();
    void parse_perf_buffer(std::vector<MemoryAccessEvent>& out);
};

} // namespace cxlsim
