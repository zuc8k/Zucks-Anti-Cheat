#pragma once
#include "ProfileLoader.hpp"
#include <optional>
#include <vector>

namespace zucks {

class AutoDetector {
public:
    void SetProfiles(std::vector<VersionProfile> profiles);

    // Given the first N bytes of a new connection, return best profile.
    std::optional<VersionProfile> Detect(const uint8_t* data, size_t len) const;

    const std::vector<VersionProfile>& Profiles() const { return profiles_; }

private:
    bool MatchesMagic(const VersionProfile& p, const uint8_t* d, size_t len) const;
    std::vector<VersionProfile> profiles_;
};

} // namespace zucks