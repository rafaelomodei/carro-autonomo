#include "services/camera/filters/MorphologyFilter.hpp"

#include <opencv2/imgproc.hpp>

namespace autonomous_car::services::camera::filters {

MorphologyFilter::MorphologyFilter(int operation, cv::Size kernel_size, int iterations)
    : operation_(operation), kernel_size_(kernel_size), iterations_(iterations) {
    if (kernel_size_.width <= 0)
        kernel_size_.width = 3;
    if (kernel_size_.height <= 0)
        kernel_size_.height = 3;
    if (iterations_ < 1)
        iterations_ = 1;
}

cv::Mat MorphologyFilter::apply(const cv::Mat &frame) const {
    if (frame.empty()) {
        return frame;
    }

    cv::Mat result;
    const cv::Mat kernel =
        cv::getStructuringElement(cv::MORPH_RECT, kernel_size_);
    cv::morphologyEx(frame, result, operation_, kernel, {-1, -1}, iterations_);
    return result;
}

} // namespace autonomous_car::services::camera::filters
