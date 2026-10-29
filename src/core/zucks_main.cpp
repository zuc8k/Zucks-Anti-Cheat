#include "zucks_config.h"
#include "zucks_engine.h"
#include "zucks_rate_limiter.h"
#include "zucks_packet_inspector.h"
#include "zucks_admin_server.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace zucks;

int main(int argc, char** argv) {
    std::string cfgPath = "config/zucks_config.ini";
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--config" && i + 1 < argc) cfgPath = argv[++i];
    }

    if (!ZucksConfig::Instance().Load(cfgPath)) {
        std::cerr << "[Zucks] Failed to load config: " << cfgPath << "\n";
        return 1;
    }
    auto& cfg = ZucksConfig::Instance();
    std::cout << "[Zucks] Config loaded. Admin: " << cfg.Admin().listen
              << ":" << cfg.Admin().port << "\n";

    // Initialize engine
    ZucksEngine::Instance().SetBanThreshold(cfg.AntiCheat().banThreshold);
    ZucksEngine::Instance().SetBanDuration(cfg.AntiCheat().banDurationMin);
    ZucksEngine::Instance().SetAlertCallback([](const std::string& ip,
                                                ZucksViolation v,
                                                const std::string& detail) {
        std::cout << "[Zucks][ALERT] IP=" << ip << " violation=" << (int)v
                  << " detail=" << detail << "\n";
    });

    // Load signatures
    ZucksEngine::Instance().LoadSignatures("config/zucks_signatures.json");

    // Start admin server
    ZucksAdminServer admin;
    if (!admin.Start(cfg.Admin().listen, cfg.Admin().port,
                     cfg.Admin().token, cfg.Admin().webRoot)) {
        std::cerr << "[Zucks] Failed to start admin server\n";
        return 1;
    }
    std::cout << "[Zucks] Admin dashboard: http://" << cfg.Admin().listen
              << ":" << cfg.Admin().port << "\n";

    // TODO: start proxy listeners (see zucks_proxy.cpp)

    // Main loop
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    return 0;
}