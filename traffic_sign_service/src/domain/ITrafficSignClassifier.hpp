#pragma once

#include <vector>

#include <opencv2/core/mat.hpp>

#include "domain/SignalDetection.hpp"

namespace traffic_sign_service {

class ITrafficSignClassifier {
public:
    virtual ~ITrafficSignClassifier() = default;

    virtual std::vector<SignalDetection> detect(const cv::Mat &frame) = 0;
};

} // namespace traffic_sign_service
