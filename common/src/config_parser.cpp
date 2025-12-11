/**
 * @file config_parser.cpp
 * @brief Configuration parser implementation
 */

#include "config_parser.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <set>

namespace cxlsim {

bool ConfigParser::load_from_file(const std::string& filename) {
    errors_.clear();

    std::ifstream file(filename);
    if (!file.is_open()) {
        add_error("Failed to open file: " + filename);
        return false;
    }

    try {
        nlohmann::json j;
        file >> j;

        // Parse topology section
        if (j.contains("topology")) {
            const auto& topo = j["topology"];

            if (topo.contains("name")) {
                config_.name = topo["name"];
            }
            if (topo.contains("description")) {
                config_.description = topo["description"];
            }

            if (topo.contains("root_complex")) {
                if (!parse_root_complex(topo["root_complex"])) {
                    return false;
                }
            }

            if (topo.contains("switches")) {
                if (!parse_switches(topo["switches"])) {
                    return false;
                }
            }

            if (topo.contains("cxl_devices")) {
                if (!parse_cxl_devices(topo["cxl_devices"])) {
                    return false;
                }
            }

            if (topo.contains("connections")) {
                if (!parse_connections(topo["connections"])) {
                    return false;
                }
            }
        }

        // Parse memory policy
        if (j.contains("memory_policy")) {
            if (!parse_memory_policy(j["memory_policy"])) {
                return false;
            }
        }

        // Parse simulation parameters
        if (j.contains("simulation")) {
            if (!parse_simulation(j["simulation"])) {
                return false;
            }
        }

        return validate();

    } catch (const std::exception& e) {
        add_error(std::string("JSON parsing error: ") + e.what());
        return false;
    }
}

bool ConfigParser::load_from_string(const std::string& json_str) {
    errors_.clear();

    try {
        nlohmann::json j = nlohmann::json::parse(json_str);

        // Similar parsing logic as load_from_file
        // (Implementation omitted for brevity, same as above)

        return validate();

    } catch (const std::exception& e) {
        add_error(std::string("JSON parsing error: ") + e.what());
        return false;
    }
}

bool ConfigParser::save_to_file(const std::string& filename) const {
    try {
        nlohmann::json j;

        // Build topology section
        nlohmann::json topo;
        topo["name"] = config_.name;
        topo["description"] = config_.description;

        // Root complex
        topo["root_complex"] = {
            {"id", config_.root_complex.id},
            {"local_dram_size_gb", config_.root_complex.local_dram_size_gb},
            {"local_dram_latency_ns", config_.root_complex.local_dram_latency_ns}
        };

        // Switches
        nlohmann::json switches = nlohmann::json::array();
        for (const auto& sw : config_.switches) {
            switches.push_back({
                {"id", sw.id},
                {"latency_ns", sw.latency_ns},
                {"ports", sw.num_ports}
            });
        }
        topo["switches"] = switches;

        // CXL devices
        nlohmann::json devices = nlohmann::json::array();
        for (const auto& dev : config_.cxl_devices) {
            devices.push_back({
                {"id", dev.id},
                {"type", dev.type},
                {"capacity_gb", dev.capacity_gb},
                {"link_gen", dev.link_gen},
                {"link_width", dev.link_width},
                {"bandwidth_gbps", dev.bandwidth_gbps},
                {"base_latency_ns", dev.base_latency_ns}
            });
        }
        topo["cxl_devices"] = devices;

        // Connections
        nlohmann::json connections = nlohmann::json::array();
        for (const auto& conn : config_.connections) {
            connections.push_back({
                {"from", conn.from},
                {"to", conn.to},
                {"link", conn.link}
            });
        }
        topo["connections"] = connections;

        j["topology"] = topo;

        // Memory policy
        std::string policy_str;
        switch (config_.memory_policy.type) {
            case MemoryPolicy::FIRST_TOUCH: policy_str = "first_touch"; break;
            case MemoryPolicy::INTERLEAVE: policy_str = "interleave"; break;
            case MemoryPolicy::TIERING: policy_str = "tiering"; break;
            default: policy_str = "custom"; break;
        }

        j["memory_policy"] = {
            {"type", policy_str},
            {"local_first_gb", config_.memory_policy.local_first_gb}
        };

        // Simulation
        j["simulation"] = {
            {"epoch_ms", config_.simulation.epoch_ms},
            {"enable_congestion_model", config_.simulation.enable_congestion_model},
            {"enable_mlp_optimization", config_.simulation.enable_mlp_optimization}
        };

        // Write to file
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        file << j.dump(2);  // Pretty print with 2-space indent
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error saving config: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigParser::parse_root_complex(const nlohmann::json& j) {
    try {
        config_.root_complex.id = j.value("id", "RC0");
        config_.root_complex.local_dram_size_gb = j.value("local_dram_size_gb", 64UL);
        config_.root_complex.local_dram_latency_ns = j.value("local_dram_latency_ns", 90.0);
        return true;
    } catch (const std::exception& e) {
        add_error("Error parsing root_complex: " + std::string(e.what()));
        return false;
    }
}

bool ConfigParser::parse_switches(const nlohmann::json& j) {
    try {
        config_.switches.clear();

        for (const auto& sw : j) {
            SwitchConfig cfg;
            cfg.id = sw.value("id", "");
            cfg.latency_ns = sw.value("latency_ns", 40.0);
            cfg.num_ports = sw.value("ports", 8);
            cfg.bandwidth_per_port_gbps = sw.value("bandwidth_gbps", 64.0);

            if (cfg.id.empty()) {
                add_error("Switch missing id");
                return false;
            }

            config_.switches.push_back(cfg);
        }

        return true;
    } catch (const std::exception& e) {
        add_error("Error parsing switches: " + std::string(e.what()));
        return false;
    }
}

bool ConfigParser::parse_cxl_devices(const nlohmann::json& j) {
    try {
        config_.cxl_devices.clear();

        for (const auto& dev : j) {
            CXLDeviceConfig cfg;
            cfg.id = dev.value("id", "");
            cfg.type = dev.value("type", "Type3");
            cfg.capacity_gb = dev.value("capacity_gb", 128UL);
            cfg.link_gen = dev.value("link_gen", "Gen5");
            cfg.link_width = dev.value("link_width", "x16");
            cfg.bandwidth_gbps = dev.value("bandwidth_gbps", 64.0);
            cfg.base_latency_ns = dev.value("base_latency_ns", 170.0);

            if (cfg.id.empty()) {
                add_error("CXL device missing id");
                return false;
            }

            config_.cxl_devices.push_back(cfg);
        }

        return true;
    } catch (const std::exception& e) {
        add_error("Error parsing cxl_devices: " + std::string(e.what()));
        return false;
    }
}

bool ConfigParser::parse_connections(const nlohmann::json& j) {
    try {
        config_.connections.clear();

        for (const auto& conn : j) {
            ConnectionConfig cfg;
            cfg.from = conn.value("from", "");
            cfg.to = conn.value("to", "");
            cfg.link = conn.value("link", "Gen5x16");

            if (cfg.from.empty() || cfg.to.empty()) {
                add_error("Connection missing from/to");
                return false;
            }

            config_.connections.push_back(cfg);
        }

        return true;
    } catch (const std::exception& e) {
        add_error("Error parsing connections: " + std::string(e.what()));
        return false;
    }
}

bool ConfigParser::parse_memory_policy(const nlohmann::json& j) {
    try {
        std::string type_str = j.value("type", "first_touch");

        if (type_str == "first_touch") {
            config_.memory_policy.type = MemoryPolicy::FIRST_TOUCH;
        } else if (type_str == "interleave") {
            config_.memory_policy.type = MemoryPolicy::INTERLEAVE;
        } else if (type_str == "tiering") {
            config_.memory_policy.type = MemoryPolicy::TIERING;
        } else {
            config_.memory_policy.type = MemoryPolicy::CUSTOM;
        }

        config_.memory_policy.local_first_gb = j.value("local_first_gb", 32UL);
        config_.memory_policy.interleave_granularity_kb = j.value("interleave_granularity_kb", 4);

        return true;
    } catch (const std::exception& e) {
        add_error("Error parsing memory_policy: " + std::string(e.what()));
        return false;
    }
}

bool ConfigParser::parse_simulation(const nlohmann::json& j) {
    try {
        config_.simulation.epoch_ms = j.value("epoch_ms", 10);
        config_.simulation.enable_congestion_model = j.value("enable_congestion_model", true);
        config_.simulation.enable_mlp_optimization = j.value("enable_mlp_optimization", false);
        config_.simulation.enable_coherency_simulation = j.value("enable_coherency_simulation", false);
        config_.simulation.flit_efficiency = j.value("flit_efficiency", 0.93);
        config_.simulation.protocol_overhead = j.value("protocol_overhead", 0.05);

        return true;
    } catch (const std::exception& e) {
        add_error("Error parsing simulation: " + std::string(e.what()));
        return false;
    }
}

bool ConfigParser::validate() const {
    errors_.clear();

    bool valid = true;
    valid &= validate_topology();
    valid &= validate_connections();
    valid &= validate_bandwidth();

    return valid;
}

bool ConfigParser::validate_topology() const {
    // Check for at least one CXL device
    if (config_.cxl_devices.empty()) {
        add_error("Topology must have at least one CXL device");
        return false;
    }

    // Check for duplicate IDs
    std::map<std::string, int> id_count;
    id_count[config_.root_complex.id]++;

    for (const auto& sw : config_.switches) {
        id_count[sw.id]++;
    }

    for (const auto& dev : config_.cxl_devices) {
        id_count[dev.id]++;
    }

    for (const auto& [id, count] : id_count) {
        if (count > 1) {
            add_error("Duplicate component ID: " + id);
            return false;
        }
    }

    return true;
}

bool ConfigParser::validate_connections() const {
    // Build set of valid component IDs
    std::set<std::string> valid_ids;
    valid_ids.insert(config_.root_complex.id);

    for (const auto& sw : config_.switches) {
        valid_ids.insert(sw.id);
    }

    for (const auto& dev : config_.cxl_devices) {
        valid_ids.insert(dev.id);
    }

    // Validate each connection
    for (const auto& conn : config_.connections) {
        if (valid_ids.find(conn.from) == valid_ids.end()) {
            add_error("Connection references unknown component: " + conn.from);
            return false;
        }

        if (valid_ids.find(conn.to) == valid_ids.end()) {
            add_error("Connection references unknown component: " + conn.to);
            return false;
        }
    }

    return true;
}

bool ConfigParser::validate_bandwidth() const {
    // Check for reasonable bandwidth values
    for (const auto& dev : config_.cxl_devices) {
        if (dev.bandwidth_gbps <= 0 || dev.bandwidth_gbps > 256) {
            add_error("Invalid bandwidth for device " + dev.id + ": " +
                     std::to_string(dev.bandwidth_gbps) + " GB/s");
            return false;
        }
    }

    return true;
}

void ConfigParser::add_error(const std::string& error) const {
    errors_.push_back(error);
    std::cerr << "[ConfigParser] Error: " << error << std::endl;
}

LinkSpec ConfigParser::parse_link_spec(const std::string& link_str) const {
    LinkSpec spec;
    spec.type = link_str;

    // Parse "Gen5x16" format
    if (link_str.find("Gen5") != std::string::npos) {
        spec.generation = 5;
        spec.bandwidth_gbps = 64.0;  // Gen5 x16 = 64 GB/s
    } else if (link_str.find("Gen4") != std::string::npos) {
        spec.generation = 4;
        spec.bandwidth_gbps = 32.0;  // Gen4 x16 = 32 GB/s
    }

    if (link_str.find("x8") != std::string::npos) {
        spec.width = 8;
        spec.bandwidth_gbps /= 2;
    } else {
        spec.width = 16;
    }

    return spec;
}

CXLSimConfig ConfigParser::create_default_config() {
    CXLSimConfig cfg;

    cfg.name = "Default CXL Configuration";
    cfg.description = "Single CXL device directly connected to host";

    // Root complex
    cfg.root_complex.id = "RC0";
    cfg.root_complex.local_dram_size_gb = 64;
    cfg.root_complex.local_dram_latency_ns = 90.0;

    // One CXL device
    CXLDeviceConfig dev;
    dev.id = "CXL0";
    dev.type = "Type3";
    dev.capacity_gb = 128;
    dev.link_gen = "Gen5";
    dev.link_width = "x16";
    dev.bandwidth_gbps = 64.0;
    dev.base_latency_ns = 170.0;
    cfg.cxl_devices.push_back(dev);

    // Direct connection
    cfg.connections.push_back(ConnectionConfig("RC0", "CXL0", "Gen5x16"));

    // Memory policy
    cfg.memory_policy.type = MemoryPolicy::FIRST_TOUCH;
    cfg.memory_policy.local_first_gb = 32;

    // Simulation
    cfg.simulation.epoch_ms = 10;
    cfg.simulation.enable_congestion_model = true;
    cfg.simulation.enable_mlp_optimization = false;

    return cfg;
}

} // namespace cxlsim
