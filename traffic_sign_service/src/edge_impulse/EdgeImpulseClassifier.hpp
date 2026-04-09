#pragma once

#include "domain/ITrafficSignClassifier.hpp"

namespace traffic_sign_service::edge_impulse {

class EdgeImpulseClassifier : public ITrafficSignClassifier {
public:
    std::vector<SignalDetection> detect(const cv::Mat &frame) override;
};

} // namespace traffic_sign_service::edge_impulse
