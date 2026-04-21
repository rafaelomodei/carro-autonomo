#include "edge_impulse/EdgeImpulseClassifier.hpp"

#include <stdexcept>
#include <vector>

#include <opencv2/imgproc.hpp>

#include "edge_impulse/LabelMapping.hpp"

#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "edge-impulse-sdk/dsp/numpy.hpp"

namespace traffic_sign_service::edge_impulse {

std::vector<SignalDetection> EdgeImpulseClassifier::detect(const cv::Mat &frame) {
    if (frame.empty()) {
        return {};
    }

    cv::Mat grayscale;
    if (frame.channels() == 1) {
        grayscale = frame;
    } else {
        cv::cvtColor(frame, grayscale, cv::COLOR_BGR2GRAY);
    }

    cv::Mat resized;
    cv::resize(grayscale, resized,
               cv::Size(EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT), 0.0, 0.0,
               cv::INTER_AREA);

    std::vector<float> features(EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    for (int row = 0; row < resized.rows; ++row) {
        for (int col = 0; col < resized.cols; ++col) {
            const auto value = resized.at<unsigned char>(row, col);
            features[static_cast<std::size_t>(row * resized.cols + col)] =
                static_cast<float>(value);
        }
    }

    signal_t signal{};
    const int signal_error = ei::numpy::signal_from_buffer(features.data(), features.size(), &signal);
    if (signal_error != 0) {
        throw std::runtime_error("Falha ao montar signal_t do Edge Impulse.");
    }

    ei_impulse_result_t result{};
    const auto classifier_error = run_classifier(&signal, &result, false);
    if (classifier_error != EI_IMPULSE_OK) {
        throw std::runtime_error("run_classifier retornou erro no Edge Impulse.");
    }

    std::vector<SignalDetection> detections;
    detections.reserve(result.bounding_boxes_count);

    for (std::uint32_t index = 0; index < result.bounding_boxes_count; ++index) {
        const auto &box = result.bounding_boxes[index];
        if (box.value <= 0.0f || box.label == nullptr) {
            continue;
        }

        const auto signal_id = trafficSignalIdFromModelLabel(box.label);
        if (!signal_id) {
            continue;
        }

        detections.push_back(SignalDetection{
            *signal_id,
            static_cast<double>(box.value),
            static_cast<int>(box.x),
            static_cast<int>(box.y),
            static_cast<int>(box.width),
            static_cast<int>(box.height),
        });
    }

    return detections;
}

} // namespace traffic_sign_service::edge_impulse
