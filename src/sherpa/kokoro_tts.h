#include "tts.h"

class KokoroTTS : public SherpaTTS {
public:
    KokoroTTS(const std::string& model_path, const std::string& device = "cpu");

    virtual GeneratedAudio Generate(const std::string& text) override;

private:
    void LoadModel(const std::string& model_path, const std::string& device = "cpu") override;

private:
    sherpa_onnx::cxx::OfflineTts tts_;
};