#include "zucks_rate_limiter.h"

namespace zucks {

ZucksRateLimiter::ZucksRateLimiter() = default;
ZucksRateLimiter::~ZucksRateLimiter() = default;

uint64_t ZucksRateLimiter::NowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

void ZucksRateLimiter::Configure(uint32_t pps, uint32_t bps,
                                 uint32_t maxPerIP, uint32_t connMs,
                                 uint32_t globalMax) {
    std::lock_guard<std::mutex> lock(mutex_);
    maxPPS_ = pps;
    maxBPS_ = bps;
    maxConnsPerIP_ = maxPerIP;
    connRateMs_ = connMs;
    globalMaxConns_ = globalMax;
}

bool ZucksRateLimiter::AllowConnection(const std::string& ip) {
    if (globalConns_.load() >= globalMaxConns_) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    auto& st = ipStates_[ip];
    uint64_t now = NowMs();
    if (st.lastConnMs != 0 && (now - st.lastConnMs) < connRateMs_) return false;
    if (st.activeConns >= maxConnsPerIP_) return false;
    st.lastConnMs = now;
    st.activeConns++;
    globalConns_.fetch_add(1);
    return true;
}

void ZucksRateLimiter::ReleaseConnection(const std::string& ip) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = ipStates_.find(ip);
    if (it == ipStates_.end()) return;
    if (it->second.activeConns > 0) {
        it->second.activeConns--;
        globalConns_.fetch_sub(1);
    }
}

bool ZucksRateLimiter::RecordTraffic(const std::string& ip, size_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& st = ipStates_[ip];
    uint64_t now = NowMs();
    if (st.windowStartMs == 0 || (now - st.windowStartMs) >= 1000) {
        st.windowStartMs = now;
        st.packetsInWindow = 0;
        st.bytesInWindow = 0;
    }
    st.packetsInWindow++;
    st.bytesInWindow += bytes;
    if (st.packetsInWindow > maxPPS_) return false;
    if (st.bytesInWindow > maxBPS_) return false;
    return true;
}

} // namespace zucks