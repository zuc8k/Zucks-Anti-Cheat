#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <vector>
#include <mutex>

namespace zucks {

class ZucksAdminServer {
public:
    ZucksAdminServer();
    ~ZucksAdminServer();

    bool Start(const std::string& listenIp, uint16_t port,
               const std::string& token, const std::string& webRoot);

    void Stop();

private:
    void AcceptLoop();
    void HandleClient(int fd);
    std::string Route(const std::string& method, const std::string& path,
                      const std::string& body, const std::string& auth);

    int listenFd_ = -1;
    std::atomic<bool> running_{false};
    std::thread acceptThread_;
    std::string token_;
    std::string webRoot_;
};

} // namespace zucks