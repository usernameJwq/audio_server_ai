#ifndef TTS_SERVER_MODELS_H_
#define TTS_SERVER_MODELS_H_

#include <string>

#include <nlohmann/json.hpp>

// 定义在线播放的请求、解析的结构体
struct TTSPlayRequest {
    std::string request_id;
    std::string text;
    std::string language;
    int voice = 0;
    int speed = 1;
};

// 定义获取音频文件的请求、响应结构体
struct TTSGetAudioRequest {
    std::string request_id;
    std::string text;
    std::string language;
    int voice = 0;
    int speed = 1;
};

struct TTSResponse {
    int code = 0;
    std::string status;
    std::string message;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TTSPlayRequest, request_id, text, language, voice, speed)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TTSGetAudioRequest, request_id, text, language, voice, speed)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(TTSResponse, code, status, message)

struct TTSJsonParseModel {
    void send_json(httplib::Response& res, int status, const TTSResponse& body) {
        nlohmann::json j = body;
        res.status = status;
        res.set_content(j.dump(), "application/json");
    }
};

#endif // TTS_SERVER_MODELS_H_