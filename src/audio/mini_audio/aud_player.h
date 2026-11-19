#ifndef AUDIO_MINIAUDIO_PLAYER_H_
#define AUDIO_MINIAUDIO_PLAYER_H_

#include <mutex>
#include <vector>

#include "player.h"

#include "miniaudio.h"

#define MINIAUDIO_IMPLEMENTATION

struct PlayBackResources {
    ma_audio_buffer buffer{};
    ma_sound sound{};

    bool buffer_initialized;
    bool sound_initialized;

    PlayBackResources() : buffer_initialized(false), sound_initialized(false) {}

    ~PlayBackResources() {
        if (sound_initialized) {
            ma_sound_uninit(&sound);
        }

        if (buffer_initialized) {
            ma_audio_buffer_uninit(&buffer);
        }
    }
};

class MiniAudioPlayer : public AudioPlayer {
public:
    MiniAudioPlayer();
    ~MiniAudioPlayer() override;

    MiniAudioPlayer(const MiniAudioPlayer&) = delete;
    MiniAudioPlayer& operator=(const MiniAudioPlayer&) = delete;

    void play(const std::vector<float>& samples, int sample_rate) override;

private:
    static void check_result(ma_result result, const char* action);

private:
    bool engine_initialized_;
    std::mutex mutex_;
    ma_engine engine_;
};

#endif // AUDIO_MINIAUDIO_PLAYER_H_