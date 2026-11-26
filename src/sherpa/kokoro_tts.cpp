#include "kokoro_tts.h"

#include <cstring>
#include <stdexcept>
#include <string>

#include "fmt/format.h"
#include "logs.h"

KokoroTTS::KokoroTTS(const std::string& model_path, const std::string& device) {
    try {
        LoadModel(model_path, device);
    } catch (const std::exception& e) {
        LOG::error("Failed to load Kokoro TTS model: {}", e.what());
        throw;
    }
}

KokoroTTS::~KokoroTTS() {
    if (tts_ != nullptr) {
        SherpaOnnxDestroyOfflineTts(tts_);
        tts_ = nullptr;
    }
}

TtsAudio KokoroTTS::Generate(const std::string& text) {
    if (tts_ == nullptr) {
        throw std::runtime_error("Kokoro TTS model is not loaded");
    }

    SherpaOnnxGenerationConfig config;
    std::memset(&config, 0, sizeof(config));
    config.sid = 0;
    config.speed = 1.0f;
    config.silence_scale = 0.2f;

    const SherpaOnnxGeneratedAudio* audio =
        SherpaOnnxOfflineTtsGenerateWithConfig(tts_, text.c_str(), &config, nullptr, nullptr);
    if (audio == nullptr) {
        throw std::runtime_error("Failed to generate speech");
    }

    TtsAudio result;
    result.sample_rate = audio->sample_rate;
    if (audio->samples != nullptr && audio->n > 0) {
        result.samples.assign(audio->samples, audio->samples + audio->n);
    }

    SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);
    return result;
}

void KokoroTTS::LoadModel(const std::string& model_path, const std::string& device) {
    SherpaOnnxOfflineTtsConfig config;
    std::memset(&config, 0, sizeof(config));

    const std::string model = fmt::format("{}/model.onnx", model_path);
    const std::string voices = fmt::format("{}/voices.bin", model_path);
    const std::string tokens = fmt::format("{}/tokens.txt", model_path);
    const std::string lexicon = fmt::format("{}/lexicon-us-en.txt,{}/lexicon-zh.txt", model_path, model_path);

    config.model.kokoro.model = model.c_str();
    config.model.kokoro.voices = voices.c_str();
    config.model.kokoro.tokens = tokens.c_str();
    config.model.kokoro.lexicon = lexicon.c_str();
    config.model.kokoro.length_scale = 1.0f;

    config.model.num_threads = 2;
    config.model.debug = 0;
    config.model.provider = device.c_str();

    config.max_num_sentences = 1;
    config.silence_scale = 0.2f;

    tts_ = SherpaOnnxCreateOfflineTts(&config);
    if (tts_ == nullptr) {
        throw std::runtime_error("Failed to create Kokoro TTS from " + model_path);
    }
}
