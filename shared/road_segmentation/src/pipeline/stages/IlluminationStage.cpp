#include "pipeline/stages/IlluminationStage.hpp"

#include <vector>

#include <opencv2/imgproc.hpp>

namespace road_segmentation_lab::pipeline::stages {

IlluminationStage::IlluminationStage(bool clahe_enabled) : clahe_enabled_(clahe_enabled) {}

cv::Mat IlluminationStage::apply(const cv::Mat &frame) const {
    if (frame.empty()) {
        return cv::Mat();
    }

    if (!clahe_enabled_) {
        return frame.clone();
    }

    cv::Mat lab;
    cv::cvtColor(frame, lab, cv::COLOR_BGR2Lab);

    std::vector<cv::Mat> channels;
    cv::split(lab, channels);
    auto clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
    clahe->apply(channels[0], channels[0]);
    cv::merge(channels, lab);

    cv::Mat result;
    cv::cvtColor(lab, result, cv::COLOR_Lab2BGR);
    return result;
}

} // namespace road_segmentation_lab::pipeline::stages
