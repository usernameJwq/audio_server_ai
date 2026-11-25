#ifndef FILE_UTILS_H_
#define FILE_UTILS_H_

#include <string>

class FileUtils {
public:
    // 仅保留字母、数字、'-'、'_'，避免 request_id 被拼进文件名时造成路径穿越
    static std::string sanitize_file_stem(const std::string& id);

    static std::string make_audio_path(const std::string& aud_dir, const std::string& request_id);
};

#endif // FILE_UTILS_H_