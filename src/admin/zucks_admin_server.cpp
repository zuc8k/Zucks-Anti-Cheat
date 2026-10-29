#include "zucks_admin_server.h"
#include "zucks_engine.h"
#include "zucks_rate_limiter.h"
#include "zucks_config.h"
#include <cstring>
#include <sstream>
#include <fstream>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  using socklen_t = int;
  #define ZUCKS_CLOSE closesocket
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #define ZUCKS_CLOSE close
#endif

namespace zucks {

ZucksAdminServer::ZucksAdminServer() = default;
ZucksAdminServer::~ZucksAdminServer() { Stop(); }

bool ZucksAdminServer::Start(const std::string& listenIp, uint16_t port,
                              const std::string& token, const std::string& webRoot) {
    token_ = token;
    webRoot_ = webRoot;
    listenFd_ = (int)socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) return false;

    int opt = 1;
    setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, listenIp.c_str(), &addr.sin_addr);

    if (bind(listenFd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        ZUCKS_CLOSE(listenFd_);
        return false;
    }
    if (listen(listenFd_, 32) < 0) {
        ZUCKS_CLOSE(listenFd_);
        return false;
    }
    running_ = true;
    acceptThread_ = std::thread(&ZucksAdminServer::AcceptLoop, this);
    return true;
}

void ZucksAdminServer::Stop() {
    running_ = false;
    if (listenFd_ >= 0) { ZUCKS_CLOSE(listenFd_); listenFd_ = -1; }
    if (acceptThread_.joinable()) acceptThread_.join();
}

void ZucksAdminServer::AcceptLoop() {
    while (running_) {
        sockaddr_in cli{};
        socklen_t len = sizeof(cli);
        int fd = (int)accept(listenFd_, (sockaddr*)&cli, &len);
        if (fd < 0) continue;
        std::thread(&ZucksAdminServer::HandleClient, this, fd).detach();
    }
}

std::string ZucksAdminServer::Route(const std::string& method,
                                     const std::string& path,
                                     const std::string& body,
                                     const std::string& auth) {
    // Auth check (Bearer)
    if (!token_.empty() && auth != ("Bearer " + token_) && path.rfind("/api/", 0) == 0) {
        return "HTTP/1.1 401 Unauthorized\r\nContent-Type: application/json\r\n\r\n"
               "{\"error\":\"unauthorized\"}";
    }

    if (path == "/api/status" && method == "GET") {
        auto& eng = ZucksEngine::Instance();
        auto banned = eng.GetBanned();
        std::ostringstream os;
        os << "{\"banned_count\":" << banned.size()
           << ",\"version\":\"1.0.0\"}";
        return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n" + os.str();
    }
    if (path == "/api/bans" && method == "GET") {
        auto banned = ZucksEngine::Instance().GetBanned();
        std::ostringstream os; os << "[";
        for (size_t i = 0; i < banned.size(); ++i) {
            if (i) os << ",";
            os << "\"" << banned[i] << "\"";
        }
        os << "]";
        return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n" + os.str();
    }
    if (path == "/api/unban" && method == "POST") {
        // body: {"ip":"x.x.x.x"}
        auto p = body.find("\"ip\"");
        if (p != std::string::npos) {
            auto q1 = body.find('"', p + 4);
            auto q2 = body.find('"', q1 + 1);
            if (q1 != std::string::npos && q2 != std::string::npos) {
                std::string ip = body.substr(q1 + 1, q2 - q1 - 1);
                ZucksEngine::Instance().Unban(ip);
                return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"ok\":true}";
            }
        }
        return "HTTP/1.1 400 Bad Request\r\nContent-Type: application/json\r\n\r\n{\"error\":\"bad request\"}";
    }
    if (path == "/" || path == "/index.html") {
        std::ifstream f(webRoot_ + "/zucks_dashboard.html");
        if (!f.is_open()) return "HTTP/1.1 404 Not Found\r\n\r\n";
        std::stringstream ss; ss << f.rdbuf();
        return "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + ss.str();
    }
    return "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\n\r\n{\"error\":\"not found\"}";
}

void ZucksAdminServer::HandleClient(int fd) {
    char buf[8192];
    int n = (int)recv(fd, buf, sizeof(buf) - 1, 0);
    if (n <= 0) { ZUCKS_CLOSE(fd); return; }
    buf[n] = '\0';
    std::string req(buf);

    auto lineEnd = req.find("\r\n");
    std::string startLine = (lineEnd == std::string::npos) ? req : req.substr(0, lineEnd);
    std::string method, path;
    std::istringstream ss(startLine);
    ss >> method >> path;

    std::string auth;
    auto a = req.find("Authorization: ");
    if (a != std::string::npos) {
        auto e = req.find("\r\n", a);
        auth = req.substr(a + 15, e - a - 15);
    }

    std::string body;
    auto b = req.find("\r\n\r\n");
    if (b != std::string::npos) body = req.substr(b + 4);

    std::string resp = Route(method, path, body, auth);
    send(fd, resp.c_str(), (int)resp.size(), 0);
    ZUCKS_CLOSE(fd);
}

} // namespace zucks