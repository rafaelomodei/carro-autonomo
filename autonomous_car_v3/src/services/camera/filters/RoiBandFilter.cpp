#include "services/camera/filters/RoiBandFilter.hpp"

#include <algorithm>
#include <cmath>

namespace autonomous_car::services::camera::filters {

namespace {
constexpr double ClampRatio(double value) { return std::clamp(value, 0.0, 1.0); }
}

RoiBandFilter::RoiBandFilter(double start_ratio, double end_ratio)
    : start_ratio_(ClampRatio(std::min(start_ratio, end_ratio))),
      end_ratio_(ClampRatio(std::max(start_ratio, end_ratio))) {}

cv::Mat RoiBandFilter::apply(const cv::Mat &frame) const {
    if (frame.empty()) {
        return cv::Mat();
    }

    if (end_ratio_ <= start_ratio_) {
        return cv::Mat::zeros(frame.size(), frame.type());
    }

    const int total_rows = frame.rows;
    int start_row = static_cast<int>(std::round(total_rows * start_ratio_));
    int end_row = static_cast<int>(std::round(total_rows * end_ratio_));
    start_row = std::clamp(start_row, 0, total_rows);
    end_row = std::clamp(end_row, 0, total_rows);

    if (start_row >= end_row) {
        return cv::Mat::zeros(frame.size(), frame.type());
    }

    cv::Mat result = cv::Mat::zeros(frame.size(), frame.type());
    const cv::Range rows(start_row, end_row);
    frame(rows, cv::Range::all()).copyTo(result(rows, cv::Range::all()));
    return result;
}

} // namespace autonomous_car::services::camera::filters
