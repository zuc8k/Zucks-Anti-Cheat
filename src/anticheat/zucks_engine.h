#pragma once
#include <string>
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <functional>
#include <vector>

namespace zucks {

enum class ZucksViolation {
    None = 0,
    InvalidPacket,
    FragmentOverflow,
    RateLimitExceeded,
    SignatureMatch,
    BehaviorAnomaly,
    IntegrityFail,
    ConnectionFlood
};

struct ZucksAction {
    enum Type { Warn, Kick, Ban } type = Warn;
    std::string reason;
    int durationMin = 0;
};

class ZucksEngine {
public:
    using AlertCallback = std::function<void(const std::string& ip, ZucksViolation, const std::string&)>;

    static ZucksEngine& Instance();

    void SetAlertCallback(AlertCallback cb) { alertCb_ = std::move(cb); }
    void SetBanThreshold(int t) { banThreshold_ = t; }
    void SetBanDuration(int min) { banDurationMin_ = min; }

    // Register a violation; returns the action to take
    ZucksAction Report(const std::string& ip, ZucksViolation v,
                       const std::string& detail);

    // Signature management
    void LoadSignatures(const std::string& jsonPath);
    bool MatchSignature(const std::string& ip, const uint8_t* data, size_t len);

    // Behavior tracking
    void TrackAction(const std::string& ip, const std::string& action,
                     uint64_t timestampMs);
    bool IsBehaviorAnomalous(const std::string& ip);

    // Integrity check (HMAC of client fingerprint)
    bool VerifyIntegrity(const std::string& ip, const std::string& hmac,
                         const std::string& expected);

    bool IsBanned(const std::string& ip);
    void Unban(const std::string& ip);
    std::vector<std::string> GetBanned();

private:
    ZucksEngine() = default;
    struct PlayerState {
        int score = 0;
        uint64_t lastViolationMs = 0;
        uint64_t banUntilMs = 0;
        uint32_t actionCount = 0;
        uint64_t firstActionMs = 0;
    };
    std::mutex mutex_;
    std::unordered_map<std::string, PlayerState> players_;
    AlertCallback alertCb_;
    int banThreshold_ = 100;
    int banDurationMin_ = 60;

    struct Signature {
        std::string name;
        std::vector<uint8_t> pattern;
        std::vector<uint8_t> mask;  // 1 = match, 0 = wildcard
    };
    std::vector<Signature> signatures_;
};

} // namespace zucks