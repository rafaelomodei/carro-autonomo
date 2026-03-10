#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <opencv2/core.hpp>

namespace autonomous_car::services::vision {

enum class VisionDebugViewId {
    Raw,
    Preprocess,
    Mask,
    Annotated,
    Dashboard,
};

struct VisionDebugViewIdHash {
    std::size_t operator()(VisionDebugViewId view) const noexcept;
};

using VisionDebugViewSet = std::unordered_set<VisionDebugViewId, VisionDebugViewIdHash>;

const std::vector<VisionDebugViewId> &allVisionDebugViews();
std::string_view toString(VisionDebugViewId view);
std::optional<VisionDebugViewId> visionDebugViewFromString(std::string_view value);
VisionDebugViewSet parseVisionDebugSubscriptionCsv(
    std::string_view csv, std::vector<std::string> *invalid_tokens = nullptr);
std::optional<std::string> buildVisionFrameJson(
    VisionDebugViewId view, const cv::Mat &frame, std::int64_t timestamp_ms, int jpeg_quality);

} // namespace autonomous_car::services::vision
