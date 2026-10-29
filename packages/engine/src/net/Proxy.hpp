#pragma once
#include "RateLimiter.hpp"
#include "../version/AutoDetector.hpp"
#include "../anticheat/Engine.hpp"
#include <atomic>
#include <thread>
#include <functional>
#include <string>
#include <cstdint>

namespace zucks {

class Proxy {
public:
    struct Config {
        std::string name;
        std::string listenHost = "0.0.0.0";
        uint16_t listenPort    = 0;
        std::string backendHost = "127.0.0.1";
        uint16_t backendPort   = 0;
        uint32_t preAuthTimeoutSec = 35;
    };

    // Sink for emitting JSON events (metrics, alerts, version-detected)
    using EventSink = std::function<void(const std::string&)>;

    Proxy(Config cfg,
          AutoDetector* detector,
          RateLimiter* limiter,
          Engine* engine,
          EventSink sink);

    ~Proxy();

    void Start();
    void Stop();

    uint64_t TotalPps() const { return totalPps_.load(); }
    uint64_t TotalBps() const { return totalBps_.load(); }
    uint32_t ActiveConns() const { return activeConns_.load(); }

private:
    void AcceptLoop();
    void HandleClient(int clientFd, const std::string& ip);
    bool ConnectBackend(int& backendFd);

    Config cfg_;
    AutoDetector* detector_;
    RateLimiter* limiter_;
    Engine* engine_;
    EventSink sink_;

    std::atomic<bool> running_{false};
    std::thread acceptThread_;
    int listenFd_ = -1;

    std::atomic<uint64_t> totalPps_{0};
    std::atomic<uint64_t> totalBps_{0};
    std::atomic<uint32_t> activeConns_{0};
};

} // namespace zucks