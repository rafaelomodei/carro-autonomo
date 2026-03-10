#include "pipeline/stages/GaussianStage.hpp"

#include <opencv2/imgproc.hpp>

namespace road_segmentation_lab::pipeline::stages {

GaussianStage::GaussianStage(bool enabled, cv::Size kernel_size, double sigma)
    : enabled_(enabled), kernel_size_(kernel_size), sigma_(sigma) {}

cv::Mat GaussianStage::apply(const cv::Mat &frame) const {
    if (frame.empty()) {
        return cv::Mat();
    }

    if (!enabled_) {
        return frame.clone();
    }

    cv::Mat blurred;
    cv::GaussianBlur(frame, blurred, kernel_size_, sigma_);
    return blurred;
}

} // namespace road_segmentation_lab::pipeline::stages
