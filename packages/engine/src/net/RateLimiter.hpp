#pragma once
#include <cstdint>
#include <mutex>
#include <atomic>
#include <string>
#include <unordered_map>
#include <chrono>

namespace zucks {

class RateLimiter {
public:
    struct Config {
        uint32_t maxPPS        = 1500;
        uint32_t maxBPS        = 5 * 1024 * 1024;
        uint32_t maxConnsPerIP = 30;
        uint32_t connRateMs    = 250;
        uint32_t globalMax     = 700;
        uint32_t windowMs      = 1000;
    };

    struct Stats {
        uint64_t totalConns = 0;
        uint64_t totalBytes = 0;
        uint64_t totalPackets = 0;
        uint32_t activeConns = 0;
    };

    RateLimiter() = default;
    ~RateLimiter() = default;

    void Configure(const Config& cfg);
    bool AllowConnection(const std::string& ip);
    void ReleaseConnection(const std::string& ip);
    bool RecordTraffic(const std::string& ip, size_t bytes);

    Stats GetStats() const;
    uint32_t ActiveConns() const { return globalConns_.load(std::memory_order_relaxed); }

    void ResetIP(const std::string& ip);

private:
    struct IPState {
        uint64_t lastConnMs      = 0;
        uint32_t activeConns     = 0;
        uint64_t windowStartMs   = 0;
        uint32_t packetsInWindow = 0;
        uint64_t bytesInWindow   = 0;
        uint64_t totalConns      = 0;
        uint64_t totalPackets    = 0;
        uint64_t totalBytes      = 0;
    };

    static uint64_t NowMs();

    mutable std::mutex mutex_;
    std::unordered_map<std::string, IPState> ipStates_;
    Config cfg_;
    std::atomic<uint32_t> globalConns_{0};
    // aggregate stats (across all IPs, monotonic)
    std::atomic<uint64_t> totalConns_{0};
    std::atomic<uint64_t> totalPackets_{0};
    std::atomic<uint64_t> totalBytes_{0};
};

} // namespace zucks