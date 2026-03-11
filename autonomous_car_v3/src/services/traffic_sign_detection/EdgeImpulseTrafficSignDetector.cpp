#include "services/traffic_sign_detection/EdgeImpulseTrafficSignDetector.hpp"

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <unordered_map>
#include <vector>

#include <opencv2/imgproc.hpp>

#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "model-parameters/model_variables.h"

namespace autonomous_car::services::traffic_sign_detection {
namespace {

#ifndef EI_CLASSIFIER_RAW_FRAME_SIZE
#define EI_CLASSIFIER_RAW_FRAME_SIZE                                                     \
    (EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT *                            \
     EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME)
#endif

DetectionBox rectToBox(const cv::Rect &rect) {
    return {rect.x, rect.y, rect.width, rect.height};
}

cv::Rect clampRectToBounds(const cv::Rect &rect, const cv::Size &bounds) {
    const cv::Rect full_bounds(0, 0, std::max(0, bounds.width), std::max(0, bounds.height));
    return rect & full_bounds;
}

} // namespace

EdgeImpulseTrafficSignDetector::EdgeImpulseTrafficSignDetector(TrafficSignConfig config)
    : config_(std::move(config)) {
    std::vector<std::string> categories;
    categories.reserve(EI_CLASSIFIER_LABEL_COUNT);
    for (std::size_t index = 0; index < EI_CLASSIFIER_LABEL_COUNT; ++index) {
        categories.emplace_back(ei_classifier_inferencing_categories[index]);
    }
    model_warning_ = buildModelCompatibilityWarning(categories);
}

void EdgeImpulseTrafficSignDetector::updateConfig(const TrafficSignConfig &config) {
    config_ = config;
}

const TrafficSignConfig &EdgeImpulseTrafficSignDetector::config() const noexcept { return config_; }

TrafficSignDetectionFrame EdgeImpulseTrafficSignDetector::detect(const cv::Mat &frame) const {
    TrafficSignDetectionFrame output;
    output.enabled = config_.enabled;
    output.last_error = model_warning_;

    if (frame.empty()) {
        output.enabled = false;
        output.last_error = "Frame vazio recebido pelo detector de sinalizacao.";
        return output;
    }

    const cv::Rect roi = computeTrafficSignRoi(frame.size(), config_);
    output.roi = {roi.x, roi.y, roi.width, roi.height, config_.roi_right_width_ratio,
                  config_.roi_top_ratio, config_.roi_bottom_ratio};

    if (!config_.enabled || roi.area() <= 0) {
        return output;
    }

    cv::Mat roi_bgr = frame(roi).clone();
    cv::Mat roi_rgb;
    cv::cvtColor(roi_bgr, roi_rgb, cv::COLOR_BGR2RGB);
    cv::resize(roi_rgb, roi_rgb, cv::Size(EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT));

    std::vector<float> features(EI_CLASSIFIER_RAW_FRAME_SIZE, 0.0f);
    std::size_t feature_index = 0;
    for (int y = 0; y < roi_rgb.rows; ++y) {
        for (int x = 0; x < roi_rgb.cols; ++x) {
            const auto pixel = roi_rgb.at<cv::Vec3b>(y, x);
            if (feature_index >= features.size()) {
                break;
            }

            const std::uint32_t packed_rgb =
                (static_cast<std::uint32_t>(pixel[0]) << 16U) |
                (static_cast<std::uint32_t>(pixel[1]) << 8U) |
                static_cast<std::uint32_t>(pixel[2]);
            features[feature_index++] = static_cast<float>(packed_rgb);
        }
    }

    auto get_feature_data = [&features](size_t offset, size_t length, float *out_ptr) -> int {
        std::memcpy(out_ptr, features.data() + offset, length * sizeof(float));
        return 0;
    };

    ei::signal_t signal{};
    signal.total_length = EI_CLASSIFIER_RAW_FRAME_SIZE;
    signal.get_data = get_feature_data;
    ei_impulse_result_t result{};
    if (run_classifier(&signal, &result, false) != EI_IMPULSE_OK) {
        output.last_error = "Falha ao executar run_classifier no Edge Impulse.";
        return output;
    }

    std::unordered_map<int, TrafficSignDetection> best_by_sign;
    std::string unknown_label_warning;
    const double scale_x =
        static_cast<double>(roi.width) / static_cast<double>(EI_CLASSIFIER_INPUT_WIDTH);
    const double scale_y =
        static_cast<double>(roi.height) / static_cast<double>(EI_CLASSIFIER_INPUT_HEIGHT);

    for (std::size_t index = 0; index < result.bounding_boxes_count; ++index) {
        const auto &box = result.bounding_boxes[index];
        if (box.value < static_cast<float>(config_.min_confidence)) {
            continue;
        }

        const auto sign_id = trafficSignIdFromModelLabel(box.label);
        if (!sign_id.has_value()) {
            unknown_label_warning = "Label desconhecida retornada pelo modelo: " +
                                    std::string(box.label);
            continue;
        }

        const cv::Rect box_in_model_space(
            static_cast<int>(std::lround(static_cast<double>(box.x) * scale_x)),
            static_cast<int>(std::lround(static_cast<double>(box.y) * scale_y)),
            std::max(1, static_cast<int>(std::lround(static_cast<double>(box.width) * scale_x))),
            std::max(1, static_cast<int>(std::lround(static_cast<double>(box.height) * scale_y))));
        const cv::Rect box_in_roi = clampRectToBounds(box_in_model_space, roi.size());
        const cv::Rect box_in_frame =
            clampRectToBounds(box_in_roi + roi.tl(), frame.size());

        TrafficSignDetection detection;
        detection.sign_id = *sign_id;
        detection.model_label = box.label;
        detection.display_label = displayLabel(*sign_id);
        detection.confidence_score = static_cast<double>(box.value);
        detection.bbox_roi = rectToBox(box_in_roi);
        detection.bbox_frame = rectToBox(box_in_frame);

        const int key = static_cast<int>(*sign_id);
        const auto current = best_by_sign.find(key);
        if (current == best_by_sign.end() || isBetterDetection(detection, current->second)) {
            best_by_sign[key] = detection;
        }
    }

    output.raw_detections.reserve(best_by_sign.size());
    for (const auto &[_, detection] : best_by_sign) {
        output.raw_detections.push_back(detection);
    }

    std::sort(output.raw_detections.begin(), output.raw_detections.end(),
              [](const auto &lhs, const auto &rhs) { return isBetterDetection(lhs, rhs); });

    if (static_cast<int>(output.raw_detections.size()) > config_.max_raw_detections) {
        output.raw_detections.resize(static_cast<std::size_t>(config_.max_raw_detections));
    }

    if (!unknown_label_warning.empty()) {
        output.last_error = unknown_label_warning;
    }

    return output;
}

} // namespace autonomous_car::services::traffic_sign_detection
