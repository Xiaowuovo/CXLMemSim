/**
 * @file topology_graph.h
 * @brief Topology graph representation for CXL hierarchy
 *
 * Represents the CXL topology as a directed weighted graph for
 * path finding and bandwidth/latency calculations.
 */

#pragma once

#include "config_parser.h"
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <optional>

namespace cxlsim {

/**
 * @brief Component type in topology
 */
enum class ComponentType {
    ROOT_COMPLEX,
    SWITCH,
    CXL_DEVICE
};

/**
 * @brief Topology node (component)
 */
struct TopologyNode {
    std::string id;
    ComponentType type;

    // Component-specific data (using union for efficiency)
    union {
        struct {
            uint64_t dram_size_gb;
            double dram_latency_ns;
        } root_complex;

        struct {
            double processing_latency_ns;
            int num_ports;
        } switch_data;

        struct {
            uint64_t capacity_gb;
            double base_latency_ns;
            bool supports_coherency;
        } cxl_device;
    } data;

    TopologyNode() : type(ComponentType::ROOT_COMPLEX) {
        data.root_complex = {0, 0};
    }
};

/**
 * @brief Link between components
 */
struct TopologyEdge {
    std::string from_id;
    std::string to_id;

    // Link properties
    int pcie_generation;        ///< 4 or 5
    int link_width;             ///< 8 or 16
    double bandwidth_gbps;      ///< Maximum bandwidth
    double latency_ns;          ///< Link traversal latency

    // Runtime state
    double current_load;        ///< Current bandwidth usage (0.0-1.0)

    TopologyEdge()
        : pcie_generation(5), link_width(16), bandwidth_gbps(64.0),
          latency_ns(0.0), current_load(0.0) {}
};

/**
 * @brief Path in topology from source to destination
 */
struct TopologyPath {
    std::vector<std::string> node_ids;      ///< Path nodes
    std::vector<TopologyEdge*> edges;       ///< Path edges
    double total_latency_ns;                ///< Sum of all latencies
    double min_bandwidth_gbps;              ///< Minimum bandwidth along path

    TopologyPath() : total_latency_ns(0.0), min_bandwidth_gbps(0.0) {}

    bool empty() const { return node_ids.empty(); }
    size_t hop_count() const { return edges.size(); }
};

/**
 * @brief Topology graph manager
 */
class TopologyGraph {
public:
    TopologyGraph() = default;
    ~TopologyGraph() = default;

    /**
     * @brief Build topology from configuration
     * @param config Configuration to build from
     * @return true on success
     */
    bool build_from_config(const CXLSimConfig& config);

    /**
     * @brief Add a node to topology
     */
    bool add_node(const TopologyNode& node);

    /**
     * @brief Add an edge (link) between nodes
     */
    bool add_edge(const TopologyEdge& edge);

    /**
     * @brief Find path between two nodes
     * @param from_id Source node ID
     * @param to_id Destination node ID
     * @return Path object (empty if no path found)
     */
    TopologyPath find_path(const std::string& from_id, const std::string& to_id);

    /**
     * @brief Calculate latency for a path
     * @param path Path to calculate latency for
     * @return Total latency in nanoseconds
     */
    double calculate_path_latency(const TopologyPath& path) const;

    /**
     * @brief Get minimum bandwidth along a path
     * @param path Path to check
     * @return Minimum bandwidth in GB/s
     */
    double get_path_bottleneck_bandwidth(const TopologyPath& path) const;

    /**
     * @brief Get node by ID
     */
    const TopologyNode* get_node(const std::string& id) const;

    /**
     * @brief Get all CXL device nodes
     */
    std::vector<const TopologyNode*> get_cxl_devices() const;

    /**
     * @brief Check if topology is valid
     */
    bool validate() const;

    /**
     * @brief Get root complex node
     */
    const TopologyNode* get_root_complex() const;

    /**
     * @brief Print topology for debugging
     */
    void print_topology() const;

    /**
     * @brief Reset all link loads to 0
     */
    void reset_link_loads();

    /**
     * @brief Update link load
     * @param from_id Source node
     * @param to_id Destination node
     * @param bandwidth_used Bandwidth used in GB/s
     */
    void update_link_load(const std::string& from_id,
                         const std::string& to_id,
                         double bandwidth_used);

    /**
     * @brief Get current link load
     * @return Load as fraction of capacity (0.0-1.0)
     */
    double get_link_load(const std::string& from_id,
                        const std::string& to_id) const;

private:
    std::map<std::string, TopologyNode> nodes_;
    std::map<std::pair<std::string, std::string>, TopologyEdge> edges_;
    std::string root_complex_id_;

    // Path finding helpers
    TopologyPath dijkstra(const std::string& from, const std::string& to);
    std::vector<std::string> get_neighbors(const std::string& node_id) const;

    // Validation helpers
    bool check_connectivity() const;
    bool check_cycles() const;
};

} // namespace cxlsim
