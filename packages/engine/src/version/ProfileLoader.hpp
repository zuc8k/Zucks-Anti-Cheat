#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace zucks {

struct FieldSpec {
    int offset = -1;
    int size = 0;
    bool littleEndian = true;
};

struct VersionProfile {
    std::string id;
    std::string displayName;
    std::vector<uint8_t> magic;
    int headerSize = 0;
    FieldSpec totalSize;
    FieldSpec fragTotal;
    FieldSpec fragIndex;
    size_t maxReassembledSize = 17408;
    uint32_t preAuthTimeoutMs = 35000;
    bool fallback = false;
};

class ProfileLoader {
public:
    static std::vector<VersionProfile> LoadDirectory(const std::string& dir);
    static VersionProfile ParseJson(const nlohmann::json& j);
};

} // namespace zucks