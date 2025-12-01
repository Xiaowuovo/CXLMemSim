/**
 * @file tracer_interface.h
 * @brief Abstract interface for memory access tracers
 *
 * This abstraction allows the simulator to work in both:
 * - Virtual machines (using MockTracer)
 * - Physical hardware (using PEBSTracer)
 */

#pragma once

#include <vector>
#include <memory>
#include <string>
#include <cstdint>

namespace cxlsim {

/**
 * @brief Memory access event structure
 *
 * Represents a single memory access captured by the tracer
 */
struct MemoryAccessEvent {
    uint64_t timestamp_ns;      ///< Timestamp in nanoseconds
    uint64_t virtual_addr;      ///< Virtual address accessed
    uint64_t physical_addr;     ///< Physical address (if available)
    uint32_t tid;               ///< Thread ID
    uint32_t cpu;               ///< CPU core number
    uint64_t latency_cycles;    ///< Measured latency in cycles

    bool is_load;               ///< true=load, false=store
    bool is_l3_miss;            ///< Whether this was an L3 cache miss

    // Extended fields (may not be available in all tracers)
    uint64_t ip;                ///< Instruction pointer
    uint32_t data_source;       ///< Data source encoding (Intel PEBS)

    MemoryAccessEvent()
        : timestamp_ns(0), virtual_addr(0), physical_addr(0),
          tid(0), cpu(0), latency_cycles(0),
          is_load(true), is_l3_miss(false),
          ip(0), data_source(0) {}
};

/**
 * @brief Abstract tracer interface
 *
 * All tracer implementations must inherit from this interface
 */
class ITracer {
public:
    virtual ~ITracer() = default;

    /**
     * @brief Initialize the tracer for a target process
     * @param target_pid Process ID to trace (-1 for system-wide)
     * @return true on success
     */
    virtual bool initialize(pid_t target_pid) = 0;

    /**
     * @brief Start collecting samples
     * @return true on success
     */
    virtual bool start() = 0;

    /**
     * @brief Stop collecting samples
     */
    virtual void stop() = 0;

    /**
     * @brief Collect samples for current epoch
     * @return Vector of memory access events
     */
    virtual std::vector<MemoryAccessEvent> collect_samples() = 0;

    /**
     * @brief Get tracer implementation name
     */
    virtual std::string name() const = 0;

    /**
     * @brief Get tracer capabilities description
     */
    virtual std::string capabilities() const = 0;

    /**
     * @brief Check if tracer supports precise address sampling
     */
    virtual bool supports_precise_address() const = 0;

    /**
     * @brief Check if tracer is currently running
     */
    virtual bool is_running() const = 0;
};

/**
 * @brief Tracer factory - automatically selects best implementation
 */
class TracerFactory {
public:
    enum class TracerType {
        AUTO,       ///< Automatically detect best available
        PEBS,       ///< Intel PEBS (physical hardware only)
        SOFTWARE,   ///< Software-based sampling (VM compatible)
        MOCK        ///< Mock tracer for testing
    };

    /**
     * @brief Create a tracer instance
     * @param type Tracer type (AUTO to auto-detect)
     * @return Unique pointer to tracer, or nullptr on failure
     */
    static std::unique_ptr<ITracer> create(TracerType type = TracerType::AUTO);

    /**
     * @brief Detect the best available tracer
     * @return Best tracer type for current environment
     */
    static TracerType detect_best_tracer();

    /**
     * @brief Check if PEBS is available
     * @return true if PEBS sampling is supported
     */
    static bool is_pebs_available();

    /**
     * @brief Get string name for tracer type
     */
    static std::string type_to_string(TracerType type);
};

} // namespace cxlsim
