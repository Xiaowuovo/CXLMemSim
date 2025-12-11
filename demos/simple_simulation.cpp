/**
 * @file simple_simulation.cpp
 * @brief Simple end-to-end simulation demo
 *
 * This demo shows how to:
 * 1. Create a configuration
 * 2. Initialize the analyzer
 * 3. Set up a tracer
 * 4. Run simulation for a few epochs
 * 5. Print statistics
 */

#include <iostream>
#include <memory>
#include <unistd.h>
#include "config_parser.h"
#include "analyzer/timing_analyzer.h"
#include "tracer/tracer_interface.h"
#include "tracer/mock_tracer.h"

using namespace cxlsim;

int main(int argc, char** argv) {
    std::cout << "======================================" << std::endl;
    std::cout << "CXLMemSim - Simple Simulation Demo" << std::endl;
    std::cout << "======================================\n" << std::endl;

    // Step 1: Create configuration
    std::cout << "[Step 1] Creating configuration..." << std::endl;

    auto config = ConfigParser::create_default_config();
    config.name = "Demo Configuration";

    // Adjust simulation parameters
    config.simulation.epoch_ms = 100;  // 100ms epochs
    config.simulation.enable_congestion_model = true;
    config.simulation.enable_mlp_optimization = false;

    std::cout << "  Configuration: " << config.name << std::endl;
    std::cout << "  CXL devices: " << config.cxl_devices.size() << std::endl;
    std::cout << "  Epoch duration: " << config.simulation.epoch_ms << " ms\n" << std::endl;

    // Step 2: Initialize TimingAnalyzer
    std::cout << "[Step 2] Initializing TimingAnalyzer..." << std::endl;

    TimingAnalyzer analyzer;
    if (!analyzer.initialize(config)) {
        std::cerr << "Failed to initialize analyzer!" << std::endl;
        return 1;
    }

    std::cout << std::endl;

    // Step 3: Create and configure tracer
    std::cout << "[Step 3] Setting up Tracer..." << std::endl;

    auto tracer_unique = TracerFactory::create(TracerFactory::TracerType::MOCK);
    if (!tracer_unique) {
        std::cerr << "Failed to create tracer!" << std::endl;
        return 1;
    }

    std::cout << "  Using: " << tracer_unique->name() << std::endl;

    // Configure mock tracer with higher miss rate for demo
    if (auto* mock = dynamic_cast<MockTracer*>(tracer_unique.get())) {
        mock->set_simulation_params(0.3, 200.0);  // 30% miss rate, 200ns avg latency
        std::cout << "  Configured: 30% L3 miss rate, 200ns avg latency" << std::endl;
    }

    if (!tracer_unique->initialize(getpid())) {
        std::cerr << "Failed to initialize tracer!" << std::endl;
        return 1;
    }

    // Convert to shared_ptr
    std::shared_ptr<ITracer> tracer = std::move(tracer_unique);
    analyzer.set_tracer(tracer);
    std::cout << std::endl;

    // Step 4: Configure address mappings
    std::cout << "[Step 4] Configuring address mappings..." << std::endl;

    // Map address range 0x10000000-0x20000000 to CXL0
    AddressMapping mapping;
    mapping.start_addr = 0x10000000;
    mapping.end_addr = 0x20000000;
    mapping.device_id = config.cxl_devices[0].id;

    analyzer.add_address_mapping(mapping);

    std::cout << "  Mapped: 0x" << std::hex << mapping.start_addr
              << "-0x" << mapping.end_addr << std::dec
              << " -> " << mapping.device_id << "\n" << std::endl;

    // Step 5: Print topology
    std::cout << "[Step 5] Topology:" << std::endl;
    analyzer.get_topology().print_topology();

    // Step 6: Run simulation for a few epochs
    std::cout << "[Step 6] Running simulation..." << std::endl;
    std::cout << "  Running 5 epochs...\n" << std::endl;

    // Configure analyzer for verbose output
    TimingAnalyzerConfig analyzer_config;
    analyzer_config.epoch_duration_ms = 100;
    analyzer_config.enable_injection = false;  // Disable actual injection for demo
    analyzer_config.verbose_logging = true;
    analyzer.set_config(analyzer_config);

    for (int i = 0; i < 5; ++i) {
        std::cout << "\n--- Epoch " << i << " ---" << std::endl;

        auto stats = analyzer.run_one_epoch();

        std::cout << "Results:" << std::endl;
        std::cout << "  Total accesses: " << stats.total_accesses << std::endl;
        std::cout << "  L3 misses: " << stats.l3_misses;
        if (stats.total_accesses > 0) {
            std::cout << " (" << (100.0 * stats.l3_misses / stats.total_accesses) << "%)";
        }
        std::cout << std::endl;
        std::cout << "  CXL accesses: " << stats.cxl_accesses << std::endl;

        if (stats.cxl_accesses > 0) {
            std::cout << "  Average latency: " << stats.avg_latency_ns << " ns" << std::endl;
        }
    }

    // Step 7: Print final summary
    std::cout << "\n======================================" << std::endl;
    std::cout << "Simulation Complete!" << std::endl;
    std::cout << "======================================" << std::endl;

    std::cout << "\nKey Observations:" << std::endl;
    std::cout << "- MockTracer generated simulated memory accesses" << std::endl;
    std::cout << "- TimingAnalyzer calculated CXL latencies" << std::endl;
    std::cout << "- Latency includes: base + link + protocol overhead" << std::endl;
    std::cout << "- Congestion model was enabled (but no congestion in demo)" << std::endl;

    std::cout << "\nNext Steps:" << std::endl;
    std::cout << "1. Try modifying the configuration (add switches, devices)" << std::endl;
    std::cout << "2. Simulate congestion by updating link loads" << std::endl;
    std::cout << "3. On physical hardware, use PEBSTracer for real data" << std::endl;
    std::cout << "4. Enable delay injection to slow down target process" << std::endl;

    return 0;
}
