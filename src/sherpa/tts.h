#ifndef TTS_SHERPA_TTS_H_
#define TTS_SHERPA_TTS_H_

#include <string>

#include "sherpa-onnx/c-api/cxx-api.h"

class SherpaTTS {
public:
    virtual void LoadModel(const std::string& model_path, const std::string& device = "cpu") = 0;
    virtual GeneratedAudio Generate(const std::string& text) = 0;
};

#endif // TTS_SHERPA_TTS_H_