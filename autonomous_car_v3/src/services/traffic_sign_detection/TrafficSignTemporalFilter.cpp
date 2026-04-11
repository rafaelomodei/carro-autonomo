#include "services/traffic_sign_detection/TrafficSignTemporalFilter.hpp"

#include <algorithm>

namespace autonomous_car::services::traffic_sign_detection {

TrafficSignTemporalFilter::TrafficSignTemporalFilter(TrafficSignConfig config)
    : config_(std::move(config)) {}

TrafficSignFrameResult TrafficSignTemporalFilter::update(TrafficSignFrameResult frame_result) {
    if (frame_result.detector_state == TrafficSignDetectorState::Disabled ||
        frame_result.detector_state == TrafficSignDetectorState::Error) {
        clearTrackingState();
        return frame_result;
    }

    if (frame_result.raw_detections.empty()) {
        ++missed_frames_;

        if (active_detection_ && missed_frames_ <= config_.max_missed_frames) {
            frame_result.detector_state = TrafficSignDetectorState::Confirmed;
            frame_result.active_detection = active_detection_;
            annotateTrackedDetection(frame_result, *active_detection_);
            return frame_result;
        }

        if (candidate_detection_ && missed_frames_ <= config_.max_missed_frames) {
            frame_result.detector_state = TrafficSignDetectorState::Candidate;
            frame_result.candidate = candidate_detection_;
            annotateTrackedDetection(frame_result, *candidate_detection_);
            return frame_result;
        }

        clearTrackingState();
        frame_result.detector_state = TrafficSignDetectorState::Idle;
        return frame_result;
    }

    TrafficSignDetection best_detection = frame_result.raw_detections.front();
    best_detection.required_frames = config_.min_consecutive_frames;
    best_detection.last_seen_at_ms = frame_result.timestamp_ms;
    missed_frames_ = 0;

    if (active_detection_ && isSameTrackedSign(*active_detection_, best_detection)) {
        best_detection.consecutive_frames =
            std::max(config_.min_consecutive_frames,
                     active_detection_->consecutive_frames + 1);
        best_detection.confirmed_at_ms =
            active_detection_->confirmed_at_ms > 0 ? active_detection_->confirmed_at_ms
                                                   : frame_result.timestamp_ms;
        active_detection_ = best_detection;
        candidate_detection_.reset();
        frame_result.detector_state = TrafficSignDetectorState::Confirmed;
        frame_result.active_detection = active_detection_;
        annotateTrackedDetection(frame_result, *active_detection_);
        return frame_result;
    }

    if (candidate_detection_ && isSameTrackedSign(*candidate_detection_, best_detection)) {
        best_detection.consecutive_frames = candidate_detection_->consecutive_frames + 1;
        if (best_detection.consecutive_frames >= config_.min_consecutive_frames) {
            best_detection.confirmed_at_ms = frame_result.timestamp_ms;
            active_detection_ = best_detection;
            candidate_detection_.reset();
            frame_result.detector_state = TrafficSignDetectorState::Confirmed;
            frame_result.active_detection = active_detection_;
            annotateTrackedDetection(frame_result, *active_detection_);
            return frame_result;
        }

        candidate_detection_ = best_detection;
        frame_result.detector_state = TrafficSignDetectorState::Candidate;
        frame_result.candidate = candidate_detection_;
        annotateTrackedDetection(frame_result, *candidate_detection_);
        return frame_result;
    }

    clearTrackingState();

    if (config_.min_consecutive_frames <= 1) {
        best_detection.consecutive_frames = config_.min_consecutive_frames;
        best_detection.confirmed_at_ms = frame_result.timestamp_ms;
        active_detection_ = best_detection;
        frame_result.detector_state = TrafficSignDetectorState::Confirmed;
        frame_result.active_detection = active_detection_;
        annotateTrackedDetection(frame_result, *active_detection_);
        return frame_result;
    }

    best_detection.consecutive_frames = 1;
    candidate_detection_ = best_detection;
    frame_result.detector_state = TrafficSignDetectorState::Candidate;
    frame_result.candidate = candidate_detection_;
    annotateTrackedDetection(frame_result, *candidate_detection_);
    return frame_result;
}

void TrafficSignTemporalFilter::reset() { clearTrackingState(); }

void TrafficSignTemporalFilter::clearTrackingState() {
    missed_frames_ = 0;
    candidate_detection_.reset();
    active_detection_.reset();
}

void TrafficSignTemporalFilter::annotateTrackedDetection(
    TrafficSignFrameResult &frame_result, const TrafficSignDetection &tracked_detection) const {
    for (auto &detection : frame_result.raw_detections) {
        if (!isSameTrackedSign(detection, tracked_detection)) {
            continue;
        }

        detection.consecutive_frames = tracked_detection.consecutive_frames;
        detection.required_frames = tracked_detection.required_frames;
        detection.confirmed_at_ms = tracked_detection.confirmed_at_ms;
        detection.last_seen_at_ms = tracked_detection.last_seen_at_ms;
        break;
    }
}

bool TrafficSignTemporalFilter::isSameTrackedSign(const TrafficSignDetection &lhs,
                                                  const TrafficSignDetection &rhs) {
    return lhs.sign_id == rhs.sign_id && lhs.model_label == rhs.model_label;
}

} // namespace autonomous_car::services::traffic_sign_detection
