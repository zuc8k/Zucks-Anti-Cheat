#include "RateLimiter.hpp"

namespace zucks {

uint64_t RateLimiter::NowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

void RateLimiter::Configure(const Config& cfg) {
    std::lock_guard<std::mutex> lk(mutex_);
    cfg_ = cfg;
}

bool RateLimiter::AllowConnection(const std::string& ip) {
    // Global cap check first (lock-free fast path)
    if (globalConns_.load(std::memory_order_relaxed) >= cfg_.globalMax) {
        return false;
    }

    std::lock_guard<std::mutex> lk(mutex_);
    auto& st = ipStates_[ip];
    uint64_t now = NowMs();

    // Rate of new connections per IP (e.g. max 1 per 250ms)
    if (st.lastConnMs != 0 && (now - st.lastConnMs) < cfg_.connRateMs) {
        return false;
    }
    // Per-IP concurrent connections
    if (st.activeConns >= cfg_.maxConnsPerIP) {
        return false;
    }
    // Global cap re-check under lock (racy otherwise)
    if (globalConns_.load() >= cfg_.globalMax) {
        return false;
    }

    st.lastConnMs = now;
    st.activeConns++;
    st.totalConns++;
    globalConns_.fetch_add(1, std::memory_order_relaxed);
    totalConns_.fetch_add(1, std::memory_order_relaxed);
    return true;
}

void RateLimiter::ReleaseConnection(const std::string& ip) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = ipStates_.find(ip);
    if (it == ipStates_.end()) return;
    if (it->second.activeConns > 0) {
        it->second.activeConns--;
        globalConns_.fetch_sub(1, std::memory_order_relaxed);
    }
}

bool RateLimiter::RecordTraffic(const std::string& ip, size_t bytes) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto& st = ipStates_[ip];
    uint64_t now = NowMs();

    if (st.windowStartMs == 0 || (now - st.windowStartMs) >= cfg_.windowMs) {
        st.windowStartMs = now;
        st.packetsInWindow = 0;
        st.bytesInWindow = 0;
    }

    st.packetsInWindow++;
    st.bytesInWindow += bytes;
    st.totalPackets++;
    st.totalBytes += bytes;

    totalPackets_.fetch_add(1, std::memory_order_relaxed);
    totalBytes_.fetch_add(bytes, std::memory_order_relaxed);

    if (st.packetsInWindow > cfg_.maxPPS) return false;
    if (st.bytesInWindow  > cfg_.maxBPS) return false;
    return true;
}

RateLimiter::Stats RateLimiter::GetStats() const {
    Stats s;
    s.totalConns   = totalConns_.load();
    s.totalBytes   = totalBytes_.load();
    s.totalPackets = totalPackets_.load();
    s.activeConns  = globalConns_.load();
    return s;
}

void RateLimiter::ResetIP(const std::string& ip) {
    std::lock_guard<std::mutex> lk(mutex_);
    ipStates_.erase(ip);
}

} // namespace zucks