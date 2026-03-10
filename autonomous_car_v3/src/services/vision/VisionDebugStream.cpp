#include "services/vision/VisionDebugStream.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <vector>

#include <opencv2/imgcodecs.hpp>

namespace autonomous_car::services::vision {
namespace {

std::string trim(std::string_view value) {
    std::size_t begin = 0;
    while (begin < value.size() &&
           std::isspace(static_cast<unsigned char>(value[begin])) != 0) {
        ++begin;
    }

    std::size_t end = value.size();
    while (end > begin &&
           std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }

    return std::string(value.substr(begin, end - begin));
}

std::string toLower(std::string_view value) {
    std::string normalized;
    normalized.reserve(value.size());

    for (char ch : value) {
        normalized.push_back(
            static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }

    return normalized;
}

std::string base64Encode(const std::vector<unsigned char> &input) {
    static constexpr char kTable[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string encoded;
    encoded.reserve(((input.size() + 2) / 3) * 4);

    std::size_t index = 0;
    while (index < input.size()) {
        const std::size_t remaining = input.size() - index;
        const std::uint32_t octet_a = input[index++];
        const std::uint32_t octet_b = remaining > 1 ? input[index++] : 0;
        const std::uint32_t octet_c = remaining > 2 ? input[index++] : 0;

        const std::uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;
        encoded.push_back(kTable[(triple >> 18) & 0x3F]);
        encoded.push_back(kTable[(triple >> 12) & 0x3F]);
        encoded.push_back(remaining > 1 ? kTable[(triple >> 6) & 0x3F] : '=');
        encoded.push_back(remaining > 2 ? kTable[triple & 0x3F] : '=');
    }

    return encoded;
}

} // namespace

std::size_t VisionDebugViewIdHash::operator()(VisionDebugViewId view) const noexcept {
    return static_cast<std::size_t>(view);
}

const std::vector<VisionDebugViewId> &allVisionDebugViews() {
    static const std::vector<VisionDebugViewId> kViews = {
        VisionDebugViewId::Raw,
        VisionDebugViewId::Preprocess,
        VisionDebugViewId::Mask,
        VisionDebugViewId::Annotated,
        VisionDebugViewId::Dashboard,
    };

    return kViews;
}

std::string_view toString(VisionDebugViewId view) {
    switch (view) {
    case VisionDebugViewId::Raw:
        return "raw";
    case VisionDebugViewId::Preprocess:
        return "preprocess";
    case VisionDebugViewId::Mask:
        return "mask";
    case VisionDebugViewId::Annotated:
        return "annotated";
    case VisionDebugViewId::Dashboard:
        return "dashboard";
    }

    return "raw";
}

std::optional<VisionDebugViewId> visionDebugViewFromString(std::string_view value) {
    const std::string normalized = toLower(trim(value));
    if (normalized == "raw") {
        return VisionDebugViewId::Raw;
    }
    if (normalized == "preprocess") {
        return VisionDebugViewId::Preprocess;
    }
    if (normalized == "mask") {
        return VisionDebugViewId::Mask;
    }
    if (normalized == "annotated") {
        return VisionDebugViewId::Annotated;
    }
    if (normalized == "dashboard") {
        return VisionDebugViewId::Dashboard;
    }

    return std::nullopt;
}

VisionDebugViewSet parseVisionDebugSubscriptionCsv(
    std::string_view csv, std::vector<std::string> *invalid_tokens) {
    VisionDebugViewSet subscriptions;

    std::size_t start = 0;
    while (start <= csv.size()) {
        const std::size_t delimiter = csv.find(',', start);
        const std::size_t end =
            delimiter == std::string_view::npos ? csv.size() : delimiter;
        const std::string token = trim(csv.substr(start, end - start));

        if (!token.empty()) {
            if (const auto parsed = visionDebugViewFromString(token)) {
                subscriptions.insert(*parsed);
            } else if (invalid_tokens) {
                invalid_tokens->push_back(token);
            }
        }

        if (delimiter == std::string_view::npos) {
            break;
        }

        start = delimiter + 1;
    }

    return subscriptions;
}

std::optional<std::string> buildVisionFrameJson(
    VisionDebugViewId view, const cv::Mat &frame, std::int64_t timestamp_ms, int jpeg_quality) {
    if (frame.empty()) {
        return std::nullopt;
    }

    std::vector<int> encode_params = {
        cv::IMWRITE_JPEG_QUALITY,
        std::clamp(jpeg_quality, 10, 100),
    };
    std::vector<unsigned char> encoded_frame;
    if (!cv::imencode(".jpg", frame, encoded_frame, encode_params)) {
        return std::nullopt;
    }

    const std::string base64 = base64Encode(encoded_frame);

    std::string payload;
    payload.reserve(base64.size() + 160);
    payload += "{\"type\":\"vision.frame\"";
    payload += ",\"view\":\"";
    payload += toString(view);
    payload += "\"";
    payload += ",\"timestamp_ms\":";
    payload += std::to_string(timestamp_ms);
    payload += ",\"mime\":\"image/jpeg\"";
    payload += ",\"width\":";
    payload += std::to_string(frame.cols);
    payload += ",\"height\":";
    payload += std::to_string(frame.rows);
    payload += ",\"data\":\"";
    payload += base64;
    payload += "\"}";

    return payload;
}

} // namespace autonomous_car::services::vision
