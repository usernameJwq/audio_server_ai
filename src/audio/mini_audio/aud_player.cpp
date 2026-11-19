#define MINIAUDIO_IMPLEMENTATION

#include "aud_player.h"

#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>

#include "fmt/format.h"
#include "logs.h"

void MiniAudioPlayer::check_result(ma_result result, const char* action) {
    if (result != MA_SUCCESS) {
        const std::string message = fmt::format("Failed to {}: {}", action, ma_result_description(result));
        throw std::runtime_error(message);
    }
}

MiniAudioPlayer::MiniAudioPlayer() : engine_initialized_(false), engine_() {
    try {
        check_result(ma_engine_init(nullptr, &engine_), "initialize miniaudio engine");
        engine_initialized_ = true;
    } catch (const std::exception& e) {
        LOG::error("Failed to initialize MiniAudioPlayer: {}", e.what());
        throw;
    }
}

MiniAudioPlayer::~MiniAudioPlayer() {
    try {
        if (engine_initialized_) {
            ma_engine_uninit(&engine_);
            engine_initialized_ = false;
        }
    } catch (const std::exception& e) {
        LOG::error("Failed to uninitialize MiniAudioPlayer: {}", e.what());
    }
}

void MiniAudioPlayer::play(const std::vector<float>& samples, int sample_rate) {
    if (samples.empty()) {
        LOG::warn("Skip playing empty audio");
        return;
    }

    if (sample_rate <= 0) {
        throw std::invalid_argument("sample_rate must be positive");
    }

    if (!engine_initialized_) {
        throw std::runtime_error("MiniAudioPlayer is not initialized");
    }

    std::lock_guard<std::mutex> lock(mutex_);

    ma_audio_buffer_config config =
        ma_audio_buffer_config_init(ma_format_f32, 1, static_cast<ma_uint64>(samples.size()), samples.data(), nullptr);

    // miniaudio 0.11 keeps sampleRate as 0 in ma_audio_buffer_config_init; set it explicitly so that
    // ma_sound can resample the TTS output to the engine's sample rate.
    config.sampleRate = static_cast<ma_uint32>(sample_rate);

    PlayBackResources resources;

    try {
        check_result(ma_audio_buffer_init(&config, &resources.buffer), "create audio buffer from PCM samples");
    } catch (const std::exception& e) {
        LOG::error("Failed to init audio buffer MiniAudioPlayer: {}", e.what());
        return;
    }

    resources.buffer_initialized = true;

    try {
        check_result(ma_sound_init_from_data_source(&engine_, &resources.buffer, 0, nullptr, &resources.sound),
                     "create sound from audio buffer");
    } catch (const std::exception& e) {
        LOG::error("Failed to init sound from audio buffer MiniAudioPlayer: {}", e.what());
        return;
    }

    resources.sound_initialized = true;

    try {
        check_result(ma_sound_start(&resources.sound), "start audio playback");
    } catch (const std::exception& e) {
        LOG::error("Failed to start audio playback MiniAudioPlayer: {}", e.what());
        return;
    }

    LOG::debug("Playing {} samples at {} Hz", samples.size(), sample_rate);

    while (ma_sound_is_playing(&resources.sound) && !ma_sound_at_end(&resources.sound)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
