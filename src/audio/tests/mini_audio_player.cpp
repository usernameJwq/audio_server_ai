#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "aud_player.h"
#include "kokoro_tts.h"

class MiniAudioPlayerTest : public ::testing::Test {
protected:
    void SetUp() override { tts_ = std::unique_ptr<KokoroTTS>(new KokoroTTS(this->KokoroModelDir(), "cpu")); }

    std::string KokoroModelDir() {
        const std::string source = __FILE__;
        const std::string::size_type pos = source.find_last_of("/\\");
        const std::string tests_dir = pos == std::string::npos ? "." : source.substr(0, pos);
        return tests_dir + "/../../../models/kokoro-multi-lang-v1_1";
    }

    std::unique_ptr<KokoroTTS> tts_ = nullptr;
};

TEST_F(MiniAudioPlayerTest, play_audio_english) {
    const auto audio = tts_->Generate("Hello, this is a test of Kokoro text to speech.");

    EXPECT_FALSE(audio.samples.empty());
    EXPECT_GT(audio.sample_rate, 0);

    MiniAudioPlayer player;
    player.play(audio.samples, audio.sample_rate);
}

TEST_F(MiniAudioPlayerTest, play_audio_zh) {
    const auto audio = tts_->Generate("真好，今天是星期天。");

    EXPECT_FALSE(audio.samples.empty());
    EXPECT_GT(audio.sample_rate, 0);

    MiniAudioPlayer player;
    player.play(audio.samples, audio.sample_rate);
}