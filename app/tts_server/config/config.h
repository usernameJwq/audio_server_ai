#ifndef TTS_CONFIG_H_
#define TTS_CONFIG_H_

#include <string>

#include "convert.h"

class config {
public:
    config();
    ~config();

public:
    ConfigProperty load_config(const std::string& cfg_path);
};

#endif
