#include "kokoro.h"

#include "fmt/format.h"

#include "logs.h"

KokoroTTS::KokoroTTS(const std::string& model_path, const std::string& device = "cpu") : tts_(nullptr) {
    try {
        LoadModel(model_path, device);
    } catch (const std::exception& e) {
        LOG::error("Failed to load Kokoro TTS model: {}", e.what());
        throw;
    }
}

GeneratedAudio KokoroTTS::Generate(const std::string& text) {
    try {
        auto audio = tts_->Generate(text);
        return audio;
    } catch (const std::exception& e) {
        LOG::error("Failed to generate speech: {}", e.what());
        throw;
    }
}

void KokoroTTS::LoadModel(const std::string& model_path, const std::string& device = "cpu") override {
    OfflineTtsConfig config;
    config.model.kokoro = fmt::format("{}/model.onnx", model_path);
    config.model.kokoro.voices = fmt::format("{}/voices.bin", model_path);
    config.model.kokoro.tokens = fmt::format("{}/tokens.txt", model_path);
    config.model.kokoro.lexicon = fmt::format("{}/lexicon-us-en.txt,{}/lexicon-zh.txt", model_path, model_path);
    config.model.num_threads = 2;
    config.model.debug = 1;

    tts_ = sherpa_onnx::cxx::OfflineTts::Create(config);
}