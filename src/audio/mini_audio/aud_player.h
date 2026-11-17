#ifndef AUDIO_MINIAUDIO_PLAYER_H_
#define AUDIO_MINIAUDIO_PLAYER_H_

#include "player.h"

class MiniAudioPlayer : public AudioPlayer {
public:
    void play(const std::vector<float>& samples, int sample_rate) override;
};

#endif // AUDIO_MINIAUDIO_PLAYER_H_