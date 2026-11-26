#include <iostream>

#include "tts_server.h"
#include "config.h"
#include "logs.h"

int main(int argc, char* argv[]) {
    // 初始化log
    init_logger();

    // 读取相关配置
    std::string yaml_path = "./config.yaml";
    auto property = Config::load_config(yaml_path);

    // 启动服务
    Server serevr(property.model.model_path, property.server.port);
    serevr.run();
}