#include <gtest/gtest.h>

#include <memory>

#include "config.h"

class YamlReadTest : public ::testing::Test {
protected:
    void SetUp() override { cfg_ = std::make_unique<config>(); }

    std::unique_ptr<config> cfg_ = nullptr;
};

TEST_F(YamlReadTest, read_yaml) {
    std::string yaml_path = "./config.yaml";

    auto property = cfg_->load_config(yaml_path);

    std::cout << "port: " << property.server.port << std::endl;
    std::cout << "model_path: " << property.model.model_path << std::endl;
    std::cout << "device: " << property.model.device << std::endl;
    std::cout << "log_level: " << property.log.log_level << std::endl;
}