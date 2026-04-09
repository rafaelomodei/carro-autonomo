#include "vision/FrameDecoder.hpp"

#include <vector>

#include <opencv2/imgcodecs.hpp>

#include "vision/Base64.hpp"

namespace {

void setError(std::string *error, const std::string &message) {
    if (error) {
        *error = message;
    }
}

} // namespace

namespace traffic_sign_service::vision {

cv::Mat decodeVisionFrameImage(const VisionFrameMessage &frame, std::string *error) {
    auto bytes = decodeBase64(frame.data, error);
    if (bytes.empty()) {
        if (!frame.data.empty()) {
            return {};
        }
        setError(error, "Frame sem bytes decodificados.");
        return {};
    }

    cv::Mat encoded(1, static_cast<int>(bytes.size()), CV_8UC1, bytes.data());
    cv::Mat decoded = cv::imdecode(encoded, cv::IMREAD_COLOR);
    if (decoded.empty()) {
        setError(error, "Falha ao decodificar JPEG do frame.");
        return {};
    }

    return decoded;
}

} // namespace traffic_sign_service::vision
