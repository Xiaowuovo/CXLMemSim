/**
 * @file test_config_parser.cpp
 * @brief Unit tests for configuration parser
 */

#include <gtest/gtest.h>
#include "config_parser.h"
#include <fstream>

using namespace cxlsim;

class ConfigParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser_ = std::make_unique<ConfigParser>();
    }

    void TearDown() override {
        parser_.reset();
    }

    std::unique_ptr<ConfigParser> parser_;
};

TEST_F(ConfigParserTest, DefaultConfig) {
    auto config = ConfigParser::create_default_config();

    EXPECT_EQ(config.name, "Default CXL Configuration");
    EXPECT_EQ(config.root_complex.id, "RC0");
    EXPECT_EQ(config.root_complex.local_dram_size_gb, 64);
    EXPECT_EQ(config.cxl_devices.size(), 1);
    EXPECT_EQ(config.cxl_devices[0].id, "CXL0");
    EXPECT_EQ(config.connections.size(), 1);
}

TEST_F(ConfigParserTest, LoadExampleFile) {
    bool loaded = parser_->load_from_file("configs/examples/simple_cxl.json");

    if (loaded) {
        const auto& config = parser_->get_config();
        EXPECT_FALSE(config.name.empty());
        EXPECT_GT(config.cxl_devices.size(), 0);

        std::cout << "Loaded config: " << config.name << std::endl;
        std::cout << "CXL devices: " << config.cxl_devices.size() << std::endl;
    } else {
        GTEST_SKIP() << "Example config file not found";
    }
}

TEST_F(ConfigParserTest, SaveAndLoad) {
    // Create and save config
    auto config = ConfigParser::create_default_config();
    config.name = "Test Configuration";

    ConfigParser saver;
    saver.get_config() = config;

    std::string temp_file = "/tmp/test_config.json";
    ASSERT_TRUE(saver.save_to_file(temp_file));

    // Load it back
    ConfigParser loader;
    ASSERT_TRUE(loader.load_from_file(temp_file));

    const auto& loaded = loader.get_config();
    EXPECT_EQ(loaded.name, "Test Configuration");
    EXPECT_EQ(loaded.root_complex.id, config.root_complex.id);

    // Cleanup
    std::remove(temp_file.c_str());
}

TEST_F(ConfigParserTest, Validation) {
    auto config = ConfigParser::create_default_config();
    parser_->get_config() = config;

    EXPECT_TRUE(parser_->validate());
}

TEST_F(ConfigParserTest, InvalidConfig_NoCXLDevice) {
    auto config = ConfigParser::create_default_config();
    config.cxl_devices.clear();  // Remove all devices

    parser_->get_config() = config;
    EXPECT_FALSE(parser_->validate());

    const auto& errors = parser_->get_errors();
    EXPECT_GT(errors.size(), 0);
}

TEST_F(ConfigParserTest, InvalidConfig_BadConnection) {
    auto config = ConfigParser::create_default_config();

    // Add connection to non-existent device
    config.connections.push_back(ConnectionConfig("RC0", "INVALID_DEVICE", "Gen5x16"));

    parser_->get_config() = config;
    EXPECT_FALSE(parser_->validate());
}

TEST_F(ConfigParserTest, MemoryPolicyParsing) {
    // Use default config and modify it
    auto config = ConfigParser::create_default_config();
    config.memory_policy.type = MemoryPolicy::INTERLEAVE;
    config.memory_policy.local_first_gb = 64;

    // Save and reload
    ConfigParser saver;
    saver.get_config() = config;

    std::string temp_file = "/tmp/test_policy.json";
    ASSERT_TRUE(saver.save_to_file(temp_file));

    // Load back
    ConfigParser loader;
    ASSERT_TRUE(loader.load_from_file(temp_file));

    const auto& loaded = loader.get_config();
    EXPECT_EQ(loaded.memory_policy.type, MemoryPolicy::INTERLEAVE);
    EXPECT_EQ(loaded.memory_policy.local_first_gb, 64);

    std::remove(temp_file.c_str());
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
