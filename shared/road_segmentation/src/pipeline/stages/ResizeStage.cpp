#include "pipeline/stages/ResizeStage.hpp"

#include <opencv2/imgproc.hpp>

namespace road_segmentation_lab::pipeline::stages {

ResizeStage::ResizeStage(bool enabled, cv::Size target_size)
    : enabled_(enabled), target_size_(target_size) {}

cv::Mat ResizeStage::apply(const cv::Mat &frame) const {
    if (frame.empty()) {
        return cv::Mat();
    }

    if (!enabled_ || target_size_.width <= 0 || target_size_.height <= 0 ||
        frame.size() == target_size_) {
        return frame.clone();
    }

    cv::Mat resized;
    cv::resize(frame, resized, target_size_, 0.0, 0.0, cv::INTER_AREA);
    return resized;
}

} // namespace road_segmentation_lab::pipeline::stages
