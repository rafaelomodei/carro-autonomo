#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>

namespace traffic_sign_service::concurrency {

template <typename T>
class LatestFrameStore {
public:
    struct Snapshot {
        std::uint64_t generation{0};
        T value;
    };

    std::uint64_t push(T value) {
        std::lock_guard<std::mutex> lock(mutex_);
        latest_ = Snapshot{++generation_, std::move(value)};
        cv_.notify_all();
        return generation_;
    }

    std::optional<Snapshot> latest() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return latest_;
    }

    std::optional<Snapshot> waitForNext(std::uint64_t last_seen_generation,
                                        const std::atomic<bool> &running) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [&] {
            return generation_ > last_seen_generation || !running.load();
        });

        if (generation_ <= last_seen_generation) {
            return std::nullopt;
        }

        return latest_;
    }

    void notifyAll() { cv_.notify_all(); }

private:
    mutable std::mutex mutex_;
    mutable std::condition_variable cv_;
    std::uint64_t generation_{0};
    std::optional<Snapshot> latest_;
};

} // namespace traffic_sign_service::concurrency
