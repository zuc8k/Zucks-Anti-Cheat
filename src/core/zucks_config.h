#pragma once
#include <string>
#include <cstdint>
#include <mutex>
#include <atomic>

namespace zucks {

struct ZucksGatewayConfig {
    std::string backendIp;
    uint16_t loginListenPort = 28888;
    uint16_t loginBackendPort = 18889;
    uint16_t gameListenPort = 22367;
    uint16_t gameBackendPort = 12367;
};

struct ZucksAdminConfig {
    std::string listen = "127.0.0.1";
    uint16_t port = 8080;
    std::string token;
    std::string webRoot = "./web";
};

struct ZucksLimitsConfig {
    uint32_t maxPPS = 1500;
    uint32_t maxBPS = 5 * 1024 * 1024;
    uint32_t globalMaxConnections = 700;
    uint32_t maxConnsPerIP = 30;
    uint32_t connRateMs = 250;
    uint32_t preAuthTimeoutMs = 35000;
    size_t   maxFragmentReassembly = 17408;
};

struct ZucksAntiCheatConfig {
    bool enableBehavior = true;
    bool enableSignatures = true;
    bool enableAnomaly = true;
    bool enableIntegrity = true;
    int  banThreshold = 100;
    int  banDurationMin = 60;
};

struct ZucksAlertsConfig {
    std::string discordWebhook;
    std::string telegramBotToken;
    std::string telegramChatId;
};

struct ZucksLoggingConfig {
    std::string dbPath = "./zucks_events.db";
    std::string level = "info";
};

class ZucksConfig {
public:
    static ZucksConfig& Instance();

    bool Load(const std::string& path);
    bool Reload();  // hot-reload
    const std::string& Path() const { return path_; }

    ZucksGatewayConfig&   Gateway()   { return gateway_; }
    ZucksAdminConfig&     Admin()     { return admin_; }
    ZucksLimitsConfig&    Limits()    { return limits_; }
    ZucksAntiCheatConfig& AntiCheat() { return anticheat_; }
    ZucksAlertsConfig&    Alerts()    { return alerts_; }
    ZucksLoggingConfig&   Logging()   { return logging_; }

private:
    ZucksConfig() = default;
    bool ParseIni(const std::string& path);

    std::string path_;
    std::mutex mutex_;
    ZucksGatewayConfig gateway_;
    ZucksAdminConfig admin_;
    ZucksLimitsConfig limits_;
    ZucksAntiCheatConfig anticheat_;
    ZucksAlertsConfig alerts_;
    ZucksLoggingConfig logging_;
};

} // namespace zucks