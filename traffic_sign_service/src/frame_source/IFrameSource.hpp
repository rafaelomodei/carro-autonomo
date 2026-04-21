#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include <opencv2/core.hpp>

namespace traffic_sign_service::frame_source {

struct FramePacket {
    cv::Mat frame;
    std::uint64_t timestamp_ms{0};
};

class IFrameSource {
public:
    using FrameHandler = std::function<void(FramePacket)>;
    using EndHandler = std::function<void()>;

    virtual ~IFrameSource() = default;

    virtual void setFrameHandler(FrameHandler handler) = 0;
    virtual void setEndHandler(EndHandler handler) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool isFinite() const noexcept = 0;
    virtual std::string description() const = 0;
};

} // namespace traffic_sign_service::frame_source
