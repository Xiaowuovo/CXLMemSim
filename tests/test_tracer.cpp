/**
 * @file test_tracer.cpp
 * @brief Unit tests for tracer abstraction
 */

#include <gtest/gtest.h>
#include "tracer/tracer_interface.h"
#include "tracer/mock_tracer.h"

using namespace cxlsim;

TEST(TracerFactory, AutoDetection) {
    auto tracer = TracerFactory::create(TracerFactory::TracerType::AUTO);
    ASSERT_NE(tracer, nullptr) << "Factory should create a valid tracer";

    std::cout << "Using: " << tracer->name() << std::endl;
    std::cout << "Capabilities: " << tracer->capabilities() << std::endl;
}

TEST(TracerFactory, TypeToString) {
    EXPECT_EQ(TracerFactory::type_to_string(TracerFactory::TracerType::AUTO), "AUTO");
    EXPECT_EQ(TracerFactory::type_to_string(TracerFactory::TracerType::PEBS), "PEBS");
    EXPECT_EQ(TracerFactory::type_to_string(TracerFactory::TracerType::MOCK), "MOCK");
}

TEST(MockTracer, Creation) {
    auto tracer = TracerFactory::create(TracerFactory::TracerType::MOCK);
    ASSERT_NE(tracer, nullptr);
    EXPECT_EQ(tracer->name(), "MockTracer");
    EXPECT_TRUE(tracer->supports_precise_address());
}

TEST(MockTracer, BasicLifecycle) {
    auto tracer = TracerFactory::create(TracerFactory::TracerType::MOCK);
    ASSERT_TRUE(tracer->initialize(1234));
    EXPECT_FALSE(tracer->is_running());

    ASSERT_TRUE(tracer->start());
    EXPECT_TRUE(tracer->is_running());

    tracer->stop();
    EXPECT_FALSE(tracer->is_running());
}

TEST(MockTracer, SampleGeneration) {
    auto tracer = TracerFactory::create(TracerFactory::TracerType::MOCK);
    ASSERT_TRUE(tracer->initialize(1234));
    ASSERT_TRUE(tracer->start());

    auto samples = tracer->collect_samples();
    EXPECT_GT(samples.size(), 0) << "Should generate some samples";

    // Verify sample structure
    if (!samples.empty()) {
        const auto& sample = samples[0];
        EXPECT_GT(sample.timestamp_ns, 0);
        EXPECT_GT(sample.virtual_addr, 0);
        EXPECT_EQ(sample.virtual_addr % 64, 0) << "Address should be cache-line aligned";
    }

    tracer->stop();
}

TEST(MockTracer, TraceFileLoading) {
    auto mock_ptr = std::make_unique<MockTracer>();

    // Try to load the sample trace
    bool loaded = mock_ptr->load_trace_file("tests/data/sample_trace.json");

    if (loaded) {
        ASSERT_TRUE(mock_ptr->initialize(1234));
        ASSERT_TRUE(mock_ptr->start());

        auto samples = mock_ptr->collect_samples();
        EXPECT_GT(samples.size(), 0) << "Should load samples from file";

        mock_ptr->stop();
    } else {
        GTEST_SKIP() << "Sample trace file not found, skipping";
    }
}

TEST(MockTracer, SimulationParams) {
    auto mock_ptr = std::make_unique<MockTracer>();

    // Configure high miss rate
    mock_ptr->set_simulation_params(0.5, 200);

    ASSERT_TRUE(mock_ptr->initialize(1234));
    ASSERT_TRUE(mock_ptr->start());

    auto samples = mock_ptr->collect_samples();
    ASSERT_GT(samples.size(), 10) << "Need enough samples to test";

    // Count misses
    int miss_count = 0;
    for (const auto& sample : samples) {
        if (sample.is_l3_miss) {
            miss_count++;
            EXPECT_GT(sample.latency_cycles, 50) << "L3 miss should have high latency";
        }
    }

    // With 50% miss rate, expect roughly half to be misses
    double actual_miss_rate = double(miss_count) / samples.size();
    EXPECT_GT(actual_miss_rate, 0.3) << "Miss rate should be significant";
    EXPECT_LT(actual_miss_rate, 0.7) << "Miss rate should not be too high";

    mock_ptr->stop();
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
