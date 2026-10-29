#pragma once
#include <cstdint>
#include <chrono>
#include <mutex>
#include <atomic>
#include <string>
#include <unordered_map>

namespace zucks {

class ZucksRateLimiter {
public:
    ZucksRateLimiter();
    ~ZucksRateLimiter();

    void Configure(uint32_t maxPPS, uint32_t maxBPS,
                   uint32_t maxConnsPerIP, uint32_t connRateMs,
                   uint32_t globalMaxConns);

    bool AllowConnection(const std::string& ip);
    void ReleaseConnection(const std::string& ip);
    bool RecordTraffic(const std::string& ip, size_t bytes);

    uint32_t ActiveConnections() const { return globalConns_.load(); }

private:
    struct IPState {
        uint64_t lastConnMs = 0;
        uint32_t activeConns = 0;
        uint64_t windowStartMs = 0;
        uint32_t packetsInWindow = 0;
        uint64_t bytesInWindow = 0;
    };
    static uint64_t NowMs();

    std::mutex mutex_;
    std::unordered_map<std::string, IPState> ipStates_;
    uint32_t maxPPS_ = 1500;
    uint32_t maxBPS_ = 5 * 1024 * 1024;
    uint32_t maxConnsPerIP_ = 30;
    uint32_t connRateMs_ = 250;
    uint32_t globalMaxConns_ = 700;
    std::atomic<uint32_t> globalConns_{0};
};

} // namespace zucks