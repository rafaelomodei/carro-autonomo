#pragma once

#include <cctype>
#include <optional>
#include <string>
#include <string_view>

namespace autonomous_car {

enum class DrivingMode { Manual, Autonomous };

enum class CommandSource { Manual, Autonomous };

inline std::string_view toString(DrivingMode mode) {
    switch (mode) {
    case DrivingMode::Manual:
        return "manual";
    case DrivingMode::Autonomous:
        return "autonomous";
    }
    return "unknown";
}

inline std::string_view toString(CommandSource source) {
    switch (source) {
    case CommandSource::Manual:
        return "manual";
    case CommandSource::Autonomous:
        return "autonomous";
    }
    return "unknown";
}

inline std::optional<DrivingMode> drivingModeFromString(const std::string &value) {
    std::string normalized;
    normalized.reserve(value.size());
    for (char ch : value) {
        normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }

    if (normalized == "manual") {
        return DrivingMode::Manual;
    }
    if (normalized == "autonomous" || normalized == "auto") {
        return DrivingMode::Autonomous;
    }
    return std::nullopt;
}

inline std::optional<CommandSource> commandSourceFromString(const std::string &value) {
    auto mode = drivingModeFromString(value);
    if (!mode) {
        return std::nullopt;
    }
    return *mode == DrivingMode::Manual ? CommandSource::Manual : CommandSource::Autonomous;
}

inline bool isSourceCompatible(CommandSource source, DrivingMode mode) {
    if (mode == DrivingMode::Manual) {
        return source == CommandSource::Manual;
    }
    return source == CommandSource::Autonomous;
}

} // namespace autonomous_car

