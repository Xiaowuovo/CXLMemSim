/**
 * @file tracer_factory.cpp
 * @brief Tracer factory implementation
 */

#include "tracer/tracer_interface.h"
#include "tracer/mock_tracer.h"

#ifdef ENABLE_PEBS_TRACER
#include "tracer/pebs_tracer.h"
#endif

#include <sys/syscall.h>
#include <unistd.h>
#include <linux/perf_event.h>
#include <cstring>
#include <iostream>

namespace cxlsim {

bool TracerFactory::is_pebs_available() {
#ifdef ENABLE_PEBS_TRACER
    // Try to open a PEBS event to check support
    struct perf_event_attr pe;
    std::memset(&pe, 0, sizeof(pe));
    pe.type = PERF_TYPE_RAW;
    pe.size = sizeof(pe);
    pe.config = 0x1cd;  // MEM_LOAD_RETIRED.L3_MISS on Intel
    pe.sample_period = 10000;
    pe.sample_type = PERF_SAMPLE_ADDR | PERF_SAMPLE_TIME | PERF_SAMPLE_TID;
    pe.precise_ip = 2;  // Requires PEBS
    pe.disabled = 1;

    int fd = syscall(__NR_perf_event_open, &pe, -1, 0, -1, 0);
    if (fd >= 0) {
        close(fd);
        std::cout << "[TracerFactory] PEBS is available" << std::endl;
        return true;
    }

    std::cout << "[TracerFactory] PEBS not available (errno=" << errno << ")" << std::endl;
    return false;
#else
    std::cout << "[TracerFactory] PEBS support not compiled in" << std::endl;
    return false;
#endif
}

TracerFactory::TracerType TracerFactory::detect_best_tracer() {
    std::cout << "[TracerFactory] Auto-detecting best tracer..." << std::endl;

    // Priority 1: Try PEBS
    if (is_pebs_available()) {
        std::cout << "[TracerFactory] Selected: PEBS (hardware sampling)" << std::endl;
        return TracerType::PEBS;
    }

    // Priority 2: Fall back to Mock
    std::cout << "[TracerFactory] Selected: Mock (simulation mode)" << std::endl;
    std::cout << "[TracerFactory] This is expected in VM environments" << std::endl;
    return TracerType::MOCK;
}

std::unique_ptr<ITracer> TracerFactory::create(TracerType type) {
    if (type == TracerType::AUTO) {
        type = detect_best_tracer();
    }

    switch (type) {
#ifdef ENABLE_PEBS_TRACER
        case TracerType::PEBS:
            if (!is_pebs_available()) {
                std::cerr << "[TracerFactory] PEBS requested but not available!" << std::endl;
                std::cerr << "[TracerFactory] Falling back to Mock tracer" << std::endl;
                return std::make_unique<MockTracer>();
            }
            std::cout << "[TracerFactory] Creating PEBSTracer" << std::endl;
            return std::make_unique<PEBSTracer>();
#endif

        case TracerType::MOCK:
            std::cout << "[TracerFactory] Creating MockTracer" << std::endl;
            return std::make_unique<MockTracer>();

        case TracerType::SOFTWARE:
            std::cerr << "[TracerFactory] SoftwareTracer not yet implemented" << std::endl;
            std::cerr << "[TracerFactory] Falling back to Mock tracer" << std::endl;
            return std::make_unique<MockTracer>();

        default:
            std::cerr << "[TracerFactory] Unknown tracer type!" << std::endl;
            return nullptr;
    }
}

std::string TracerFactory::type_to_string(TracerType type) {
    switch (type) {
        case TracerType::AUTO: return "AUTO";
        case TracerType::PEBS: return "PEBS";
        case TracerType::SOFTWARE: return "SOFTWARE";
        case TracerType::MOCK: return "MOCK";
        default: return "UNKNOWN";
    }
}

} // namespace cxlsim
