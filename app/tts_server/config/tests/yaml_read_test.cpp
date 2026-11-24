#include <gtest/gtest.h>

#include <memory>

#include "config.h"

class YamlReadTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(YamlReadTest, read_yaml) {
    std::string yaml_path = "./config.yaml";

    auto property = Config::load_config(yaml_path);

    std::cout << "port: " << property.server.port << std::endl;
    std::cout << "model_path: " << property.model.model_path << std::endl;
    std::cout << "device: " << property.model.device << std::endl;
    std::cout << "log_level: " << property.log.log_level << std::endl;
}