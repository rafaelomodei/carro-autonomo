#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <utility>

namespace autonomous_car::services::async {

template <typename T>
class LatestValueMailbox {
public:
    LatestValueMailbox() = default;
    LatestValueMailbox(const LatestValueMailbox &) = delete;
    LatestValueMailbox &operator=(const LatestValueMailbox &) = delete;

    bool offer(T value) {
        std::lock_guard<std::mutex> lock(mutex_);
        const bool replaced = value_.has_value();
        if (replaced) {
            dropped_count_.fetch_add(1, std::memory_order_relaxed);
        }
        value_ = std::move(value);
        condition_.notify_one();
        return replaced;
    }

    std::optional<T> tryPop() {
        std::lock_guard<std::mutex> lock(mutex_);
        return popLocked();
    }

    template <typename StopRequested, typename Rep, typename Period>
    std::optional<T> waitPopFor(const std::chrono::duration<Rep, Period> &timeout,
                                StopRequested stop_requested) {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait_for(lock, timeout,
                            [&] { return value_.has_value() || stop_requested(); });

        if (!value_.has_value()) {
            return std::nullopt;
        }

        return popLocked();
    }

    template <typename StopRequested>
    std::optional<T> waitPop(StopRequested stop_requested) {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [&] { return value_.has_value() || stop_requested(); });

        if (!value_.has_value()) {
            return std::nullopt;
        }

        return popLocked();
    }

    void notifyAll() { condition_.notify_all(); }

    [[nodiscard]] std::uint64_t droppedCount() const noexcept {
        return dropped_count_.load(std::memory_order_relaxed);
    }

private:
    std::optional<T> popLocked() {
        if (!value_.has_value()) {
            return std::nullopt;
        }

        std::optional<T> value = std::move(value_);
        value_.reset();
        return value;
    }

    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::optional<T> value_;
    std::atomic<std::uint64_t> dropped_count_{0};
};

} // namespace autonomous_car::services::async
