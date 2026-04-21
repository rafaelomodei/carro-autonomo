#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace traffic_sign_service::vision {

struct VisionFrameMessage {
    std::string view;
    std::uint64_t timestamp_ms{0};
    std::string mime;
    int width{0};
    int height{0};
    std::string data;
};

std::optional<VisionFrameMessage> parseVisionFrameMessage(const std::string &payload,
                                                          std::string *error = nullptr);

} // namespace traffic_sign_service::vision
