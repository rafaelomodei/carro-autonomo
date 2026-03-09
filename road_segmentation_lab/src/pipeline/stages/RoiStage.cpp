#include "pipeline/stages/RoiStage.hpp"

#include <algorithm>
#include <cmath>

namespace road_segmentation_lab::pipeline::stages {

RoiStage::RoiStage(double top_ratio, double bottom_ratio)
    : top_ratio_(std::clamp(top_ratio, 0.0, 1.0)),
      bottom_ratio_(std::clamp(bottom_ratio, 0.0, 1.0)) {
    if (bottom_ratio_ <= top_ratio_) {
        bottom_ratio_ = std::min(1.0, top_ratio_ + 0.05);
    }
}

cv::Rect RoiStage::computeRect(const cv::Size &frame_size) const {
    if (frame_size.width <= 0 || frame_size.height <= 0) {
        return {};
    }

    const int top = std::clamp(static_cast<int>(std::lround(frame_size.height * top_ratio_)), 0,
                               frame_size.height - 1);
    int bottom = std::clamp(static_cast<int>(std::lround(frame_size.height * bottom_ratio_)), top + 1,
                            frame_size.height);
    if (bottom <= top) {
        bottom = frame_size.height;
    }

    return cv::Rect(0, top, frame_size.width, bottom - top);
}

cv::Mat RoiStage::apply(const cv::Mat &frame, cv::Rect *rect) const {
    if (frame.empty()) {
        if (rect) {
            *rect = {};
        }
        return cv::Mat();
    }

    const cv::Rect roi = computeRect(frame.size());
    if (rect) {
        *rect = roi;
    }
    if (roi.width <= 0 || roi.height <= 0) {
        return cv::Mat();
    }
    return frame(roi).clone();
}

} // namespace road_segmentation_lab::pipeline::stages
