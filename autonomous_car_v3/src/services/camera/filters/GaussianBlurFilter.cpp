#include "services/camera/filters/GaussianBlurFilter.hpp"

#include <opencv2/imgproc.hpp>

namespace autonomous_car::services::camera::filters {

GaussianBlurFilter::GaussianBlurFilter(cv::Size kernel_size, double sigma_x)
    : kernel_size_(kernel_size), sigma_x_(sigma_x) {
    if (kernel_size_.width % 2 == 0) {
        kernel_size_.width += 1;
    }
    if (kernel_size_.height % 2 == 0) {
        kernel_size_.height += 1;
    }
}

cv::Mat GaussianBlurFilter::apply(const cv::Mat &frame) const {
    if (frame.empty()) {
        return frame;
    }
    cv::Mat result;
    cv::GaussianBlur(frame, result, kernel_size_, sigma_x_);
    return result;
}

} // namespace autonomous_car::services::camera::filters
