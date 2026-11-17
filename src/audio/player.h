#ifndef AUDIO_PLAYER_H_
#define AUDIO_PLAYER_H_

#include <vector>

class AudioPlayer {
public:
    virtual void play(const std::vector<float>& samples, int sample_rate) = 0;
    virtual ~AudioPlayer() = default;
};

#endif // AUDIO_PLAYER_H_