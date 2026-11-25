#include "file_utils.h"

#include <chrono>
#include <filesystem>
#include <stdexcept>

std::string FileUtils::sanitize_file_stem(const std::string& id) {
    std::string stem;

    for (unsigned char ch : id) {
        if (std::isalnum(ch) || ch == '-' || ch == '_') {
            stem.push_back(static_cast<char>(ch));
        }

        if (stem.size() >= 64) {
            break;
        }
    }

    return stem;
}

std::string FileUtils::make_audio_path(const std::string& aud_dir, const std::string& request_id) {
    std::error_code ec;
    std::filesystem::create_directories(aud_dir, ec);
    if (ec) {
        throw std::runtime_error("create audio dir failed: " + ec.message());
    }

    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

    std::string file_name = std::to_string(ms) + ".wav";
    const std::string stem = sanitize_file_stem(request_id);
    if (!stem.empty()) {
        file_name = stem + "_" + file_name;
    }

    return (std::filesystem::path(aud_dir) / file_name).string();
}