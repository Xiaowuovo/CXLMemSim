/**
 * @file topology_graph.cpp
 * @brief Topology graph implementation
 */

#include "topology/topology_graph.h"
#include <iostream>
#include <queue>
#include <limits>
#include <algorithm>
#include <set>

namespace cxlsim {

bool TopologyGraph::build_from_config(const CXLSimConfig& config) {
    nodes_.clear();
    edges_.clear();

    // Add root complex
    TopologyNode rc_node;
    rc_node.id = config.root_complex.id;
    rc_node.type = ComponentType::ROOT_COMPLEX;
    rc_node.data.root_complex.dram_size_gb = config.root_complex.local_dram_size_gb;
    rc_node.data.root_complex.dram_latency_ns = config.root_complex.local_dram_latency_ns;

    if (!add_node(rc_node)) {
        std::cerr << "[TopologyGraph] Failed to add root complex" << std::endl;
        return false;
    }

    root_complex_id_ = rc_node.id;

    // Add switches
    for (const auto& sw_cfg : config.switches) {
        TopologyNode sw_node;
        sw_node.id = sw_cfg.id;
        sw_node.type = ComponentType::SWITCH;
        sw_node.data.switch_data.processing_latency_ns = sw_cfg.latency_ns;
        sw_node.data.switch_data.num_ports = sw_cfg.num_ports;

        if (!add_node(sw_node)) {
            std::cerr << "[TopologyGraph] Failed to add switch: " << sw_cfg.id << std::endl;
            return false;
        }
    }

    // Add CXL devices
    for (const auto& dev_cfg : config.cxl_devices) {
        TopologyNode dev_node;
        dev_node.id = dev_cfg.id;
        dev_node.type = ComponentType::CXL_DEVICE;
        dev_node.data.cxl_device.capacity_gb = dev_cfg.capacity_gb;
        dev_node.data.cxl_device.base_latency_ns = dev_cfg.base_latency_ns;
        dev_node.data.cxl_device.supports_coherency = dev_cfg.supports_coherency;

        if (!add_node(dev_node)) {
            std::cerr << "[TopologyGraph] Failed to add CXL device: " << dev_cfg.id << std::endl;
            return false;
        }
    }

    // Add connections (edges)
    for (const auto& conn : config.connections) {
        TopologyEdge edge;
        edge.from_id = conn.from;
        edge.to_id = conn.to;

        // Parse link specification
        if (conn.link.find("Gen5") != std::string::npos) {
            edge.pcie_generation = 5;
            edge.bandwidth_gbps = 64.0;  // Gen5 x16
        } else if (conn.link.find("Gen4") != std::string::npos) {
            edge.pcie_generation = 4;
            edge.bandwidth_gbps = 32.0;  // Gen4 x16
        }

        if (conn.link.find("x8") != std::string::npos) {
            edge.link_width = 8;
            edge.bandwidth_gbps /= 2;
        } else {
            edge.link_width = 16;
        }

        edge.latency_ns = 10.0;  // Base PCIe link latency
        edge.current_load = 0.0;

        if (!add_edge(edge)) {
            std::cerr << "[TopologyGraph] Failed to add edge: "
                      << conn.from << " -> " << conn.to << std::endl;
            return false;
        }

        // Add reverse edge for bidirectional connectivity
        TopologyEdge reverse_edge = edge;
        reverse_edge.from_id = edge.to_id;
        reverse_edge.to_id = edge.from_id;
        add_edge(reverse_edge);
    }

    std::cout << "[TopologyGraph] Built topology with " << nodes_.size()
              << " nodes and " << edges_.size() << " edges" << std::endl;

    return validate();
}

bool TopologyGraph::add_node(const TopologyNode& node) {
    if (node.id.empty()) {
        return false;
    }

    if (nodes_.find(node.id) != nodes_.end()) {
        std::cerr << "[TopologyGraph] Duplicate node ID: " << node.id << std::endl;
        return false;
    }

    nodes_[node.id] = node;
    return true;
}

bool TopologyGraph::add_edge(const TopologyEdge& edge) {
    if (edge.from_id.empty() || edge.to_id.empty()) {
        return false;
    }

    auto key = std::make_pair(edge.from_id, edge.to_id);
    edges_[key] = edge;
    return true;
}

TopologyPath TopologyGraph::find_path(const std::string& from_id, const std::string& to_id) {
    return dijkstra(from_id, to_id);
}

TopologyPath TopologyGraph::dijkstra(const std::string& from, const std::string& to) {
    TopologyPath result;

    // Check if nodes exist
    if (nodes_.find(from) == nodes_.end() || nodes_.find(to) == nodes_.end()) {
        return result;  // Empty path
    }

    // Dijkstra's algorithm
    std::map<std::string, double> distances;
    std::map<std::string, std::string> previous;
    std::set<std::string> visited;

    // Initialize distances
    for (const auto& [id, _] : nodes_) {
        distances[id] = std::numeric_limits<double>::infinity();
    }
    distances[from] = 0.0;

    // Priority queue: (distance, node_id)
    using PQElement = std::pair<double, std::string>;
    std::priority_queue<PQElement, std::vector<PQElement>, std::greater<PQElement>> pq;
    pq.push({0.0, from});

    while (!pq.empty()) {
        auto [dist, current] = pq.top();
        pq.pop();

        if (visited.count(current)) {
            continue;
        }
        visited.insert(current);

        if (current == to) {
            break;  // Found shortest path
        }

        // Check all neighbors
        for (const auto& neighbor : get_neighbors(current)) {
            if (visited.count(neighbor)) {
                continue;
            }

            auto edge_key = std::make_pair(current, neighbor);
            if (edges_.find(edge_key) == edges_.end()) {
                continue;
            }

            const auto& edge = edges_[edge_key];
            double new_dist = distances[current] + edge.latency_ns;

            // Add switch processing latency if neighbor is a switch
            if (nodes_[neighbor].type == ComponentType::SWITCH) {
                new_dist += nodes_[neighbor].data.switch_data.processing_latency_ns;
            }

            if (new_dist < distances[neighbor]) {
                distances[neighbor] = new_dist;
                previous[neighbor] = current;
                pq.push({new_dist, neighbor});
            }
        }
    }

    // Reconstruct path
    if (distances[to] == std::numeric_limits<double>::infinity()) {
        return result;  // No path found
    }

    std::vector<std::string> path_nodes;
    std::string current = to;
    while (current != from) {
        path_nodes.push_back(current);
        if (previous.find(current) == previous.end()) {
            return TopologyPath();  // Path broken
        }
        current = previous[current];
    }
    path_nodes.push_back(from);
    std::reverse(path_nodes.begin(), path_nodes.end());

    // Build result
    result.node_ids = path_nodes;
    result.total_latency_ns = distances[to];
    result.min_bandwidth_gbps = std::numeric_limits<double>::max();

    // Collect edges and find minimum bandwidth
    for (size_t i = 0; i < path_nodes.size() - 1; ++i) {
        auto edge_key = std::make_pair(path_nodes[i], path_nodes[i + 1]);
        if (edges_.find(edge_key) != edges_.end()) {
            result.edges.push_back(&edges_[edge_key]);
            result.min_bandwidth_gbps = std::min(result.min_bandwidth_gbps,
                                                  edges_[edge_key].bandwidth_gbps);
        }
    }

    return result;
}

std::vector<std::string> TopologyGraph::get_neighbors(const std::string& node_id) const {
    std::vector<std::string> neighbors;

    for (const auto& [key, edge] : edges_) {
        if (key.first == node_id) {
            neighbors.push_back(key.second);
        }
    }

    return neighbors;
}

double TopologyGraph::calculate_path_latency(const TopologyPath& path) const {
    if (path.empty()) {
        return 0.0;
    }

    return path.total_latency_ns;
}

double TopologyGraph::get_path_bottleneck_bandwidth(const TopologyPath& path) const {
    if (path.empty()) {
        return 0.0;
    }

    return path.min_bandwidth_gbps;
}

const TopologyNode* TopologyGraph::get_node(const std::string& id) const {
    auto it = nodes_.find(id);
    if (it != nodes_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<const TopologyNode*> TopologyGraph::get_cxl_devices() const {
    std::vector<const TopologyNode*> devices;

    for (const auto& [id, node] : nodes_) {
        if (node.type == ComponentType::CXL_DEVICE) {
            devices.push_back(&node);
        }
    }

    return devices;
}

const TopologyNode* TopologyGraph::get_root_complex() const {
    if (root_complex_id_.empty()) {
        return nullptr;
    }
    return get_node(root_complex_id_);
}

bool TopologyGraph::validate() const {
    // Check connectivity
    if (!check_connectivity()) {
        std::cerr << "[TopologyGraph] Validation failed: not all nodes are connected" << std::endl;
        return false;
    }

    // Check for cycles (optional - CXL topologies should be DAGs or trees)
    // For now, we allow cycles since bidirectional links create them

    return true;
}

bool TopologyGraph::check_connectivity() const {
    if (nodes_.empty()) {
        return false;
    }

    // BFS from root complex to check if all CXL devices are reachable
    const auto* rc = get_root_complex();
    if (!rc) {
        return false;
    }

    std::set<std::string> visited;
    std::queue<std::string> q;
    q.push(rc->id);
    visited.insert(rc->id);

    while (!q.empty()) {
        std::string current = q.front();
        q.pop();

        for (const auto& neighbor : get_neighbors(current)) {
            if (visited.count(neighbor) == 0) {
                visited.insert(neighbor);
                q.push(neighbor);
            }
        }
    }

    // Check if all CXL devices are reachable
    for (const auto& [id, node] : nodes_) {
        if (node.type == ComponentType::CXL_DEVICE) {
            if (visited.count(id) == 0) {
                std::cerr << "[TopologyGraph] Device " << id << " is not reachable" << std::endl;
                return false;
            }
        }
    }

    return true;
}

bool TopologyGraph::check_cycles() const {
    // DFS-based cycle detection
    // Not critical for now
    return true;
}

void TopologyGraph::print_topology() const {
    std::cout << "\n=== Topology Graph ===" << std::endl;
    std::cout << "Nodes: " << nodes_.size() << std::endl;

    for (const auto& [id, node] : nodes_) {
        std::cout << "  " << id << " (";
        switch (node.type) {
            case ComponentType::ROOT_COMPLEX: std::cout << "Root Complex"; break;
            case ComponentType::SWITCH: std::cout << "Switch"; break;
            case ComponentType::CXL_DEVICE: std::cout << "CXL Device"; break;
        }
        std::cout << ")" << std::endl;
    }

    std::cout << "\nEdges: " << edges_.size() << std::endl;
    for (const auto& [key, edge] : edges_) {
        std::cout << "  " << key.first << " -> " << key.second
                  << " (BW: " << edge.bandwidth_gbps << " GB/s, "
                  << "Latency: " << edge.latency_ns << " ns)" << std::endl;
    }
    std::cout << "======================\n" << std::endl;
}

void TopologyGraph::reset_link_loads() {
    for (auto& [key, edge] : edges_) {
        edge.current_load = 0.0;
    }
}

void TopologyGraph::update_link_load(const std::string& from_id,
                                     const std::string& to_id,
                                     double bandwidth_used) {
    auto key = std::make_pair(from_id, to_id);
    if (edges_.find(key) != edges_.end()) {
        edges_[key].current_load = bandwidth_used / edges_[key].bandwidth_gbps;
        // Clamp to [0, 1]
        edges_[key].current_load = std::max(0.0, std::min(1.0, edges_[key].current_load));
    }
}

double TopologyGraph::get_link_load(const std::string& from_id,
                                    const std::string& to_id) const {
    auto key = std::make_pair(from_id, to_id);
    auto it = edges_.find(key);
    if (it != edges_.end()) {
        return it->second.current_load;
    }
    return 0.0;
}

} // namespace cxlsim
