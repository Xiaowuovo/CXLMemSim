/**
 * @file config_parser.h
 * @brief Configuration file parser for CXL topology and simulation parameters
 *
 * Parses JSON configuration files to build topology and set simulation parameters.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <nlohmann/json.hpp>

namespace cxlsim {

/**
 * @brief CXL link specification
 */
struct LinkSpec {
    std::string type;           ///< Link type (e.g., "Gen5x16")
    int generation;             ///< PCIe generation (4 or 5)
    int width;                  ///< Link width (x8, x16, etc.)
    double bandwidth_gbps;      ///< Bandwidth in GB/s
    double latency_ns;          ///< Base latency in nanoseconds

    LinkSpec() : generation(5), width(16), bandwidth_gbps(64.0), latency_ns(0.0) {}
};

/**
 * @brief Root Complex configuration
 */
struct RootComplexConfig {
    std::string id;
    uint64_t local_dram_size_gb;
    double local_dram_latency_ns;

    RootComplexConfig()
        : id("RC0"), local_dram_size_gb(64), local_dram_latency_ns(90.0) {}
};

/**
 * @brief CXL Switch configuration
 */
struct SwitchConfig {
    std::string id;
    double latency_ns;          ///< Switch processing latency
    int num_ports;              ///< Number of ports
    double bandwidth_per_port_gbps;

    SwitchConfig()
        : latency_ns(40.0), num_ports(8), bandwidth_per_port_gbps(64.0) {}
};

/**
 * @brief CXL Device configuration
 */
struct CXLDeviceConfig {
    std::string id;
    std::string type;           ///< "Type1", "Type2", "Type3"
    uint64_t capacity_gb;
    std::string link_gen;       ///< "Gen4", "Gen5"
    std::string link_width;     ///< "x4", "x8", "x16"（科研关键：影响带宽瓶颈）
    double bandwidth_gbps;
    double base_latency_ns;     ///< Device-specific latency
    double media_latency_ns;    ///< 介质延迟（DRAM vs NVM，科研关键）

    // CXL.mem specific
    bool supports_hdm;          ///< Host-managed Device Memory
    bool supports_coherency;    ///< Device coherency support

    CXLDeviceConfig()
        : type("Type3"), capacity_gb(128), link_gen("Gen5"), link_width("x16"),
          bandwidth_gbps(64.0), base_latency_ns(170.0), media_latency_ns(90.0),
          supports_hdm(true), supports_coherency(false) {}
};

/**
 * @brief Topology connection between components
 */
struct ConnectionConfig {
    std::string from;           ///< Source component ID
    std::string to;             ///< Destination component ID
    std::string link;           ///< Link specification

    ConnectionConfig() = default;
    ConnectionConfig(const std::string& f, const std::string& t, const std::string& l)
        : from(f), to(t), link(l) {}
};

/**
 * @brief Memory allocation policy
 */
enum class MemoryPolicy {
    FIRST_TOUCH,    ///< Allocate on first-touch node
    INTERLEAVE,     ///< Interleave across all nodes
    TIERING,        ///< Hot data local, cold data CXL
    CUSTOM          ///< User-defined policy
};

/**
 * @brief Memory policy configuration
 */
struct MemoryPolicyConfig {
    MemoryPolicy type;
    uint64_t local_first_gb;    ///< Allocate first N GB locally
    int interleave_granularity_kb;

    MemoryPolicyConfig()
        : type(MemoryPolicy::FIRST_TOUCH),
          local_first_gb(32),
          interleave_granularity_kb(4) {}
};

/**
 * @brief Workload access pattern
 */
enum class AccessPattern {
    SEQUENTIAL,     ///< Sequential access
    RANDOM,         ///< Random access
    STRIDE,         ///< Strided access
    MIXED           ///< Mixed pattern
};

/**
 * @brief Workload configuration (科研关键)
 */
struct WorkloadConfig {
    // Traffic mode
    bool trace_driven;              ///< true=Trace file, false=Synthetic
    std::string trace_file_path;    ///< Path to trace file (CSV/TXT)
    
    // Synthetic traffic parameters
    AccessPattern access_pattern;   ///< Access pattern
    double read_ratio;              ///< Read percentage (0.0-1.0)
    double injection_rate_gbps;     ///< Traffic injection rate GB/s
    uint64_t working_set_gb;        ///< Working set size
    int stride_bytes;               ///< Stride size for STRIDE pattern
    
    // Temporal parameters
    double duration_sec;            ///< Workload duration (seconds)
    int num_threads;                ///< Number of concurrent threads
    
    WorkloadConfig()
        : trace_driven(false), trace_file_path(""),
          access_pattern(AccessPattern::RANDOM), read_ratio(0.7),
          injection_rate_gbps(10.0), working_set_gb(32), stride_bytes(4096),
          duration_sec(10.0), num_threads(1) {}
};

/**
 * @brief Simulation parameters
 */
struct SimulationConfig {
    int epoch_ms;                       ///< Epoch duration in milliseconds
    bool enable_congestion_model;       ///< Enable bandwidth congestion
    bool enable_mlp_optimization;       ///< Memory-level parallelism
    bool enable_coherency_simulation;   ///< CXL.cache coherency

    // Performance model parameters
    double flit_efficiency;             ///< Flit packing efficiency (0.9-1.0)
    double protocol_overhead;           ///< Protocol overhead ratio

    SimulationConfig()
        : epoch_ms(10), enable_congestion_model(true),
          enable_mlp_optimization(false), enable_coherency_simulation(false),
          flit_efficiency(0.93), protocol_overhead(0.05) {}
};

/**
 * @brief Complete configuration
 */
struct CXLSimConfig {
    std::string name;
    std::string description;

    RootComplexConfig root_complex;
    std::vector<SwitchConfig> switches;
    std::vector<CXLDeviceConfig> cxl_devices;
    std::vector<ConnectionConfig> connections;

    MemoryPolicyConfig memory_policy;
    WorkloadConfig workload;        ///< 负载配置（科研关键）
    SimulationConfig simulation;

    CXLSimConfig() = default;
};

/**
 * @brief Configuration parser
 */
class ConfigParser {
public:
    ConfigParser() = default;
    ~ConfigParser() = default;

    /**
     * @brief Load configuration from JSON file
     * @param filename Path to JSON config file
     * @return true on success
     */
    bool load_from_file(const std::string& filename);

    /**
     * @brief Load configuration from JSON string
     * @param json_str JSON string
     * @return true on success
     */
    bool load_from_string(const std::string& json_str);

    /**
     * @brief Save configuration to JSON file
     * @param filename Output file path
     * @return true on success
     */
    bool save_to_file(const std::string& filename) const;

    /**
     * @brief Get parsed configuration
     */
    const CXLSimConfig& get_config() const { return config_; }
    CXLSimConfig& get_config() { return config_; }

    /**
     * @brief Validate configuration
     * @return true if configuration is valid
     */
    bool validate() const;

    /**
     * @brief Get validation error messages
     */
    const std::vector<std::string>& get_errors() const { return errors_; }

    /**
     * @brief Create default configuration
     */
    static CXLSimConfig create_default_config();

private:
    CXLSimConfig config_;
    mutable std::vector<std::string> errors_;

    // Parsing helpers
    bool parse_root_complex(const nlohmann::json& j);
    bool parse_switches(const nlohmann::json& j);
    bool parse_cxl_devices(const nlohmann::json& j);
    bool parse_connections(const nlohmann::json& j);
    bool parse_memory_policy(const nlohmann::json& j);
    bool parse_simulation(const nlohmann::json& j);

    // Validation helpers
    bool validate_topology() const;
    bool validate_connections() const;
    bool validate_bandwidth() const;

    // Utility
    void add_error(const std::string& error) const;
    LinkSpec parse_link_spec(const std::string& link_str) const;
};

} // namespace cxlsim
