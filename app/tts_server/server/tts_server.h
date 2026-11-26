#ifndef TTS_SERVER_HTTP_H_
#define TTS_SERVER_HTTP_H_

#include <memory>
#include <mutex>
#include <string>

#include "httplib.h"

#include "kokoro_tts.h"
#include "aud_player.h"

class Server {
public:
    Server(const std::string& model_path, const int& port);
    ~Server();

public:
    void run();

    bool play_audio(const std::string& text, int voice = 0, int speed = 1);

    std::string generator_audio(const std::string& text, int voice = 0, int speed = 1,
                                const std::string& request_id = "");

private:
    void register_routes();

private:
    int port_;
    std::string model_path_;

    httplib::Server server_;

    std::mutex tts_mutex_;

    std::unique_ptr<KokoroTTS> tts_;
    std::shared_ptr<MiniAudioPlayer> player_;

    static std::string KHost;
    static std::string kAudioDir;
};

#endif // TTS_SERVER_HTTP_H_
