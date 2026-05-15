/**
 * @file mock_tracer.cpp
 * @brief Mock tracer implementation
 */

#include "tracer/mock_tracer.h"
#include <fstream>
#include <chrono>
#include <iostream>
#include <nlohmann/json.hpp>

namespace cxlsim {

MockTracer::MockTracer()
    : rng_(std::random_device{}()) {
}

bool MockTracer::initialize(pid_t target_pid) {
    target_pid_ = target_pid;
    current_index_ = 0;
    std::cout << "[MockTracer] Initialized for PID " << target_pid << std::endl;
    std::cout << "[MockTracer] L3 miss rate: " << (l3_miss_rate_ * 100) << "%" << std::endl;
    return true;
}

bool MockTracer::start() {
    if (running_) {
        std::cerr << "[MockTracer] Already running!" << std::endl;
        return false;
    }

    running_ = true;
    current_index_ = 0;

    if (!preloaded_trace_.empty()) {
        std::cout << "[MockTracer] Started in replay mode ("
                  << preloaded_trace_.size() << " events)" << std::endl;
    } else {
        std::cout << "[MockTracer] Started in generation mode" << std::endl;
    }

    return true;
}

void MockTracer::stop() {
    running_ = false;
    std::cout << "[MockTracer] Stopped" << std::endl;
}

std::vector<MemoryAccessEvent> MockTracer::collect_samples() {
    if (!running_) {
        return {};
    }

    std::vector<MemoryAccessEvent> samples;

    // Mode 1: Replay from preloaded trace
    if (!preloaded_trace_.empty()) {
        size_t remaining = preloaded_trace_.size() - current_index_;
        size_t batch_size = std::min(size_t(1000), remaining);

        if (batch_size > 0) {
            samples.insert(samples.end(),
                          preloaded_trace_.begin() + current_index_,
                          preloaded_trace_.begin() + current_index_ + batch_size);
            current_index_ += batch_size;
        }

        // Loop mode
        if (current_index_ >= preloaded_trace_.size() && loop_mode_) {
            current_index_ = 0;
        }

        return samples;
    }

    // Mode 2: Generate random events
    // Scale batch size by thread count so more threads = more traffic
    std::uniform_int_distribution<> count_dist(100 * num_threads_, 500 * num_threads_);
    int num_samples = count_dist(rng_);

    samples.reserve(num_samples);
    for (int i = 0; i < num_samples; ++i) {
        samples.push_back(generate_random_event());
    }

    return samples;
}

bool MockTracer::load_trace_file(const std::string& json_path) {
    try {
        std::ifstream file(json_path);
        if (!file.is_open()) {
            std::cerr << "[MockTracer] Failed to open: " << json_path << std::endl;
            return false;
        }

        nlohmann::json j;
        file >> j;

        preloaded_trace_.clear();

        if (j.contains("events")) {
            for (const auto& event : j["events"]) {
                MemoryAccessEvent e;
                e.timestamp_ns = event.value("timestamp", 0UL);
                e.virtual_addr = event.value("addr", 0UL);
                e.tid = event.value("tid", 0U);
                e.cpu = event.value("cpu", 0U);
                e.is_l3_miss = event.value("l3_miss", false);
                e.latency_cycles = event.value("latency_ns", 100UL);
                e.is_load = event.value("is_load", true);
                preloaded_trace_.push_back(e);
            }
        }

        std::cout << "[MockTracer] Loaded " << preloaded_trace_.size()
                  << " events from " << json_path << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "[MockTracer] Error loading trace: " << e.what() << std::endl;
        return false;
    }
}

void MockTracer::set_simulation_params(double l3_miss_rate, uint64_t avg_latency_ns) {
    l3_miss_rate_ = std::clamp(l3_miss_rate, 0.0, 1.0);
    avg_latency_ns_ = avg_latency_ns;
    std::cout << "[MockTracer] Params updated: miss_rate=" << (l3_miss_rate_ * 100)
              << "%, avg_latency=" << avg_latency_ns_ << "ns" << std::endl;
}

void MockTracer::set_address_range(uint64_t start_addr, uint64_t end_addr) {
    addr_start_ = start_addr;
    addr_end_ = end_addr;
}

MemoryAccessEvent MockTracer::generate_random_event() {
    MemoryAccessEvent e;

    e.timestamp_ns = get_current_timestamp_ns();

    // ── Generate address according to access pattern ──────────────
    uint64_t range = (addr_end_ > addr_start_) ? (addr_end_ - addr_start_) : 1;
    switch (access_pattern_) {
        case AccessPattern::SEQUENTIAL: {
            // Advance by one cache line each call, wrap around
            e.virtual_addr = (addr_start_ + sequential_cursor_) & ~0x3F;
            sequential_cursor_ = (sequential_cursor_ + 64) % range;
            break;
        }
        case AccessPattern::STRIDE: {
            // Advance by stride_bytes_ each call, wrap around
            e.virtual_addr = (addr_start_ + sequential_cursor_) & ~0x3F;
            sequential_cursor_ = (sequential_cursor_ + stride_bytes_) % range;
            break;
        }
        case AccessPattern::MIXED: {
            // Alternate between sequential and random
            std::uniform_real_distribution<> coin(0.0, 1.0);
            if (coin(rng_) < 0.5) {
                e.virtual_addr = (addr_start_ + sequential_cursor_) & ~0x3F;
                sequential_cursor_ = (sequential_cursor_ + 64) % range;
            } else {
                std::uniform_int_distribution<uint64_t> addr_dist(addr_start_, addr_end_);
                e.virtual_addr = addr_dist(rng_) & ~0x3F;
            }
            break;
        }
        case AccessPattern::RANDOM:
        default: {
            std::uniform_int_distribution<uint64_t> addr_dist(addr_start_, addr_end_);
            e.virtual_addr = addr_dist(rng_) & ~0x3F;
            break;
        }
    }

    e.tid = target_pid_ > 0 ? target_pid_ : 1234;
    e.cpu = 0;

    // ── Read/Write decision based on read_ratio ───────────────────
    std::uniform_real_distribution<> rw_dist(0.0, 1.0);
    e.is_load = (rw_dist(rng_) < read_ratio_);

    // ── L3 miss determination ─────────────────────────────────────
    std::uniform_real_distribution<> miss_dist(0.0, 1.0);
    e.is_l3_miss = (miss_dist(rng_) < l3_miss_rate_);

    if (e.is_l3_miss) {
        // L3 miss: higher latency with variation
        std::normal_distribution<> latency_dist(avg_latency_ns_, avg_latency_ns_ * 0.2);
        e.latency_cycles = std::max(uint64_t(50), uint64_t(latency_dist(rng_)));
    } else {
        // L3 hit: low latency
        std::uniform_int_distribution<uint64_t> hit_latency_dist(10, 30);
        e.latency_cycles = hit_latency_dist(rng_);
    }

    return e;
}

uint64_t MockTracer::get_current_timestamp_ns() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
}

} // namespace cxlsim
