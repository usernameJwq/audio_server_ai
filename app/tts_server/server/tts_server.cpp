#include "tts_server.h"

#include <string>
#include <thread>
#include <utility>

#include <nlohmann/json.hpp>

#include "fmt/format.h"

#include "models.h"
#include "logs.h"
#include "file_utils.h"

std::string Server::KHost = "0.0.0.0";
std::string Server::kAudioDir = "./tts_audio";

Server::Server(const std::string& model_path, const int& port)
    : port_(port), model_path_(model_path), tts_(nullptr), player_(nullptr) {
    // 初始化TTS
    try {
        tts_ = std::make_unique<KokoroTTS>(model_path);
    } catch (const std::exception& e) {
        const std::string msg = fmt::format("KokoroTTS init failed, {}", e.what());
        LOG::error(msg);
        throw std::runtime_error(msg);
    }

    // 初始化Player
    try {
        player_ = std::make_shared<MiniAudioPlayer>();
    } catch (const std::exception& e) {
        const std::string msg = fmt::format("MiniAudioPlayer init failed, {}", e.what());
        LOG::error(msg);
        throw std::runtime_error(msg);
    }
}

Server::~Server() {}

void Server::run() {
    register_routes();

    std::string msg = fmt::format("tts server start, port is {}", port_);
    LOG::info(msg);
    std::cout << msg << std::endl;

    const bool ret = server_.listen(KHost, port_);
    if (!ret) {
        LOG::error("serevr listen port {} failed", std::to_string(port_));
    }
}

void Server::register_routes() {
    LOG::info("register tts request router");

    std::string aud_play_info = fmt::format("音频播放: http://localhost:8080/tts/play");

    std::cout << aud_play_info << std::endl;

    // 在线播放：同步合成 PCM，播放放到后台线程，接口立即返回
    server_.Post("/tts/play", [this](const httplib::Request& req, httplib::Response& res) {
        TTSJsonParseModel obj;
        if (player_->is_playing()) {
            obj.send_json(res, 500, TTSResponse{500, "busying", "audio is playing"});
            return;
        }

        TTSPlayRequest request;

        try {
            request = nlohmann::json::parse(req.body).get<TTSPlayRequest>();
        } catch (const std::exception& e) {
            LOG::error("parse play request failed, {}", e.what());
            obj.send_json(res, 400, TTSResponse{400, "error", std::string("invalid request: ") + e.what()});
            return;
        }

        if (request.text.empty()) {
            obj.send_json(res, 400, TTSResponse{400, "error", "text is empty"});
            return;
        }

        LOG::info("tts play request, request_id={}, language={}, voice={}, speed={}", request.request_id,
                  request.language, request.voice, request.speed);

        if (play_audio(request.text, request.voice, request.speed)) {
            obj.send_json(res, 200, TTSResponse{0, "success", "playing"});
        } else {
            obj.send_json(res, 500, TTSResponse{500, "error", "generate or play audio failed"});
        }
    });

    // 生成音频文件：返回 wav 文件的绝对路径（放在 message 字段）
    server_.Post("/tts/audio", [this](const httplib::Request& req, httplib::Response& res) {
        TTSGetAudioRequest request;
        TTSJsonParseModel obj;

        try {
            request = nlohmann::json::parse(req.body).get<TTSGetAudioRequest>();
        } catch (const std::exception& e) {
            LOG::error("parse audio request failed, {}", e.what());
            obj.send_json(res, 400, TTSResponse{400, "error", std::string("invalid request: ") + e.what()});
            return;
        }

        if (request.text.empty()) {
            obj.send_json(res, 400, TTSResponse{400, "error", "text is empty"});
            return;
        }

        LOG::info("tts audio request, request_id={}, language={}, voice={}, speed={}", request.request_id,
                  request.language, request.voice, request.speed);

        try {
            const std::string path = generator_audio(request.text, request.voice, request.speed, request.request_id);
            obj.send_json(res, 200, TTSResponse{0, "success", path});
        } catch (const std::exception& e) {
            LOG::error("generate audio failed, {}", e.what());
            obj.send_json(res, 500, TTSResponse{500, "error", e.what()});
        }
    });
}

bool Server::play_audio(const std::string& text, int voice, int speed) {
    try {
        TtsAudio pcm;
        {
            std::lock_guard<std::mutex> lock(tts_mutex_);
            pcm = tts_->Generate(text);
        }

        if (pcm.samples.empty()) {
            LOG::error("generate pcm failed, samples is empty");
            return false;
        }

        auto player = player_;
        std::thread([player, pcm = std::move(pcm)] {
            try {
                player->play(pcm.samples, pcm.sample_rate);
            } catch (const std::exception& e) {
                LOG::error("play audio failed, {}", e.what());
            }
        }).detach();

        return true;
    } catch (const std::exception& e) {
        LOG::error("play audio failed, {}", e.what());
        return false;
    }
}

std::string Server::generator_audio(const std::string& text, int voice, int speed, const std::string& request_id) {
    TtsAudio pcm;
    {
        std::lock_guard<std::mutex> lock(tts_mutex_);
        pcm = tts_->Generate(text);
    }

    if (pcm.samples.empty()) {
        throw std::runtime_error("generated pcm is empty");
    }

    const std::string path = FileUtils::make_audio_path(kAudioDir, request_id);
    if (SherpaOnnxWriteWave(pcm.samples.data(), static_cast<int32_t>(pcm.samples.size()), pcm.sample_rate,
                            path.c_str()) == 0) {
        throw std::runtime_error("write wave file failed: " + path);
    }

    std::error_code ec;
    std::filesystem::path abs_path = std::filesystem::absolute(path, ec);
    if (ec) {
        LOG::error("get absolute audio path failed, {}, return relative path", ec.message());
        return path;
    }

    return abs_path.lexically_normal().string();
}
