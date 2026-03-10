#pragma once

#include <opencv2/core.hpp>

#include <string>

namespace road_segmentation_lab::pipeline::stages {

class UndistortStage {
  public:
    explicit UndistortStage(const std::string &calibration_file = {});

    cv::Mat apply(const cv::Mat &frame) const;
    bool isActive() const noexcept;
    const std::string &statusMessage() const noexcept;

  private:
    bool active_{false};
    std::string status_message_;
    cv::Mat camera_matrix_;
    cv::Mat dist_coeffs_;
};

} // namespace road_segmentation_lab::pipeline::stages
