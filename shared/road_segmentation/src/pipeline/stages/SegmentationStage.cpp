#include "pipeline/stages/SegmentationStage.hpp"

#include <vector>

#include <opencv2/imgproc.hpp>

namespace road_segmentation_lab::pipeline::stages {

SegmentationStage::SegmentationStage(const config::LabConfig &config)
    : mode_(config.segmentation_mode),
      hsv_low_(config.hsv_low),
      hsv_high_(config.hsv_high),
      gray_threshold_(config.gray_threshold),
      adaptive_block_size_(config.adaptive_block_size),
      adaptive_c_(config.adaptive_c) {}

cv::Mat SegmentationStage::apply(const cv::Mat &frame) const {
    if (frame.empty()) {
        return cv::Mat();
    }

    cv::Mat mask;
    switch (mode_) {
    case config::SegmentationMode::HsvDark: {
        cv::Mat hsv;
        cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
        cv::inRange(hsv, hsv_low_, hsv_high_, mask);
        break;
    }
    case config::SegmentationMode::GrayThreshold: {
        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        cv::threshold(gray, mask, gray_threshold_, 255, cv::THRESH_BINARY_INV);
        break;
    }
    case config::SegmentationMode::LabDark: {
        cv::Mat lab;
        cv::cvtColor(frame, lab, cv::COLOR_BGR2Lab);
        std::vector<cv::Mat> channels;
        cv::split(lab, channels);
        cv::threshold(channels[0], mask, gray_threshold_, 255, cv::THRESH_BINARY_INV);
        break;
    }
    case config::SegmentationMode::AdaptiveGray: {
        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        cv::adaptiveThreshold(gray, mask, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                              cv::THRESH_BINARY_INV, adaptive_block_size_, adaptive_c_);
        break;
    }
    }

    return mask;
}

std::string SegmentationStage::modeName() const { return config::segmentationModeToString(mode_); }

} // namespace road_segmentation_lab::pipeline::stages
