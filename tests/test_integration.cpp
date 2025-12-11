/**
 * @file test_integration.cpp
 * @brief Integration tests for complete workflow
 */

#include <gtest/gtest.h>
#include "config_parser.h"
#include "topology/topology_graph.h"
#include "analyzer/latency_model.h"

using namespace cxlsim;

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a simple test configuration
        config_ = ConfigParser::create_default_config();
        config_.name = "Integration Test Config";
    }

    CXLSimConfig config_;
};

TEST_F(IntegrationTest, EndToEndWorkflow) {
    // Step 1: Parse configuration
    ConfigParser parser;
    parser.get_config() = config_;
    ASSERT_TRUE(parser.validate());

    std::cout << "\n[Integration Test] Configuration validated" << std::endl;

    // Step 2: Build topology
    TopologyGraph topology;
    ASSERT_TRUE(topology.build_from_config(config_));

    std::cout << "[Integration Test] Topology built successfully" << std::endl;
    topology.print_topology();

    // Step 3: Verify topology
    const auto* rc = topology.get_root_complex();
    ASSERT_NE(rc, nullptr);
    EXPECT_EQ(rc->type, ComponentType::ROOT_COMPLEX);

    auto devices = topology.get_cxl_devices();
    ASSERT_GT(devices.size(), 0);
    std::cout << "[Integration Test] Found " << devices.size() << " CXL device(s)" << std::endl;

    // Step 4: Test path finding
    auto path = topology.find_path(rc->id, devices[0]->id);
    ASSERT_FALSE(path.empty());

    std::cout << "[Integration Test] Path from RC to " << devices[0]->id << ":" << std::endl;
    std::cout << "  Hops: " << path.hop_count() << std::endl;
    std::cout << "  Total latency: " << path.total_latency_ns << " ns" << std::endl;
    std::cout << "  Min bandwidth: " << path.min_bandwidth_gbps << " GB/s" << std::endl;

    EXPECT_GT(path.hop_count(), 0);
    EXPECT_GT(path.total_latency_ns, 0.0);
    EXPECT_GT(path.min_bandwidth_gbps, 0.0);

    // Step 5: Calculate latency using model
    LatencyModel latency_model;
    auto breakdown = latency_model.calculate_latency(devices[0]->id, topology);

    std::cout << "\n[Integration Test] Latency breakdown:" << std::endl;
    std::cout << "  Base latency: " << breakdown.base_latency_ns << " ns" << std::endl;
    std::cout << "  Link latency: " << breakdown.link_latency_ns << " ns" << std::endl;
    std::cout << "  Switch latency: " << breakdown.switch_latency_ns << " ns" << std::endl;
    std::cout << "  Protocol overhead: " << breakdown.protocol_overhead_ns << " ns" << std::endl;
    std::cout << "  Congestion penalty: " << breakdown.congestion_penalty_ns << " ns" << std::endl;
    std::cout << "  TOTAL: " << breakdown.total_ns << " ns" << std::endl;

    EXPECT_GT(breakdown.total_ns, 0.0);
    EXPECT_GT(breakdown.base_latency_ns, 0.0);
}

TEST_F(IntegrationTest, MultiDeviceTopology) {
    // Add a second CXL device
    CXLDeviceConfig dev2;
    dev2.id = "CXL1";
    dev2.type = "Type3";
    dev2.capacity_gb = 64;
    dev2.bandwidth_gbps = 32.0;  // Gen5 x8
    dev2.base_latency_ns = 180.0;
    config_.cxl_devices.push_back(dev2);

    // Add connection
    config_.connections.push_back(ConnectionConfig("RC0", "CXL1", "Gen5x8"));

    // Build topology
    TopologyGraph topology;
    ASSERT_TRUE(topology.build_from_config(config_));

    auto devices = topology.get_cxl_devices();
    EXPECT_EQ(devices.size(), 2);

    // Test paths to both devices
    const auto* rc = topology.get_root_complex();
    for (const auto* dev : devices) {
        auto path = topology.find_path(rc->id, dev->id);
        ASSERT_FALSE(path.empty());
        std::cout << "Path to " << dev->id << ": "
                  << path.hop_count() << " hops, "
                  << path.total_latency_ns << " ns" << std::endl;
    }
}

TEST_F(IntegrationTest, TopologyWithSwitch) {
    // Add a switch
    SwitchConfig sw;
    sw.id = "SW0";
    sw.latency_ns = 40.0;
    sw.num_ports = 8;
    config_.switches.push_back(sw);

    // Reconnect through switch
    config_.connections.clear();
    config_.connections.push_back(ConnectionConfig("RC0", "SW0", "Gen5x16"));
    config_.connections.push_back(ConnectionConfig("SW0", "CXL0", "Gen5x16"));

    // Build topology
    TopologyGraph topology;
    ASSERT_TRUE(topology.build_from_config(config_));

    const auto* rc = topology.get_root_complex();
    auto devices = topology.get_cxl_devices();

    auto path = topology.find_path(rc->id, devices[0]->id);
    ASSERT_FALSE(path.empty());
    EXPECT_EQ(path.hop_count(), 2);  // RC -> SW -> Device

    std::cout << "Path through switch:" << std::endl;
    for (const auto& node_id : path.node_ids) {
        std::cout << "  " << node_id << std::endl;
    }

    // Latency should include switch processing
    LatencyModel model;
    auto breakdown = model.calculate_latency(devices[0]->id, topology);

    EXPECT_GT(breakdown.switch_latency_ns, 0.0);
    std::cout << "Switch latency: " << breakdown.switch_latency_ns << " ns" << std::endl;
}

TEST_F(IntegrationTest, CongestionSimulation) {
    TopologyGraph topology;
    ASSERT_TRUE(topology.build_from_config(config_));

    // Simulate congestion on the link
    const auto* rc = topology.get_root_complex();
    auto devices = topology.get_cxl_devices();

    // Set high link load (80% of 64 GB/s = 51.2 GB/s)
    topology.update_link_load(rc->id, devices[0]->id, 51.2);

    // Calculate latency with congestion
    LatencyModelParams params;
    params.enable_congestion = true;

    LatencyModel model(params);
    auto breakdown = model.calculate_latency(devices[0]->id, topology);

    std::cout << "\nCongestion test:" << std::endl;
    std::cout << "  Link load: 80%" << std::endl;
    std::cout << "  Congestion penalty: " << breakdown.congestion_penalty_ns << " ns" << std::endl;

    // Should have some congestion penalty
    EXPECT_GT(breakdown.congestion_penalty_ns, 0.0);

    // Reset load and recalculate
    topology.reset_link_loads();
    auto breakdown_clean = model.calculate_latency(devices[0]->id, topology);

    // Latency should be lower without congestion
    EXPECT_LT(breakdown_clean.total_ns, breakdown.total_ns);
}

TEST_F(IntegrationTest, SaveLoadRoundtrip) {
    // Create and save config
    ConfigParser saver;
    saver.get_config() = config_;

    std::string temp_file = "/tmp/test_integration.json";
    ASSERT_TRUE(saver.save_to_file(temp_file));

    // Load back
    ConfigParser loader;
    ASSERT_TRUE(loader.load_from_file(temp_file));

    // Build topology from loaded config
    TopologyGraph topology;
    ASSERT_TRUE(topology.build_from_config(loader.get_config()));

    auto devices = topology.get_cxl_devices();
    EXPECT_GT(devices.size(), 0);

    // Cleanup
    std::remove(temp_file.c_str());

    std::cout << "Save/Load roundtrip successful" << std::endl;
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
