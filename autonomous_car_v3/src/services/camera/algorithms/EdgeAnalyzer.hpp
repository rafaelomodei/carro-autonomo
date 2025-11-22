#pragma once

#include "services/camera/algorithms/EdgeAnalysisResult.hpp"
#include "services/camera/algorithms/EdgeDetectionConfig.hpp"

namespace autonomous_car::services::camera::algorithms {

class EdgeAnalyzer {
  public:
    EdgeAnalyzer();
    explicit EdgeAnalyzer(EdgeDetectionConfig config);

    EdgeAnalysisResult analyze(const cv::Mat &frame) const;
    cv::Mat buildDebugView(const cv::Mat &frame, const EdgeAnalysisResult &analysis) const;

  private:
    cv::Mat buildHistogramView(const EdgeAnalysisResult &analysis, int width) const;
    cv::Mat colorizeEdges(const cv::Mat &frame, const cv::Mat &edges) const;

    EdgeDetectionConfig config_;
};

} // namespace autonomous_car::services::camera::algorithms
