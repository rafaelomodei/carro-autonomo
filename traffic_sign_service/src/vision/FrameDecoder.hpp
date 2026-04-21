#pragma once

#include <string>

#include <opencv2/core/mat.hpp>

#include "vision/VisionFrame.hpp"

namespace traffic_sign_service::vision {

cv::Mat decodeVisionFrameImage(const VisionFrameMessage &frame, std::string *error = nullptr);

} // namespace traffic_sign_service::vision
