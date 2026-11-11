#ifndef TTS_SHERPA_TTS_H_
#define TTS_SHERPA_TTS_H_

#include <cstdint>
#include <string>
#include <vector>

struct TtsAudio {
    std::vector<float> samples;
    int32_t sample_rate = 0;
};

class SherpaTTS {
public:
    virtual ~SherpaTTS() = default;

    virtual void LoadModel(const std::string& model_path, const std::string& device = "cpu") = 0;

    virtual TtsAudio Generate(const std::string& text) = 0;
};

#endif // TTS_SHERPA_TTS_H_
