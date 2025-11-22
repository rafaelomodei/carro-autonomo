#pragma once

#include <vector>

#include <opencv2/core.hpp>

namespace autonomous_car::services::camera::algorithms {

struct EdgeAnalysisResult {
    cv::Mat edges;
    cv::Mat overlay;
    std::vector<int> edge_density_per_column;
    int max_density = 0;
};

} // namespace autonomous_car::services::camera::algorithms
