#include "services/camera/filters/BottomMaskFilter.hpp"

#include <algorithm>

namespace autonomous_car::services::camera::filters {

BottomMaskFilter::BottomMaskFilter(double keep_ratio)
    : keep_ratio_(std::clamp(keep_ratio, 0.0, 1.0)) {}

cv::Mat BottomMaskFilter::apply(const cv::Mat &frame) const {
    if (frame.empty() || keep_ratio_ <= 0.0) {
        return cv::Mat::zeros(frame.size(), frame.type());
    }

    if (keep_ratio_ >= 1.0) {
        return frame.clone();
    }

    cv::Mat result = cv::Mat::zeros(frame.size(), frame.type());
    const int start_row = static_cast<int>(frame.rows * (1.0 - keep_ratio_));
    const cv::Range rows(start_row, frame.rows);
    frame(rows, cv::Range::all())
        .copyTo(result(rows, cv::Range::all()));
    return result;
}

} // namespace autonomous_car::services::camera::filters
