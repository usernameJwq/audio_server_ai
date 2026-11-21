#ifndef TTS_CONVERT_H_
#define TTS_CONVERT_H_

#include <string>

#include "yaml-cpp/yaml.h"

struct ConfigProperty {
    struct Server {
        int port;
    } server;

    struct Model {
        std::string model_path;
        std::string device;
    } model;

    struct Log {
        std::string log_level;
    } log;
};

// 对自定义结构体重写YAML解析

namespace YAML {

template <>
struct convert<ConfigProperty> {
    static bool decode(const Node& node, ConfigProperty& rhs) {
        if (!node.IsMap()) return false;
        if (node["server"]) rhs.server = node["server"].as<ConfigProperty::Server>();
        if (node["model"]) rhs.model = node["model"].as<ConfigProperty::Model>();
        if (node["log"]) rhs.log = node["log"].as<ConfigProperty::Log>();
        return true;
    }
};

template <>
struct convert<ConfigProperty::Server> {
    static bool decode(const Node& node, ConfigProperty::Server& rhs) {
        if (!node.IsMap()) return false;
        if (node["port"]) rhs.port = node["port"].as<int>();
        return true;
    }
};

template <>
struct convert<ConfigProperty::Model> {
    static bool decode(const Node& node, ConfigProperty::Model& rhs) {
        if (!node.IsMap()) return false;
        if (node["model_path"]) rhs.model_path = node["model_path"].as<std::string>();
        if (node["device"]) rhs.device = node["device"].as<std::string>();
        return true;
    }
};

template <>
struct convert<ConfigProperty::Log> {
    static bool decode(const Node& node, ConfigProperty::Log& rhs) {
        if (!node.IsMap()) return false;
        if (node["log_level"]) rhs.log_level = node["log_level"].as<std::string>();
        return true;
    }
};

} // namespace YAML

#endif