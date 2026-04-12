#include "services/traffic_sign_detection/EdgeImpulseTrafficSignDetector.hpp"

#include <algorithm>
#include <cstring>
#include <sstream>

#include <opencv2/imgproc.hpp>

#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "model-parameters/model_metadata.h"

namespace autonomous_car::services::traffic_sign_detection {
namespace {

constexpr const char *kModelCompatibilityError =
    "Modelo Edge Impulse sem classes esperadas. Substitua o export pelo pacote atualizado do V3.";

std::string describeModelLabels() {
    std::ostringstream stream;
    for (int index = 0; index < EI_CLASSIFIER_LABEL_COUNT; ++index) {
        if (index > 0) {
            stream << ", ";
        }
        stream << ei_classifier_inferencing_categories[index];
    }
    return stream.str();
}

} // namespace

EdgeImpulseTrafficSignDetector::EdgeImpulseTrafficSignDetector(TrafficSignConfig config)
    : config_(std::move(config)),
      input_buffer_(EI_CLASSIFIER_RAW_SAMPLE_COUNT, 0.0f) {
    std::string validation_error;
    model_ready_ = validateModelLabels(validation_error);
    if (!model_ready_) {
        last_error_ = std::move(validation_error);
    }
}

TrafficSignFrameResult EdgeImpulseTrafficSignDetector::detect(const cv::Mat &frame,
                                                              std::int64_t timestamp_ms) {
    TrafficSignInferenceInput input;
    input.frame = frame;
    input.full_frame_size = frame.size();
    return detect(input, timestamp_ms);
}

TrafficSignFrameResult EdgeImpulseTrafficSignDetector::detect(
    const TrafficSignInferenceInput &input, std::int64_t timestamp_ms) {
    const cv::Mat &frame = input.frame;
    const cv::Size full_frame_size =
        input.full_frame_size.area() > 0 ? input.full_frame_size : frame.size();
    const TrafficSignRoi roi = input.roi.value_or(buildTrafficSignRoi(
        full_frame_size, config_.roi_left_ratio, config_.roi_right_ratio,
        config_.roi_top_ratio, config_.roi_bottom_ratio, config_.debug_roi_enabled));

    if (!config_.enabled) {
        return makeTrafficSignFrameResult(TrafficSignDetectorState::Disabled, roi, timestamp_ms);
    }

    if (!model_ready_) {
        return makeErrorResult(frame.size(), timestamp_ms, last_error_);
    }

    if (frame.empty()) {
        last_error_ = "Frame vazio recebido pelo detector de sinalizacao.";
        return makeErrorResult(full_frame_size, timestamp_ms, last_error_);
    }

    if (roi.frame_rect.area() <= 0 || roi.frame_rect.width <= 0 || roi.frame_rect.height <= 0 ||
        roi.frame_rect.x < 0 || roi.frame_rect.y < 0 ||
        roi.frame_rect.x + roi.frame_rect.width > full_frame_size.width ||
        roi.frame_rect.y + roi.frame_rect.height > full_frame_size.height) {
        last_error_ = "ROI de sinalizacao invalido para o frame atual.";
        return makeErrorResult(full_frame_size, timestamp_ms, last_error_);
    }

    cv::Mat roi_frame;
    if (input.roi.has_value()) {
        if (frame.cols != roi.frame_rect.width || frame.rows != roi.frame_rect.height) {
            last_error_ = "Frame de ROI nao corresponde ao recorte configurado.";
            return makeErrorResult(full_frame_size, timestamp_ms, last_error_);
        }
        roi_frame = frame;
    } else {
        if (roi.frame_rect.x + roi.frame_rect.width > frame.cols ||
            roi.frame_rect.y + roi.frame_rect.height > frame.rows) {
            last_error_ = "ROI de sinalizacao fora dos limites do frame recebido.";
            return makeErrorResult(full_frame_size, timestamp_ms, last_error_);
        }
        roi_frame = frame(roi.frame_rect);
    }

    prepareInputBuffer(roi_frame);

    ei::signal_t signal;
    signal.total_length = input_buffer_.size();
    signal.get_data = [this](size_t offset, size_t length, float *out_ptr) -> int {
        if (offset + length > input_buffer_.size()) {
            return -1;
        }

        std::copy(input_buffer_.begin() + static_cast<std::ptrdiff_t>(offset),
                  input_buffer_.begin() + static_cast<std::ptrdiff_t>(offset + length),
                  out_ptr);
        return 0;
    };

    ei_impulse_result_t result = {};
    const EI_IMPULSE_ERROR run_error = run_classifier(&signal, &result, false);
    if (run_error != EI_IMPULSE_OK) {
        last_error_ = "Falha ao executar inferencia do Edge Impulse.";
        return makeErrorResult(full_frame_size, timestamp_ms, last_error_);
    }

    last_error_.clear();

    TrafficSignFrameResult frame_result =
        makeTrafficSignFrameResult(TrafficSignDetectorState::Idle, roi, timestamp_ms);

    const cv::Size model_input_size(EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT);
    const cv::Rect roi_bounds(0, 0, roi.frame_rect.width, roi.frame_rect.height);
    const cv::Rect frame_bounds(0, 0, full_frame_size.width, full_frame_size.height);

    for (std::uint32_t index = 0; index < result.bounding_boxes_count; ++index) {
        const auto &bbox = result.bounding_boxes[index];
        if (bbox.value <= 0.0f || bbox.label == nullptr ||
            bbox.value < config_.min_confidence) {
            continue;
        }

        TrafficSignBoundingBox model_box{
            static_cast<int>(bbox.x),
            static_cast<int>(bbox.y),
            static_cast<int>(bbox.width),
            static_cast<int>(bbox.height),
        };
        TrafficSignBoundingBox roi_box =
            clampBoundingBox(scaleBoundingBox(model_box, model_input_size, roi.frame_rect.size()),
                             roi_bounds);
        TrafficSignBoundingBox frame_box =
            clampBoundingBox(translateBoundingBox(roi_box, roi.frame_rect.tl()), frame_bounds);

        TrafficSignDetection detection;
        detection.sign_id = trafficSignIdFromModelLabel(bbox.label);
        detection.model_label = bbox.label;
        detection.display_label = displayLabel(detection.sign_id);
        detection.confidence_score = static_cast<double>(bbox.value);
        detection.bbox_roi = roi_box;
        detection.bbox_frame = frame_box;
        detection.consecutive_frames = 1;
        detection.required_frames = config_.min_consecutive_frames;
        detection.last_seen_at_ms = timestamp_ms;

        frame_result.raw_detections.push_back(std::move(detection));
    }

    std::sort(frame_result.raw_detections.begin(), frame_result.raw_detections.end(),
              [](const TrafficSignDetection &lhs, const TrafficSignDetection &rhs) {
                  return lhs.confidence_score > rhs.confidence_score;
              });

    if (static_cast<int>(frame_result.raw_detections.size()) > config_.max_raw_detections) {
        frame_result.raw_detections.resize(
            static_cast<std::size_t>(config_.max_raw_detections));
    }

    return frame_result;
}

bool EdgeImpulseTrafficSignDetector::validateModelLabels(std::string &error_message) const {
    bool has_stop = false;
    bool has_turn_left = false;

    for (int index = 0; index < EI_CLASSIFIER_LABEL_COUNT; ++index) {
        const TrafficSignId sign_id =
            trafficSignIdFromModelLabel(ei_classifier_inferencing_categories[index]);
        has_stop = has_stop || sign_id == TrafficSignId::Stop;
        has_turn_left = has_turn_left || sign_id == TrafficSignId::TurnLeft;
    }

    if (has_stop && has_turn_left) {
        return true;
    }

    error_message =
        std::string(kModelCompatibilityError) + " Labels atuais: " + describeModelLabels();
    return false;
}

TrafficSignFrameResult EdgeImpulseTrafficSignDetector::makeErrorResult(
    const cv::Size &frame_size, std::int64_t timestamp_ms,
    const std::string &error_message) const {
    return makeTrafficSignFrameResult(
        TrafficSignDetectorState::Error,
        buildTrafficSignRoi(frame_size, config_.roi_left_ratio, config_.roi_right_ratio,
                            config_.roi_top_ratio, config_.roi_bottom_ratio,
                            config_.debug_roi_enabled),
        timestamp_ms, error_message);
}

void EdgeImpulseTrafficSignDetector::prepareInputBuffer(const cv::Mat &roi_frame) {
    if (roi_frame.channels() == 1) {
        grayscale_buffer_ = roi_frame;
    } else if (roi_frame.channels() == 4) {
        cv::cvtColor(roi_frame, grayscale_buffer_, cv::COLOR_BGRA2GRAY);
    } else {
        cv::cvtColor(roi_frame, grayscale_buffer_, cv::COLOR_BGR2GRAY);
    }

    cv::resize(grayscale_buffer_, resized_buffer_,
               cv::Size(EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT), 0.0, 0.0,
               cv::INTER_AREA);

    const std::size_t expected_size =
        static_cast<std::size_t>(resized_buffer_.rows * resized_buffer_.cols);
    if (input_buffer_.size() != expected_size) {
        input_buffer_.assign(expected_size, 0.0f);
    }

    for (int row = 0; row < resized_buffer_.rows; ++row) {
        const auto *row_ptr = resized_buffer_.ptr<std::uint8_t>(row);
        for (int col = 0; col < resized_buffer_.cols; ++col) {
            input_buffer_[static_cast<std::size_t>(row * resized_buffer_.cols + col)] =
                static_cast<float>(row_ptr[col]);
        }
    }
}

} // namespace autonomous_car::services::traffic_sign_detection
