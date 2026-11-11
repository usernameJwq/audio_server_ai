#ifndef TTS_KOKORO_TTS_H_
#define TTS_KOKORO_TTS_H_

#include <string>

#include "sherpa-onnx/c-api/c-api.h"
#include "tts.h"

class KokoroTTS : public SherpaTTS {
public:
    KokoroTTS(const std::string& model_path, const std::string& device = "cpu");
    ~KokoroTTS() override;

    TtsAudio Generate(const std::string& text) override;

private:
    void LoadModel(const std::string& model_path, const std::string& device = "cpu") override;

private:
    const SherpaOnnxOfflineTts* tts_ = nullptr;
};

#endif // TTS_KOKORO_TTS_H_
