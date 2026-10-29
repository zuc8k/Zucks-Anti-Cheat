#include "AutoDetector.hpp"

namespace zucks {

void AutoDetector::SetProfiles(std::vector<VersionProfile> p) {
    profiles_ = std::move(p);
}

bool AutoDetector::MatchesMagic(const VersionProfile& p,
                                const uint8_t* d, size_t len) const {
    if (p.magic.empty()) return false;
    if (len < p.magic.size()) return false;
    for (size_t i = 0; i < p.magic.size(); ++i)
        if (d[i] != p.magic[i]) return false;
    return true;
}

std::optional<VersionProfile> AutoDetector::Detect(const uint8_t* d, size_t len) const {
    // 1. Try magic-based profiles first (most specific)
    for (const auto& p : profiles_) {
        if (!p.fallback && MatchesMagic(p, d, len))
            return p;
    }
    // 2. Fallback profile
    for (const auto& p : profiles_)
        if (p.fallback) return p;
    return std::nullopt;
}

} // namespace zucks