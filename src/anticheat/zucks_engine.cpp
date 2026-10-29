#include "zucks_engine.h"
#include <chrono>
#include <fstream>
#include <sstream>

namespace zucks {

static uint64_t NowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

ZucksEngine& ZucksEngine::Instance() {
    static ZucksEngine inst;
    return inst;
}

ZucksAction ZucksEngine::Report(const std::string& ip, ZucksViolation v,
                                const std::string& detail) {
    ZucksAction act;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& st = players_[ip];
        int weight = 1;
        switch (v) {
            case ZucksViolation::InvalidPacket:    weight = 5; break;
            case ZucksViolation::FragmentOverflow: weight = 20; break;
            case ZucksViolation::RateLimitExceeded:weight = 10; break;
            case ZucksViolation::SignatureMatch:   weight = 40; break;
            case ZucksViolation::BehaviorAnomaly:  weight = 15; break;
            case ZucksViolation::IntegrityFail:    weight = 30; break;
            case ZucksViolation::ConnectionFlood:  weight = 15; break;
            default: break;
        }
        st.score += weight;
        st.lastViolationMs = NowMs();

        if (st.score >= banThreshold_) {
            st.banUntilMs = st.lastViolationMs + (uint64_t)banDurationMin_ * 60 * 1000;
            act.type = ZucksAction::Ban;
            act.durationMin = banDurationMin_;
        } else if (st.score >= banThreshold_ / 2) {
            act.type = ZucksAction::Kick;
        } else {
            act.type = ZucksAction::Warn;
        }
        act.reason = detail;
    }
    if (alertCb_) alertCb_(ip, v, detail);
    return act;
}

bool ZucksEngine::IsBanned(const std::string& ip) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = players_.find(ip);
    if (it == players_.end()) return false;
    return it->second.banUntilMs > NowMs();
}

void ZucksEngine::Unban(const std::string& ip) {
    std::lock_guard<std::mutex> lock(mutex_);
    players_.erase(ip);
}

std::vector<std::string> ZucksEngine::GetBanned() {
    std::vector<std::string> out;
    uint64_t now = NowMs();
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [ip, st] : players_) {
        if (st.banUntilMs > now) out.push_back(ip);
    }
    return out;
}

void ZucksEngine::LoadSignatures(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return;
    // Minimal JSON: [{"name":"...","pattern":"hex","mask":"hex"},...]
    std::stringstream ss; ss << f.rdbuf();
    std::string content = ss.str();
    // (Simplified parser — replace with nlohmann/json in production)
    size_t pos = 0;
    while ((pos = content.find("\"name\"", pos)) != std::string::npos) {
        // ...
        pos += 6;
    }
}

bool ZucksEngine::MatchSignature(const std::string& ip,
                                 const uint8_t* data, size_t len) {
    (void)ip;
    for (auto& sig : signatures_) {
        if (sig.pattern.empty() || len < sig.pattern.size()) continue;
        bool match = true;
        for (size_t i = 0; i < sig.pattern.size(); ++i) {
            if (sig.mask[i] && data[i] != sig.pattern[i]) { match = false; break; }
        }
        if (match) return true;
    }
    return false;
}

void ZucksEngine::TrackAction(const std::string& ip, const std::string& action,
                              uint64_t timestampMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& st = players_[ip];
    if (st.firstActionMs == 0) st.firstActionMs = timestampMs;
    st.actionCount++;
    st.lastViolationMs = timestampMs;
    (void)action;
}

bool ZucksEngine::IsBehaviorAnomalous(const std::string& ip) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = players_.find(ip);
    if (it == players_.end()) return false;
    uint64_t now = NowMs();
    uint64_t window = now - it->second.firstActionMs;
    if (window < 1000) return false;
    double rate = (double)it->second.actionCount / (window / 1000.0);
    return rate > 50.0;  // >50 actions/sec = suspicious
}

bool ZucksEngine::VerifyIntegrity(const std::string& ip, const std::string& hmac,
                                  const std::string& expected) {
    (void)ip;
    return hmac == expected;
}

} // namespace zucks