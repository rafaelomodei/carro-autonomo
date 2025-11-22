#include "services/camera/algorithms/EdgeAnalyzer.hpp"

#include <algorithm>
#include <numeric>
#include <utility>

#include <opencv2/imgproc.hpp>

namespace autonomous_car::services::camera::algorithms {
namespace {
cv::Mat ToGray(const cv::Mat &frame) {
    if (frame.empty()) {
        return cv::Mat();
    }

    cv::Mat gray;
    if (frame.channels() == 1) {
        gray = frame;
    } else {
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    }
    return gray;
}

std::vector<int> BuildEdgeHistogram(const cv::Mat &edges) {
    if (edges.empty()) {
        return {};
    }

    cv::Mat reduced;
    cv::reduce(edges, reduced, 0, cv::REDUCE_SUM, CV_32S);

    std::vector<int> histogram(static_cast<size_t>(reduced.cols), 0);
    for (int x = 0; x < reduced.cols; ++x) {
        histogram[static_cast<size_t>(x)] = reduced.at<int>(0, x) / 255; // normaliza contagem
    }
    return histogram;
}
} // namespace

EdgeAnalyzer::EdgeAnalyzer() : EdgeAnalyzer(EdgeDetectionConfig::LoadFromEnv()) {}

EdgeAnalyzer::EdgeAnalyzer(EdgeDetectionConfig config) : config_(std::move(config)) {}

EdgeAnalysisResult EdgeAnalyzer::analyze(const cv::Mat &frame) const {
    EdgeAnalysisResult result;
    if (frame.empty()) {
        return result;
    }

    cv::Mat gray = ToGray(frame);
    if (gray.empty()) {
        return result;
    }

    if (config_.blur_kernel.width > 1 || config_.blur_kernel.height > 1) {
        cv::GaussianBlur(gray, gray, config_.blur_kernel, 0.0);
    }

    cv::Canny(gray, result.edges, config_.canny_threshold1, config_.canny_threshold2,
              config_.canny_aperture, config_.canny_l2gradient);

    result.edge_density_per_column = BuildEdgeHistogram(result.edges);
    result.max_density = result.edge_density_per_column.empty()
                             ? 0
                             : *std::max_element(result.edge_density_per_column.begin(),
                                                 result.edge_density_per_column.end());

    result.overlay = colorizeEdges(frame, result.edges);
    return result;
}

cv::Mat EdgeAnalyzer::buildDebugView(const cv::Mat &frame, const EdgeAnalysisResult &analysis) const {
    if (frame.empty() || analysis.edges.empty()) {
        return cv::Mat();
    }

    const cv::Mat histogram_view = buildHistogramView(analysis, frame.cols);

    cv::Mat debug(frame.rows + histogram_view.rows, frame.cols, CV_8UC3, cv::Scalar(20, 20, 20));
    cv::Mat top = debug(cv::Rect(0, 0, frame.cols, frame.rows));
    analysis.overlay.copyTo(top);

    cv::Mat bottom = debug(cv::Rect(0, frame.rows, histogram_view.cols, histogram_view.rows));
    histogram_view.copyTo(bottom);

    cv::putText(debug, "Deteccao de bordas (Canny)", {20, 40}, cv::FONT_HERSHEY_SIMPLEX, 0.9,
                {255, 255, 255}, 2, cv::LINE_AA);
    return debug;
}

cv::Mat EdgeAnalyzer::buildHistogramView(const EdgeAnalysisResult &analysis, int width) const {
    const int chart_height = 180;
    cv::Mat chart(chart_height, width, CV_8UC3, cv::Scalar(30, 30, 30));

    if (analysis.edge_density_per_column.empty() || analysis.max_density <= 0) {
        cv::putText(chart, "Sem bordas detectadas", {20, chart_height / 2}, cv::FONT_HERSHEY_SIMPLEX,
                    0.8, {200, 200, 200}, 2, cv::LINE_AA);
        return chart;
    }

    const double scale = static_cast<double>(chart_height - 20) /
                         static_cast<double>(std::max(1, analysis.max_density));

    for (int x = 0; x < width && x < static_cast<int>(analysis.edge_density_per_column.size()); ++x) {
        const int value = analysis.edge_density_per_column[static_cast<size_t>(x)];
        const int bar_height = static_cast<int>(value * scale);
        cv::line(chart, {x, chart_height - 1}, {x, chart_height - 1 - bar_height},
                 {0, 215, 255}, 1, cv::LINE_AA);
    }

    cv::putText(chart, "Densidade de bordas por coluna", {20, 30}, cv::FONT_HERSHEY_SIMPLEX, 0.6,
                {240, 240, 240}, 1, cv::LINE_AA);
    return chart;
}

cv::Mat EdgeAnalyzer::colorizeEdges(const cv::Mat &frame, const cv::Mat &edges) const {
    if (frame.empty() || edges.empty()) {
        return frame.clone();
    }

    cv::Mat overlay = frame.clone();
    cv::Mat colored(edges.size(), CV_8UC3, cv::Scalar(0, 255, 255));
    colored.copyTo(overlay, edges);
    return overlay;
}

} // namespace autonomous_car::services::camera::algorithms
