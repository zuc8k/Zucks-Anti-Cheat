#include "ProfileLoader.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

namespace zucks {

VersionProfile ProfileLoader::ParseJson(const nlohmann::json& j) {
    VersionProfile p;
    p.id = j.value("id", "");
    p.displayName = j.value("displayName", p.id);
    if (j.contains("magic"))
        for (auto& b : j["magic"]) p.magic.push_back((uint8_t)b.get<int>());
    p.headerSize = j.value("headerSize", 8);
    p.maxReassembledSize = j.value("maxReassembledSize", 17408);
    p.preAuthTimeoutMs = j.value("preAuthTimeoutMs", 35000);
    p.fallback = j.value("fallback", false);

    auto readField = [&](const char* key, FieldSpec& out) {
        if (!j.contains("fields") || !j["fields"].contains(key)) return;
        auto f = j["fields"][key];
        out.offset = f.value("offset", -1);
        out.size = f.value("size", 0);
        out.littleEndian = (f.value("endian", "le") == "le");
    };
    readField("totalSize", p.totalSize);
    readField("fragTotal", p.fragTotal);
    readField("fragIndex", p.fragIndex);
    return p;
}

std::vector<VersionProfile> ProfileLoader::LoadDirectory(const std::string& dir) {
    std::vector<VersionProfile> out;
    if (!fs::exists(dir)) {
        std::cerr << "[Zucks] Profiles directory missing: " << dir << "\n";
        return out;
    }
    for (auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() != ".json") continue;
        std::ifstream f(entry.path());
        if (!f.is_open()) continue;
        try {
            auto j = nlohmann::json::parse(f);
            out.push_back(ParseJson(j));
            std::cout << "[Zucks] Loaded profile: " << out.back().id << "\n";
        } catch (const std::exception& e) {
            std::cerr << "[Zucks] Bad profile " << entry.path() << ": " << e.what() << "\n";
        }
    }
    return out;
}

} // namespace zucks