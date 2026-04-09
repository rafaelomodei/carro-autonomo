#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <string_view>

namespace autonomous_car::services::traffic_signals {

enum class TrafficSignalId { Stop, TurnLeft, TurnRight };

struct TrafficSignalEvent {
    TrafficSignalId signal_id{TrafficSignalId::Stop};
    std::uint64_t received_at_ms{0};
};

std::optional<TrafficSignalId> trafficSignalIdFromString(std::string_view value);
std::string_view toString(TrafficSignalId signal_id);

class TrafficSignalRegistry {
public:
    bool recordDetectedSignal(TrafficSignalId signal_id, std::uint64_t received_at_ms);
    bool recordDetectedSignal(std::string_view raw_signal_id, std::uint64_t received_at_ms);
    std::optional<TrafficSignalEvent> lastDetectedSignal() const;

private:
    mutable std::mutex mutex_;
    std::optional<TrafficSignalEvent> last_detected_signal_;
};

} // namespace autonomous_car::services::traffic_signals
