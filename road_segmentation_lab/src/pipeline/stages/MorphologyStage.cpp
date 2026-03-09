#include "pipeline/stages/MorphologyStage.hpp"

#include <opencv2/imgproc.hpp>

namespace road_segmentation_lab::pipeline::stages {

MorphologyStage::MorphologyStage(bool enabled, cv::Size kernel_size, int iterations)
    : enabled_(enabled), kernel_size_(kernel_size), iterations_(iterations) {}

cv::Mat MorphologyStage::apply(const cv::Mat &frame) const {
    if (frame.empty()) {
        return cv::Mat();
    }

    if (!enabled_) {
        return frame.clone();
    }

    const cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, kernel_size_);
    cv::Mat closed;
    cv::morphologyEx(frame, closed, cv::MORPH_CLOSE, kernel, {-1, -1}, iterations_);

    cv::Mat result;
    cv::morphologyEx(closed, result, cv::MORPH_OPEN, kernel, {-1, -1}, 1);
    return result;
}

} // namespace road_segmentation_lab::pipeline::stages
