/**
 * @file latency_model.cpp
 * @brief Latency model implementation
 */

#include "analyzer/latency_model.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace cxlsim {

LatencyBreakdown LatencyModel::calculate_latency(const std::string& device_id,
                                                 const TopologyGraph& topology) const {
    LatencyBreakdown breakdown;

    // Get device node
    const auto* device = topology.get_node(device_id);
    if (!device || device->type != ComponentType::CXL_DEVICE) {
        std::cerr << "[LatencyModel] Invalid device: " << device_id << std::endl;
        return breakdown;
    }

    // Base device latency
    breakdown.base_latency_ns = device->data.cxl_device.base_latency_ns;

    // Find path from root complex to device
    const auto* rc = topology.get_root_complex();
    if (!rc) {
        std::cerr << "[LatencyModel] No root complex found" << std::endl;
        return breakdown;
    }

    auto path = const_cast<TopologyGraph&>(topology).find_path(rc->id, device_id);
    if (path.empty()) {
        std::cerr << "[LatencyModel] No path from RC to " << device_id << std::endl;
        return breakdown;
    }

    // Link latency (sum of all link traversals)
    breakdown.link_latency_ns = 0.0;
    for (const auto* edge : path.edges) {
        breakdown.link_latency_ns += edge->latency_ns;
    }

    // Switch processing latency
    breakdown.switch_latency_ns = 0.0;
    for (const auto& node_id : path.node_ids) {
        const auto* node = topology.get_node(node_id);
        if (node && node->type == ComponentType::SWITCH) {
            breakdown.switch_latency_ns += node->data.switch_data.processing_latency_ns;
        }
    }

    // Protocol overhead
    breakdown.protocol_overhead_ns = calculate_protocol_overhead(breakdown.base_latency_ns);

    // Congestion penalty (if enabled)
    if (params_.enable_congestion) {
        breakdown.congestion_penalty_ns = calculate_congestion_penalty(path, topology);
    }

    // Calculate total
    breakdown.calculate_total();

    return breakdown;
}

double LatencyModel::calculate_access_latency(const MemoryAccessEvent& event,
                                              const std::string& device_id,
                                              const TopologyGraph& topology) const {
    auto breakdown = calculate_latency(device_id, topology);

    // Apply MLP optimization if enabled
    if (params_.enable_mlp) {
        // For now, assume MLP degree of 1 (no parallelism)
        // This should be enhanced with actual MLP detection from LBR data
        return apply_mlp_optimization(breakdown.total_ns, 1);
    }

    return breakdown.total_ns;
}

double LatencyModel::apply_mlp_optimization(double base_latency, int mlp_degree) const {
    if (mlp_degree <= 1 || !params_.enable_mlp) {
        return base_latency;
    }

    // MLP can hide some latency through parallel memory accesses
    // Simplified model: effective_latency = base_latency / sqrt(mlp_degree)
    double mlp_factor = std::sqrt(static_cast<double>(mlp_degree));
    double effective_latency = base_latency / mlp_factor;

    // Apply user-configured MLP factor
    effective_latency *= params_.mlp_factor;

    return std::max(effective_latency, base_latency * 0.5);  // At least 50% of base latency
}

double LatencyModel::calculate_protocol_overhead(double base_latency) const {
    // CXL protocol overhead as a fraction of base latency
    return base_latency * params_.protocol_overhead_ratio;
}

double LatencyModel::calculate_congestion_penalty(const TopologyPath& path,
                                                  const TopologyGraph& topology) const {
    if (!params_.enable_congestion || path.empty()) {
        return 0.0;
    }

    double max_congestion_penalty = 0.0;

    // Check congestion on each link in the path
    for (const auto* edge : path.edges) {
        double load = topology.get_link_load(edge->from_id, edge->to_id);

        if (load > 0.7) {  // Congestion threshold at 70%
            // Exponential penalty as load approaches 1.0
            // penalty = base_delay * (1 + exp((load - 0.7) / 0.1))
            double normalized_load = (load - 0.7) / 0.3;  // Map 0.7-1.0 to 0-1
            double penalty_factor = 1.0 + std::exp(normalized_load * 3.0);
            double penalty = edge->latency_ns * (penalty_factor - 1.0);

            max_congestion_penalty = std::max(max_congestion_penalty, penalty);
        }
    }

    return max_congestion_penalty;
}

double LatencyModel::calculate_flit_overhead(double data_size_bytes, int pcie_gen) const {
    // CXL Flit overhead calculation
    double flit_size = (pcie_gen >= 5) ? 256.0 : 68.0;  // bytes
    double flit_efficiency = (pcie_gen >= 5) ? 0.93 : 0.90;

    // Calculate number of flits needed
    int num_flits = static_cast<int>(std::ceil(data_size_bytes / (flit_size * flit_efficiency)));

    // Each flit adds a small overhead (~1-2ns)
    return num_flits * 1.5;
}

double LatencyModel::calculate_snoop_overhead(bool requires_snoop) const {
    if (!requires_snoop) {
        return 0.0;
    }

    // CXL.cache Back-Invalidation overhead
    // Typical: 30-50ns for snoop broadcast and response
    return 40.0;
}

// LatencyCalculator utility functions

double LatencyCalculator::calculate_tlp_overhead(size_t payload_bytes) {
    // PCIe TLP overhead:
    // - Header: ~20 bytes
    // - LCRC: 4 bytes
    // - Encoding overhead: ~2%

    double overhead_bytes = 24.0;
    double total_bytes = payload_bytes + overhead_bytes;

    // Assume overhead translates to ~5% latency penalty
    return 0.05 * (total_bytes / payload_bytes);
}

} // namespace cxlsim
