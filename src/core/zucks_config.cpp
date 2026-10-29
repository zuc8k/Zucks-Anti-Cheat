#include "zucks_config.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace zucks {

static inline std::string Trim(const std::string& s) {
    auto a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    auto b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

ZucksConfig& ZucksConfig::Instance() {
    static ZucksConfig instance;
    return instance;
}

bool ZucksConfig::Load(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    path_ = path;
    return ParseIni(path);
}

bool ZucksConfig::Reload() {
    return Load(path_);
}

bool ZucksConfig::ParseIni(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    std::string section, line;
    while (std::getline(f, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        if (line.front() == '[' && line.back() == ']') {
            section = line.substr(1, line.size() - 2);
            continue;
        }
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = Trim(line.substr(0, eq));
        std::string val = Trim(line.substr(eq + 1));

        if (section == "ZUCKS_GATEWAY") {
            if (key == "BACKEND_IP") gateway_.backendIp = val;
            else if (key == "LOGIN_LISTEN_PORT") gateway_.loginListenPort = (uint16_t)std::stoi(val);
            else if (key == "LOGIN_BACKEND_PORT") gateway_.loginBackendPort = (uint16_t)std::stoi(val);
            else if (key == "GAME_LISTEN_PORT") gateway_.gameListenPort = (uint16_t)std::stoi(val);
            else if (key == "GAME_BACKEND_PORT") gateway_.gameBackendPort = (uint16_t)std::stoi(val);
        } else if (section == "ZUCKS_ADMIN") {
            if (key == "ADMIN_LISTEN") admin_.listen = val;
            else if (key == "ADMIN_PORT") admin_.port = (uint16_t)std::stoi(val);
            else if (key == "ADMIN_TOKEN") admin_.token = val;
            else if (key == "WEB_ROOT") admin_.webRoot = val;
        } else if (section == "ZUCKS_LIMITS") {
            if (key == "MAX_PPS") limits_.maxPPS = (uint32_t)std::stoul(val);
            else if (key == "MAX_BPS") limits_.maxBPS = (uint32_t)std::stoul(val);
            else if (key == "GLOBAL_MAX_CONNECTIONS") limits_.globalMaxConnections = (uint32_t)std::stoul(val);
            else if (key == "MAX_CONNS_PER_IP") limits_.maxConnsPerIP = (uint32_t)std::stoul(val);
            else if (key == "CONN_RATE_MS") limits_.connRateMs = (uint32_t)std::stoul(val);
            else if (key == "PRE_AUTH_TIMEOUT_MS") limits_.preAuthTimeoutMs = (uint32_t)std::stoul(val);
            else if (key == "MAX_FRAGMENT_REASSEMBLY") limits_.maxFragmentReassembly = std::stoul(val);
        } else if (section == "ZUCKS_ANTICHEAT") {
            if (key == "ENABLE_BEHAVIOR") anticheat_.enableBehavior = (val == "1");
            else if (key == "ENABLE_SIGNATURES") anticheat_.enableSignatures = (val == "1");
            else if (key == "ENABLE_ANOMALY") anticheat_.enableAnomaly = (val == "1");
            else if (key == "ENABLE_INTEGRITY") anticheat_.enableIntegrity = (val == "1");
            else if (key == "BAN_THRESHOLD") anticheat_.banThreshold = std::stoi(val);
            else if (key == "BAN_DURATION_MIN") anticheat_.banDurationMin = std::stoi(val);
        } else if (section == "ZUCKS_ALERTS") {
            if (key == "DISCORD_WEBHOOK") alerts_.discordWebhook = val;
            else if (key == "TELEGRAM_BOT_TOKEN") alerts_.telegramBotToken = val;
            else if (key == "TELEGRAM_CHAT_ID") alerts_.telegramChatId = val;
        } else if (section == "ZUCKS_LOGGING") {
            if (key == "DB_PATH") logging_.dbPath = val;
            else if (key == "LOG_LEVEL") logging_.level = val;
        }
    }
    return true;
}

} // namespace zucks