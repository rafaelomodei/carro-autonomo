#include "services/traffic_sign_detection/TrafficSignTemporalFilter.hpp"

#include <algorithm>

namespace autonomous_car::services::traffic_sign_detection {

TrafficSignTemporalFilter::TrafficSignTemporalFilter(TrafficSignConfig config)
    : config_(std::move(config)) {}

void TrafficSignTemporalFilter::updateConfig(const TrafficSignConfig &config) {
    config_ = config;
    if (!config_.enabled) {
        reset();
        return;
    }

    if (candidate_) {
        candidate_->required_frames = config_.min_consecutive_frames;
    }
    if (active_detection_) {
        active_detection_->required_frames = config_.min_consecutive_frames;
        active_detection_->consecutive_frames =
            std::max(active_detection_->consecutive_frames, config_.min_consecutive_frames);
    }
}

void TrafficSignTemporalFilter::reset() {
    candidate_.reset();
    active_detection_.reset();
    active_missed_frames_ = 0;
}

TrafficSignFrameResult TrafficSignTemporalFilter::update(const TrafficSignDetectionFrame &frame,
                                                         std::int64_t timestamp_ms) {
    if (!frame.enabled) {
        reset();
        return buildSnapshot(frame);
    }

    const TrafficSignDetection *best_detection =
        frame.raw_detections.empty() ? nullptr : &frame.raw_detections.front();

    if (active_detection_) {
        const bool active_seen =
            best_detection && best_detection->sign_id == active_detection_->sign_id;

        if (active_seen) {
            active_detection_->model_label = best_detection->model_label;
            active_detection_->display_label = best_detection->display_label;
            active_detection_->confidence_score = best_detection->confidence_score;
            active_detection_->bbox_frame = best_detection->bbox_frame;
            active_detection_->bbox_roi = best_detection->bbox_roi;
            active_detection_->last_seen_at_ms = timestamp_ms;
            active_detection_->consecutive_frames =
                std::max(active_detection_->consecutive_frames, config_.min_consecutive_frames);
            active_detection_->required_frames = config_.min_consecutive_frames;
            active_missed_frames_ = 0;
        } else {
            ++active_missed_frames_;
            const int missed_threshold = std::max(1, config_.max_missed_frames);
            if (active_missed_frames_ >= missed_threshold) {
                active_detection_.reset();
                active_missed_frames_ = 0;
            }
        }
    }

    if (!best_detection) {
        candidate_.reset();
        return buildSnapshot(frame);
    }

    if (active_detection_ && best_detection->sign_id == active_detection_->sign_id) {
        candidate_.reset();
        return buildSnapshot(frame);
    }

    if (!candidate_ || candidate_->sign_id != best_detection->sign_id) {
        candidate_ = toStabilized(*best_detection, 1, timestamp_ms);
    } else {
        candidate_->model_label = best_detection->model_label;
        candidate_->display_label = best_detection->display_label;
        candidate_->confidence_score = best_detection->confidence_score;
        candidate_->bbox_frame = best_detection->bbox_frame;
        candidate_->bbox_roi = best_detection->bbox_roi;
        candidate_->consecutive_frames += 1;
        candidate_->required_frames = config_.min_consecutive_frames;
        candidate_->last_seen_at_ms = timestamp_ms;
    }

    if (candidate_ &&
        candidate_->consecutive_frames >= config_.min_consecutive_frames) {
        candidate_->confirmed_at_ms = timestamp_ms;
        candidate_->required_frames = config_.min_consecutive_frames;
        active_detection_ = *candidate_;
        candidate_.reset();
        active_missed_frames_ = 0;
    }

    return buildSnapshot(frame);
}

TrafficSignFrameResult
TrafficSignTemporalFilter::buildSnapshot(const TrafficSignDetectionFrame &frame) const {
    TrafficSignFrameResult result;
    result.enabled = frame.enabled;
    result.roi = frame.roi;
    result.raw_detections = frame.raw_detections;
    result.candidate = candidate_;
    result.active_detection = active_detection_;
    result.last_error = frame.last_error;

    if (!frame.enabled) {
        result.detector_state = TrafficSignDetectorState::Disabled;
        return result;
    }

    if (result.active_detection.has_value()) {
        result.detector_state = TrafficSignDetectorState::Confirmed;
        return result;
    }

    if (result.candidate.has_value()) {
        result.detector_state = TrafficSignDetectorState::Candidate;
        return result;
    }

    if (!result.last_error.empty()) {
        result.detector_state = TrafficSignDetectorState::Error;
        return result;
    }

    result.detector_state = TrafficSignDetectorState::Idle;
    return result;
}

StabilizedTrafficSignDetection TrafficSignTemporalFilter::toStabilized(
    const TrafficSignDetection &detection, int consecutive_frames, std::int64_t timestamp_ms) const {
    StabilizedTrafficSignDetection stabilized;
    stabilized.sign_id = detection.sign_id;
    stabilized.model_label = detection.model_label;
    stabilized.display_label = detection.display_label;
    stabilized.confidence_score = detection.confidence_score;
    stabilized.bbox_frame = detection.bbox_frame;
    stabilized.bbox_roi = detection.bbox_roi;
    stabilized.consecutive_frames = consecutive_frames;
    stabilized.required_frames = config_.min_consecutive_frames;
    stabilized.confirmed_at_ms = 0;
    stabilized.last_seen_at_ms = timestamp_ms;
    return stabilized;
}

} // namespace autonomous_car::services::traffic_sign_detection
