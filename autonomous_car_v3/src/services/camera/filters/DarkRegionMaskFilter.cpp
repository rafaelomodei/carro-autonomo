#include "services/camera/filters/DarkRegionMaskFilter.hpp"

#include <opencv2/imgproc.hpp>

namespace autonomous_car::services::camera::filters {

DarkRegionMaskFilter::DarkRegionMaskFilter(cv::Scalar lower, cv::Scalar upper)
    : lower_(lower), upper_(upper) {}

cv::Mat DarkRegionMaskFilter::apply(const cv::Mat &frame) const {
    if (frame.empty()) {
        return frame;
    }

    cv::Mat hsv;
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
    cv::Mat mask;
    cv::inRange(hsv, lower_, upper_, mask);
    return mask;
}

} // namespace autonomous_car::services::camera::filters
