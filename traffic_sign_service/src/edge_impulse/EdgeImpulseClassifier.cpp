#include "edge_impulse/EdgeImpulseClassifier.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string_view>

#include "edge_impulse/LabelMapping.hpp"

#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "model-parameters/model_metadata.h"

namespace traffic_sign_service::edge_impulse {
namespace {

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

constexpr const char *kModelCompatibilityError =
    "Modelo Edge Impulse embutido sem classes esperadas para traffic_sign_service.";

std::string normalizeToken(std::string_view value) {
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

ImageResizeMode embeddedResizeMode() {
    switch (EI_CLASSIFIER_RESIZE_MODE) {
    case EI_CLASSIFIER_RESIZE_FIT_SHORTEST:
        return ImageResizeMode::FitShortest;
    case EI_CLASSIFIER_RESIZE_FIT_LONGEST:
        return ImageResizeMode::FitLongest;
    default:
        return ImageResizeMode::Squash;
    }
}

bool embeddedModelUsesGrayscale() {
    if (ei_dsp_blocks_size == 0 || ei_dsp_blocks[0].config == nullptr) {
        return false;
    }

    const auto *config =
        static_cast<const ei_dsp_config_image_t *>(ei_dsp_blocks[0].config);
    if (config->channels == nullptr) {
        return false;
    }

    return normalizeToken(config->channels) == "grayscale";
}

} // namespace

EdgeImpulseClassifier::EdgeImpulseClassifier(config::ServiceConfig config)
    : config_{std::move(config)},
      input_buffer_(EI_CLASSIFIER_RAW_SAMPLE_COUNT, 0.0f) {
    model_labels_.reserve(EI_CLASSIFIER_LABEL_COUNT);
    for (int index = 0; index < EI_CLASSIFIER_LABEL_COUNT; ++index) {
        model_labels_.push_back(ei_classifier_inferencing_categories[index]);
    }

    model_labels_summary_ = describeModelLabels();
    model_input_summary_ = describeImageSignal(
        {inputSize(), embeddedResizeMode(),
         config_.traffic_sign_preprocess_grayscale || embeddedModelUsesGrayscale()},
        embeddedModelUsesGrayscale() ? std::optional<int>(1) : std::optional<int>(3));
    std::string validation_error;
    model_ready_ = validateModelLabels(validation_error);
    if (!model_ready_) {
        last_error_ = std::move(validation_error);
    }
}

TrafficSignFrameResult EdgeImpulseClassifier::detect(const TrafficSignInferenceInput &input,
                                                     std::int64_t timestamp_ms) {
    const cv::Mat &frame = input.frame;
    const cv::Size full_frame_size =
        input.full_frame_size.area() > 0 ? input.full_frame_size : frame.size();
    const TrafficSignRoi roi = input.roi.value_or(buildTrafficSignRoi(
        full_frame_size, config_.traffic_sign_roi_left_ratio,
        config_.traffic_sign_roi_right_ratio, config_.traffic_sign_roi_top_ratio,
        config_.traffic_sign_roi_bottom_ratio, config_.traffic_sign_debug_roi_enabled));

    if (!model_ready_) {
        return makeErrorResult(full_frame_size, timestamp_ms, last_error_);
    }

    if (frame.empty()) {
        last_error_ = "Frame vazio recebido pelo detector embutido.";
        return makeErrorResult(full_frame_size, timestamp_ms, last_error_);
    }

    if (roi.frame_rect.area() <= 0 || roi.frame_rect.x < 0 || roi.frame_rect.y < 0 ||
        roi.frame_rect.x + roi.frame_rect.width > full_frame_size.width ||
        roi.frame_rect.y + roi.frame_rect.height > full_frame_size.height) {
        last_error_ = "ROI de sinalizacao invalida para o frame atual.";
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
    if (input_buffer_.empty() ||
        input_buffer_.size() != static_cast<std::size_t>(EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE)) {
        last_error_ = "Falha ao preparar a entrada RGB888 do modelo embutido.";
        return makeErrorResult(full_frame_size, timestamp_ms, last_error_);
    }

    signal_t signal{};
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

    ei_impulse_result_t result{};
    const EI_IMPULSE_ERROR classifier_error = run_classifier(&signal, &result, false);
    if (classifier_error != EI_IMPULSE_OK) {
        last_error_ = "Falha ao executar inferencia do modelo embutido.";
        return makeErrorResult(full_frame_size, timestamp_ms, last_error_);
    }

    last_error_.clear();

    TrafficSignFrameResult frame_result =
        makeTrafficSignFrameResult(TrafficSignDetectorState::Idle, roi, timestamp_ms);
    attachModelDebugInfo(frame_result, frame, roi_frame, input.capture_debug_frames);

    const cv::Size model_input_size(EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT);
    const cv::Rect roi_bounds(0, 0, roi.frame_rect.width, roi.frame_rect.height);
    const cv::Rect frame_bounds(0, 0, full_frame_size.width, full_frame_size.height);

    for (std::uint32_t index = 0; index < result.bounding_boxes_count; ++index) {
        const auto &bbox = result.bounding_boxes[index];
        if (bbox.value <= 0.0f || bbox.label == nullptr ||
            bbox.value < config_.traffic_sign_min_confidence) {
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

        const TrafficSignalId signal_id = trafficSignalIdFromModelLabel(bbox.label);

        SignalDetection detection;
        detection.signal_id = signal_id;
        detection.model_label = bbox.label;
        detection.display_label = displayLabel(signal_id);
        detection.confidence = static_cast<double>(bbox.value);
        detection.bbox_roi = roi_box;
        detection.bbox_frame = frame_box;
        detection.consecutive_frames = 1;
        detection.required_frames = config_.traffic_sign_min_consecutive_frames;
        detection.last_seen_at_ms = timestamp_ms;
        frame_result.raw_detections.push_back(std::move(detection));
    }

    std::sort(frame_result.raw_detections.begin(), frame_result.raw_detections.end(),
              [](const SignalDetection &lhs, const SignalDetection &rhs) {
                  return lhs.confidence > rhs.confidence;
              });

    if (static_cast<int>(frame_result.raw_detections.size()) >
        config_.traffic_sign_max_raw_detections) {
        frame_result.raw_detections.resize(
            static_cast<std::size_t>(config_.traffic_sign_max_raw_detections));
    }

    return frame_result;
}

std::vector<std::string> EdgeImpulseClassifier::modelLabels() const { return model_labels_; }

cv::Size EdgeImpulseClassifier::inputSize() const {
    return cv::Size(EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT);
}

std::string EdgeImpulseClassifier::backendName() const { return "embedded_cpp"; }

bool EdgeImpulseClassifier::validateModelLabels(std::string &error_message) const {
    bool has_stop = false;
    bool has_turn_left = false;

    for (int index = 0; index < EI_CLASSIFIER_LABEL_COUNT; ++index) {
        const TrafficSignalId signal_id =
            trafficSignalIdFromModelLabel(ei_classifier_inferencing_categories[index]);
        has_stop = has_stop || signal_id == TrafficSignalId::Stop;
        has_turn_left = has_turn_left || signal_id == TrafficSignalId::TurnLeft;
    }

    if (has_stop && has_turn_left) {
        return true;
    }

    error_message =
        std::string(kModelCompatibilityError) + " Labels atuais: " + describeModelLabels();
    return false;
}

TrafficSignFrameResult EdgeImpulseClassifier::makeErrorResult(
    const cv::Size &frame_size, std::int64_t timestamp_ms, const std::string &error_message) const {
    TrafficSignFrameResult result = makeTrafficSignFrameResult(
        TrafficSignDetectorState::Error,
        buildTrafficSignRoi(frame_size, config_.traffic_sign_roi_left_ratio,
                            config_.traffic_sign_roi_right_ratio,
                            config_.traffic_sign_roi_top_ratio,
                            config_.traffic_sign_roi_bottom_ratio,
                            config_.traffic_sign_debug_roi_enabled),
        timestamp_ms, error_message);
    result.model_labels = model_labels_;
    result.model_labels_summary = model_labels_summary_;
    result.model_input_summary = model_input_summary_;
    return result;
}

void EdgeImpulseClassifier::attachModelDebugInfo(TrafficSignFrameResult &frame_result,
                                                 const cv::Mat &frame, const cv::Mat &roi_frame,
                                                 bool capture_debug_frames) {
    frame_result.model_labels = model_labels_;
    frame_result.model_labels_summary = model_labels_summary_;
    frame_result.model_input_summary = model_input_summary_;
    if (!capture_debug_frames) {
        frame_result.debug_frame.release();
        frame_result.debug_roi_frame.release();
        frame_result.debug_model_input_frame.release();
        return;
    }

    if (!frame.empty()) {
        frame_result.debug_frame = frame.clone();
    }
    if (!roi_frame.empty()) {
        frame_result.debug_roi_frame = roi_frame.clone();
    }
    if (!resized_buffer_.empty()) {
        frame_result.debug_model_input_frame = resized_buffer_.clone();
    }
}

void EdgeImpulseClassifier::prepareInputBuffer(const cv::Mat &roi_frame) {
    const EncodedImageSignal encoded = encodeImageSignal(
        roi_frame,
        {cv::Size(EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT),
         embeddedResizeMode(),
         config_.traffic_sign_preprocess_grayscale || embeddedModelUsesGrayscale()});

    resized_buffer_ = encoded.debug_image;
    input_buffer_ = encoded.features;
}

} // namespace traffic_sign_service::edge_impulse
