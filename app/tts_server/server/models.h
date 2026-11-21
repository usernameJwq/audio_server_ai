#ifndef TTS_SERVER_MODELS_H_
#define TTS_SERVER_MODELS_H_

#include <string>

// 定义在线播放的请求、解析的结构体
struct TTSPlayRequest {
    std::string request_id;
    std::string text;
    std::string language;
    int voice;
    int speed;
};

// 定义获取音频文件的请求、响应结构体
struct TTSGetAudioRequest {
    std::string request_id;
    std::string text;
    std::string language;
    int voice;
    int speed;
};

struct TTSResponse {
    int code;
    std::string status;
    std::string message;
};

#endif // TTS_SERVER_MODELS_H_