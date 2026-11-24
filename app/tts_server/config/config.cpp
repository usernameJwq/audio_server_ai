#include "config.h"

#include <fstream>

#include "logs.h"

Config::Config() {}

Config::~Config() {}

ConfigProperty Config::load_config(const std::string& cfg_path) {
    std::ifstream fin(cfg_path);
    if (!fin.is_open()) {
        LOG::error("open yaml file failed");
        return ConfigProperty{};
    }

    YAML::Node root = YAML::Load(fin);

    ConfigProperty cfg = root.as<ConfigProperty>();

    if (fin.is_open()) {
        fin.close();
    }

    return cfg;
}