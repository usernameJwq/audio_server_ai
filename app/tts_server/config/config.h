#ifndef TTS_CONFIG_H_
#define TTS_CONFIG_H_

#include <string>

#include "convert.h"

class Config {
public:
    Config();
    ~Config();

public:
    static ConfigProperty load_config(const std::string& cfg_path);
};

#endif
