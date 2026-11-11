#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "kokoro_tts.h"

class KokoroTtsTest : public ::testing::Test {
protected:
    void SetUp() override {
        if (!tts_) {
            tts_ = std::unique_ptr<KokoroTTS>(new KokoroTTS(this->KokoroModelDir(), "cpu"));
        }
    }

    std::string KokoroModelDir() {
        const std::string source = __FILE__;
        const std::string::size_type pos = source.find_last_of("/\\");
        const std::string tests_dir = pos == std::string::npos ? "." : source.substr(0, pos);
        return tests_dir + "/../../../models/kokoro-multi-lang-v1_1";
    }

    std::unique_ptr<KokoroTTS> tts_ = nullptr;
};

TEST_F(KokoroTtsTest, LoadsModel) { ASSERT_NE(tts_, nullptr); }

TEST_F(KokoroTtsTest, GeneratesEnglishAudio) {
    ASSERT_NE(tts_, nullptr);

    const auto audio = tts_->Generate("Hello, this is a test of Kokoro text to speech.");

    EXPECT_FALSE(audio.samples.empty());
    EXPECT_GT(audio.sample_rate, 0);
}

TEST_F(KokoroTtsTest, GeneratesChineseAudio) {
    ASSERT_NE(tts_, nullptr);

    const auto audio = tts_->Generate("你好，这是 Kokoro 语音合成测试。");

    EXPECT_FALSE(audio.samples.empty());
    EXPECT_GT(audio.sample_rate, 0);
}
