#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <opencv2/core.hpp>

namespace traffic_sign_service::edge_impulse {

enum class ImageResizeMode {
    FitShortest,
    FitLongest,
    Squash,
};

struct ImageSignalEncodingOptions {
    cv::Size input_size;
    ImageResizeMode resize_mode{ImageResizeMode::Squash};
    bool preprocess_grayscale{false};
};

struct EncodedImageSignal {
    cv::Mat debug_image;
    std::vector<float> features;
};

std::string_view toString(ImageResizeMode mode);
std::optional<ImageResizeMode> imageResizeModeFromString(std::string_view value);
EncodedImageSignal encodeImageSignal(const cv::Mat &frame,
                                     const ImageSignalEncodingOptions &options);
std::string describeImageSignal(const ImageSignalEncodingOptions &options,
                                std::optional<int> model_channels = std::nullopt);

} // namespace traffic_sign_service::edge_impulse
