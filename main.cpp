/**
 * @file main.cpp
 * @brief CXLMemSim command-line interface (for testing)
 *
 * This is a simple CLI tool for testing the backend.
 * The Qt GUI will be in frontend/main.cpp
 */

#include <iostream>
#include <cstring>
#include <unistd.h>
#include "tracer/tracer_interface.h"
#include "tracer/mock_tracer.h"

using namespace cxlsim;

void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [OPTIONS]\n"
              << "\nOptions:\n"
              << "  --check-tracer        Check available tracer type\n"
              << "  --test-mock           Test mock tracer\n"
              << "  --load-trace FILE     Load and replay trace file\n"
              << "  --help                Show this help\n"
              << std::endl;
}

void check_tracer() {
    std::cout << "\n=== Tracer Detection ===\n" << std::endl;

    // Check PEBS
    bool pebs = TracerFactory::is_pebs_available();
    std::cout << "PEBS Support: " << (pebs ? "✓ Available" : "✗ Not available") << std::endl;

    // Auto-detect
    auto type = TracerFactory::detect_best_tracer();
    std::cout << "Best Tracer: " << TracerFactory::type_to_string(type) << std::endl;

    // Create tracer
    auto tracer = TracerFactory::create();
    if (tracer) {
        std::cout << "\nTracer Info:" << std::endl;
        std::cout << "  Name: " << tracer->name() << std::endl;
        std::cout << "  Capabilities: " << tracer->capabilities() << std::endl;
        std::cout << "  Precise Address: " << (tracer->supports_precise_address() ? "Yes" : "No") << std::endl;
    }

    std::cout << std::endl;
}

void test_mock_tracer() {
    std::cout << "\n=== Testing Mock Tracer ===\n" << std::endl;

    auto tracer = TracerFactory::create(TracerFactory::TracerType::MOCK);
    if (!tracer) {
        std::cerr << "Failed to create mock tracer!" << std::endl;
        return;
    }

    std::cout << "Initializing..." << std::endl;
    if (!tracer->initialize(getpid())) {
        std::cerr << "Failed to initialize!" << std::endl;
        return;
    }

    std::cout << "Starting tracer..." << std::endl;
    if (!tracer->start()) {
        std::cerr << "Failed to start!" << std::endl;
        return;
    }

    std::cout << "Collecting samples..." << std::endl;
    for (int epoch = 0; epoch < 3; ++epoch) {
        auto samples = tracer->collect_samples();
        std::cout << "Epoch " << epoch << ": " << samples.size() << " samples" << std::endl;

        // Print first few samples
        int count = 0;
        for (const auto& sample : samples) {
            if (count++ >= 3) break;
            std::cout << "  [" << count << "] "
                      << "addr=0x" << std::hex << sample.virtual_addr << std::dec
                      << " miss=" << sample.is_l3_miss
                      << " latency=" << sample.latency_cycles << "ns"
                      << std::endl;
        }

        usleep(100000); // 100ms
    }

    std::cout << "Stopping tracer..." << std::endl;
    tracer->stop();

    std::cout << "✓ Test completed successfully\n" << std::endl;
}

void test_trace_file(const char* filename) {
    std::cout << "\n=== Loading Trace File ===\n" << std::endl;
    std::cout << "File: " << filename << std::endl;

    auto mock = std::make_unique<MockTracer>();

    if (!mock->load_trace_file(filename)) {
        std::cerr << "Failed to load trace file!" << std::endl;
        return;
    }

    mock->initialize(1234);
    mock->start();

    auto samples = mock->collect_samples();
    std::cout << "Loaded " << samples.size() << " events" << std::endl;

    // Print statistics
    int l3_misses = 0;
    uint64_t total_latency = 0;
    for (const auto& sample : samples) {
        if (sample.is_l3_miss) {
            l3_misses++;
            total_latency += sample.latency_cycles;
        }
    }

    if (l3_misses > 0) {
        std::cout << "L3 Misses: " << l3_misses << " ("
                  << (100.0 * l3_misses / samples.size()) << "%)" << std::endl;
        std::cout << "Avg Miss Latency: " << (total_latency / l3_misses) << "ns" << std::endl;
    }

    mock->stop();
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "CXLMemSim - CXL Memory Simulator\n"
              << "Version 1.0.0 (Development Build)\n" << std::endl;

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    std::string arg = argv[1];

    if (arg == "--help" || arg == "-h") {
        print_usage(argv[0]);
    }
    else if (arg == "--check-tracer") {
        check_tracer();
    }
    else if (arg == "--test-mock") {
        test_mock_tracer();
    }
    else if (arg == "--load-trace" && argc >= 3) {
        test_trace_file(argv[2]);
    }
    else {
        std::cerr << "Unknown option: " << arg << std::endl;
        print_usage(argv[0]);
        return 1;
    }

    return 0;
}
