#include "edge_impulse/ImageSignalEncoder.hpp"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdint>

#include <opencv2/imgproc.hpp>

namespace {

std::string normalize(std::string_view value) {
    std::string normalized;
    normalized.reserve(value.size());

    for (char ch : value) {
        if (std::isalnum(static_cast<unsigned char>(ch)) != 0) {
            normalized.push_back(
                static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        }
    }

    return normalized;
}

cv::Mat ensureBgr(const cv::Mat &frame) {
    if (frame.empty()) {
        return {};
    }

    if (frame.channels() == 3) {
        return frame;
    }

    cv::Mat converted;
    if (frame.channels() == 1) {
        cv::cvtColor(frame, converted, cv::COLOR_GRAY2BGR);
    } else if (frame.channels() == 4) {
        cv::cvtColor(frame, converted, cv::COLOR_BGRA2BGR);
    } else {
        converted = frame.clone();
    }
    return converted;
}

cv::Mat resizeSquash(const cv::Mat &image, const cv::Size &output_size) {
    cv::Mat resized;
    cv::resize(image, resized, output_size, 0.0, 0.0, cv::INTER_AREA);
    return resized;
}

cv::Mat resizeFitShortest(const cv::Mat &image, const cv::Size &output_size) {
    if (image.empty() || output_size.width <= 0 || output_size.height <= 0) {
        return {};
    }

    const double target_ratio =
        static_cast<double>(output_size.width) / static_cast<double>(output_size.height);
    const double source_ratio =
        static_cast<double>(image.cols) / static_cast<double>(image.rows);

    cv::Rect crop_rect(0, 0, image.cols, image.rows);
    if (source_ratio > target_ratio) {
        const int new_width = std::max(
            1, static_cast<int>(std::lround(static_cast<double>(image.rows) * target_ratio)));
        crop_rect.x = std::max(0, (image.cols - new_width) / 2);
        crop_rect.width = std::min(new_width, image.cols - crop_rect.x);
    } else if (source_ratio < target_ratio) {
        const int new_height = std::max(
            1, static_cast<int>(std::lround(static_cast<double>(image.cols) / target_ratio)));
        crop_rect.y = std::max(0, (image.rows - new_height) / 2);
        crop_rect.height = std::min(new_height, image.rows - crop_rect.y);
    }

    cv::Mat resized;
    cv::resize(image(crop_rect), resized, output_size, 0.0, 0.0, cv::INTER_AREA);
    return resized;
}

cv::Mat resizeFitLongest(const cv::Mat &image, const cv::Size &output_size) {
    if (image.empty() || output_size.width <= 0 || output_size.height <= 0) {
        return {};
    }

    const double scale_x =
        static_cast<double>(output_size.width) / static_cast<double>(image.cols);
    const double scale_y =
        static_cast<double>(output_size.height) / static_cast<double>(image.rows);
    const double scale = std::min(scale_x, scale_y);

    const int new_width =
        std::max(1, static_cast<int>(std::lround(static_cast<double>(image.cols) * scale)));
    const int new_height =
        std::max(1, static_cast<int>(std::lround(static_cast<double>(image.rows) * scale)));

    cv::Mat resized;
    cv::resize(image, resized, {new_width, new_height}, 0.0, 0.0, cv::INTER_AREA);

    cv::Mat letterboxed(output_size, CV_8UC3, cv::Scalar(0, 0, 0));
    const int offset_x = std::max(0, (output_size.width - new_width) / 2);
    const int offset_y = std::max(0, (output_size.height - new_height) / 2);
    resized.copyTo(letterboxed(cv::Rect(offset_x, offset_y, resized.cols, resized.rows)));
    return letterboxed;
}

cv::Mat resizeForModel(const cv::Mat &image,
                       const traffic_sign_service::edge_impulse::ImageSignalEncodingOptions
                           &options) {
    switch (options.resize_mode) {
    case traffic_sign_service::edge_impulse::ImageResizeMode::FitShortest:
        return resizeFitShortest(image, options.input_size);
    case traffic_sign_service::edge_impulse::ImageResizeMode::FitLongest:
        return resizeFitLongest(image, options.input_size);
    case traffic_sign_service::edge_impulse::ImageResizeMode::Squash:
        return resizeSquash(image, options.input_size);
    }

    return resizeSquash(image, options.input_size);
}

} // namespace

namespace traffic_sign_service::edge_impulse {

std::string_view toString(ImageResizeMode mode) {
    switch (mode) {
    case ImageResizeMode::FitShortest:
        return "fit-shortest";
    case ImageResizeMode::FitLongest:
        return "fit-longest";
    case ImageResizeMode::Squash:
        return "squash";
    }

    return "squash";
}

std::optional<ImageResizeMode> imageResizeModeFromString(std::string_view value) {
    const std::string normalized = normalize(value);
    if (normalized == "fitshortest") {
        return ImageResizeMode::FitShortest;
    }
    if (normalized == "fitlongest") {
        return ImageResizeMode::FitLongest;
    }
    if (normalized == "squash") {
        return ImageResizeMode::Squash;
    }
    return std::nullopt;
}

EncodedImageSignal encodeImageSignal(const cv::Mat &frame,
                                     const ImageSignalEncodingOptions &options) {
    EncodedImageSignal encoded;
    if (frame.empty() || options.input_size.width <= 0 || options.input_size.height <= 0) {
        return encoded;
    }

    const cv::Mat bgr_frame = ensureBgr(frame);
    const cv::Mat resized = resizeForModel(bgr_frame, options);
    if (resized.empty()) {
        return encoded;
    }

    encoded.features.reserve(static_cast<std::size_t>(resized.rows * resized.cols));

    if (options.preprocess_grayscale) {
        cv::cvtColor(resized, encoded.debug_image, cv::COLOR_BGR2GRAY);
        for (int row = 0; row < encoded.debug_image.rows; ++row) {
            const auto *row_ptr = encoded.debug_image.ptr<std::uint8_t>(row);
            for (int col = 0; col < encoded.debug_image.cols; ++col) {
                const std::uint32_t pixel = row_ptr[col];
                encoded.features.push_back(static_cast<float>((pixel << 16) | (pixel << 8) |
                                                              pixel));
            }
        }
        return encoded;
    }

    encoded.debug_image = resized;
    for (int row = 0; row < resized.rows; ++row) {
        const auto *row_ptr = resized.ptr<std::uint8_t>(row);
        for (int col = 0; col < resized.cols; ++col) {
            const std::size_t offset = static_cast<std::size_t>(col) * 3U;
            const std::uint32_t blue = row_ptr[offset + 0];
            const std::uint32_t green = row_ptr[offset + 1];
            const std::uint32_t red = row_ptr[offset + 2];
            encoded.features.push_back(
                static_cast<float>((red << 16) | (green << 8) | blue));
        }
    }

    return encoded;
}

std::string describeImageSignal(const ImageSignalEncodingOptions &options,
                                std::optional<int> model_channels) {
    std::string summary = "resize=" + std::string(toString(options.resize_mode));
    summary += " | preprocess=";
    summary += options.preprocess_grayscale ? "grayscale" : "rgb";
    if (model_channels.has_value()) {
        summary += " | modelo=" + std::to_string(*model_channels) + " canal";
        if (*model_channels != 1) {
            summary += "s";
        }
    }
    return summary;
}

} // namespace traffic_sign_service::edge_impulse
