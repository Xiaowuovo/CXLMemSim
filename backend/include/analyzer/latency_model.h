/**
 * @file latency_model.h
 * @brief Latency calculation model for CXL memory access
 *
 * Calculates the total latency for accessing CXL memory including:
 * - Base device latency
 * - Link traversal latency
 * - Switch processing latency
 * - Protocol overhead
 * - MLP (Memory Level Parallelism) optimization
 */

#pragma once

#include "topology/topology_graph.h"
#include "tracer/tracer_interface.h"
#include <cstdint>

namespace cxlsim {

/**
 * @brief Latency breakdown for analysis
 */
struct LatencyBreakdown {
    double base_latency_ns;         ///< Device base latency
    double link_latency_ns;         ///< Link traversal time
    double switch_latency_ns;       ///< Switch processing time
    double protocol_overhead_ns;    ///< CXL protocol overhead
    double congestion_penalty_ns;   ///< Congestion-induced delay
    double total_ns;                ///< Total latency

    LatencyBreakdown()
        : base_latency_ns(0), link_latency_ns(0), switch_latency_ns(0),
          protocol_overhead_ns(0), congestion_penalty_ns(0), total_ns(0) {}

    void calculate_total() {
        total_ns = base_latency_ns + link_latency_ns + switch_latency_ns +
                   protocol_overhead_ns + congestion_penalty_ns;
    }
};

/**
 * @brief Latency model parameters
 */
struct LatencyModelParams {
    double mlp_factor;              ///< MLP optimization factor (1.0 = no optimization)
    double protocol_overhead_ratio; ///< Protocol overhead as fraction of base latency
    bool enable_mlp;                ///< Enable MLP optimization
    bool enable_congestion;         ///< Enable congestion modeling

    LatencyModelParams()
        : mlp_factor(1.0), protocol_overhead_ratio(0.05),
          enable_mlp(false), enable_congestion(true) {}
};

/**
 * @brief Latency calculation model
 */
class LatencyModel {
public:
    LatencyModel() = default;
    explicit LatencyModel(const LatencyModelParams& params) : params_(params) {}
    ~LatencyModel() = default;

    /**
     * @brief Calculate latency for accessing a CXL device
     * @param device_id CXL device ID
     * @param topology Topology graph
     * @return Latency breakdown
     */
    LatencyBreakdown calculate_latency(const std::string& device_id,
                                       const TopologyGraph& topology) const;

    /**
     * @brief Calculate latency for a memory access event
     * @param event Memory access event
     * @param device_id Target CXL device
     * @param topology Topology graph
     * @return Calculated latency in nanoseconds
     */
    double calculate_access_latency(const MemoryAccessEvent& event,
                                   const std::string& device_id,
                                   const TopologyGraph& topology) const;

    /**
     * @brief Calculate latency with MLP consideration
     * @param base_latency Base latency without MLP
     * @param mlp_degree Number of parallel memory accesses
     * @return Effective latency with MLP
     */
    double apply_mlp_optimization(double base_latency, int mlp_degree) const;

    /**
     * @brief Set model parameters
     */
    void set_params(const LatencyModelParams& params) { params_ = params; }

    /**
     * @brief Get current parameters
     */
    const LatencyModelParams& get_params() const { return params_; }

    /**
     * @brief Calculate protocol overhead
     * @param base_latency Base latency
     * @return Overhead in nanoseconds
     */
    double calculate_protocol_overhead(double base_latency) const;

    /**
     * @brief Calculate congestion penalty
     * @param path Path to device
     * @param topology Topology graph
     * @return Congestion penalty in nanoseconds
     */
    double calculate_congestion_penalty(const TopologyPath& path,
                                       const TopologyGraph& topology) const;

private:
    LatencyModelParams params_;

    // Helper functions
    double calculate_flit_overhead(double data_size_bytes, int pcie_gen) const;
    double calculate_snoop_overhead(bool requires_snoop) const;
};

/**
 * @brief Simple latency calculator (utility class)
 */
class LatencyCalculator {
public:
    /**
     * @brief Calculate round-trip latency
     * @param one_way_latency One-way latency
     * @return Round-trip latency (2x one-way)
     */
    static double round_trip(double one_way_latency) {
        return one_way_latency * 2.0;
    }

    /**
     * @brief Convert cycles to nanoseconds
     * @param cycles Number of cycles
     * @param freq_ghz Frequency in GHz
     * @return Time in nanoseconds
     */
    static double cycles_to_ns(uint64_t cycles, double freq_ghz) {
        return static_cast<double>(cycles) / freq_ghz;
    }

    /**
     * @brief Convert nanoseconds to cycles
     * @param ns Time in nanoseconds
     * @param freq_ghz Frequency in GHz
     * @return Number of cycles
     */
    static uint64_t ns_to_cycles(double ns, double freq_ghz) {
        return static_cast<uint64_t>(ns * freq_ghz);
    }

    /**
     * @brief Calculate PCIe TLP (Transaction Layer Packet) overhead
     * @param payload_bytes Payload size in bytes
     * @return Overhead in nanoseconds (approximate)
     */
    static double calculate_tlp_overhead(size_t payload_bytes);

    /**
     * @brief Estimate CXL.cache snoop latency
     * @return Typical snoop latency in nanoseconds
     */
    static double estimate_snoop_latency() {
        return 30.0;  // Typical: 30-50ns
    }
};

} // namespace cxlsim
