#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

#include "domain/TrafficSignTypes.hpp"

namespace traffic_sign_service {

class ITrafficSignClassifier {
public:
    virtual ~ITrafficSignClassifier() = default;

    virtual TrafficSignFrameResult detect(const TrafficSignInferenceInput &input,
                                          std::int64_t timestamp_ms) = 0;
    virtual std::vector<std::string> modelLabels() const = 0;
    virtual cv::Size inputSize() const = 0;
    virtual std::string backendName() const = 0;
};

} // namespace traffic_sign_service
