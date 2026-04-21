#include "domain/TrafficSignTypes.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace {

std::string normalize(std::string_view value) {
    std::string normalized;
    normalized.reserve(value.size());

    for (char ch : value) {
        if (std::isalnum(static_cast<unsigned char>(ch)) != 0) {
            normalized.push_back(
                static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        }
    }

    return normalized;
}

} // namespace

namespace traffic_sign_service {

std::string_view toString(FrameSourceMode mode) {
    switch (mode) {
    case FrameSourceMode::Image:
        return "image";
    case FrameSourceMode::Video:
        return "video";
    case FrameSourceMode::Camera:
        return "camera";
    case FrameSourceMode::WebSocket:
        return "websocket";
    }

    return "websocket";
}

std::optional<FrameSourceMode> frameSourceModeFromString(std::string_view value) {
    const std::string normalized = normalize(value);
    if (normalized == "image") {
        return FrameSourceMode::Image;
    }
    if (normalized == "video") {
        return FrameSourceMode::Video;
    }
    if (normalized == "camera") {
        return FrameSourceMode::Camera;
    }
    if (normalized == "websocket") {
        return FrameSourceMode::WebSocket;
    }
    return std::nullopt;
}

std::string_view toString(EdgeImpulseBackend backend) {
    switch (backend) {
    case EdgeImpulseBackend::Eim:
        return "eim";
    case EdgeImpulseBackend::EmbeddedCpp:
        return "embedded_cpp";
    }

    return "eim";
}

std::optional<EdgeImpulseBackend> edgeImpulseBackendFromString(std::string_view value) {
    const std::string normalized = normalize(value);
    if (normalized == "eim") {
        return EdgeImpulseBackend::Eim;
    }
    if (normalized == "embeddedcpp") {
        return EdgeImpulseBackend::EmbeddedCpp;
    }
    return std::nullopt;
}

std::string_view toString(TrafficSignDetectorState state) {
    switch (state) {
    case TrafficSignDetectorState::Disabled:
        return "disabled";
    case TrafficSignDetectorState::Idle:
        return "idle";
    case TrafficSignDetectorState::Candidate:
        return "candidate";
    case TrafficSignDetectorState::Confirmed:
        return "confirmed";
    case TrafficSignDetectorState::Error:
        return "error";
    }

    return "error";
}

double trafficSignRoiRightWidthRatio(const TrafficSignRoi &roi) {
    return std::clamp(roi.right_ratio - roi.left_ratio, 0.0, 1.0);
}

TrafficSignRoi buildTrafficSignRoi(cv::Size frame_size, double left_ratio, double right_ratio,
                                   double top_ratio, double bottom_ratio,
                                   bool debug_roi_enabled) {
    TrafficSignRoi roi;
    roi.left_ratio = std::clamp(left_ratio, 0.0, 0.95);
    roi.right_ratio = std::clamp(right_ratio, roi.left_ratio + 0.01, 1.0);
    roi.top_ratio = std::clamp(top_ratio, 0.0, 0.95);
    roi.bottom_ratio = std::clamp(bottom_ratio, roi.top_ratio + 0.01, 1.0);
    roi.debug_roi_enabled = debug_roi_enabled;
    roi.source_frame_size = frame_size;

    if (frame_size.width <= 0 || frame_size.height <= 0) {
        return roi;
    }

    const int left_x = std::clamp(
        static_cast<int>(std::lround(static_cast<double>(frame_size.width) * roi.left_ratio)),
        0, frame_size.width - 1);
    const int right_x = std::clamp(
        static_cast<int>(std::lround(static_cast<double>(frame_size.width) * roi.right_ratio)),
        left_x + 1, frame_size.width);
    const int top_y = std::clamp(
        static_cast<int>(std::lround(static_cast<double>(frame_size.height) * roi.top_ratio)),
        0, frame_size.height - 1);
    const int bottom_y = std::clamp(
        static_cast<int>(std::lround(static_cast<double>(frame_size.height) *
                                     roi.bottom_ratio)),
        top_y + 1, frame_size.height);

    roi.frame_rect = cv::Rect(left_x, top_y, std::max(1, right_x - left_x), bottom_y - top_y);
    return roi;
}

TrafficSignBoundingBox scaleBoundingBox(const TrafficSignBoundingBox &bbox, cv::Size from_size,
                                        cv::Size to_size) {
    if (from_size.width <= 0 || from_size.height <= 0 || to_size.width <= 0 ||
        to_size.height <= 0) {
        return {};
    }

    const double scale_x =
        static_cast<double>(to_size.width) / static_cast<double>(from_size.width);
    const double scale_y =
        static_cast<double>(to_size.height) / static_cast<double>(from_size.height);

    return TrafficSignBoundingBox{
        static_cast<int>(std::lround(static_cast<double>(bbox.x) * scale_x)),
        static_cast<int>(std::lround(static_cast<double>(bbox.y) * scale_y)),
        std::max(1, static_cast<int>(std::lround(static_cast<double>(bbox.width) * scale_x))),
        std::max(1, static_cast<int>(std::lround(static_cast<double>(bbox.height) * scale_y))),
    };
}

TrafficSignBoundingBox translateBoundingBox(const TrafficSignBoundingBox &bbox,
                                            const cv::Point &offset) {
    return TrafficSignBoundingBox{
        bbox.x + offset.x,
        bbox.y + offset.y,
        bbox.width,
        bbox.height,
    };
}

TrafficSignBoundingBox clampBoundingBox(const TrafficSignBoundingBox &bbox,
                                        const cv::Rect &bounds) {
    if (bounds.width <= 0 || bounds.height <= 0) {
        return {};
    }

    const int left = std::clamp(bbox.x, bounds.x, bounds.x + bounds.width - 1);
    const int top = std::clamp(bbox.y, bounds.y, bounds.y + bounds.height - 1);
    const int right = std::clamp(bbox.x + std::max(0, bbox.width), bounds.x + 1,
                                 bounds.x + bounds.width);
    const int bottom = std::clamp(bbox.y + std::max(0, bbox.height), bounds.y + 1,
                                  bounds.y + bounds.height);

    return TrafficSignBoundingBox{
        left,
        top,
        std::max(1, right - left),
        std::max(1, bottom - top),
    };
}

cv::Rect toCvRect(const TrafficSignBoundingBox &bbox) {
    return cv::Rect(bbox.x, bbox.y, std::max(0, bbox.width), std::max(0, bbox.height));
}

TrafficSignFrameResult makeTrafficSignFrameResult(TrafficSignDetectorState state,
                                                  const TrafficSignRoi &roi,
                                                  std::int64_t timestamp_ms,
                                                  std::string last_error) {
    TrafficSignFrameResult result;
    result.timestamp_ms = timestamp_ms;
    result.detector_state = state;
    result.roi = roi;
    result.last_error = std::move(last_error);
    return result;
}

std::int64_t trafficSignResultAgeMs(const TrafficSignFrameResult &result,
                                    std::int64_t reference_timestamp_ms) {
    if (result.timestamp_ms <= 0 || reference_timestamp_ms <= 0) {
        return -1;
    }

    return std::max<std::int64_t>(0, reference_timestamp_ms - result.timestamp_ms);
}

} // namespace traffic_sign_service
