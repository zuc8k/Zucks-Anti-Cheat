#include "Proxy.hpp"

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <sstream>

namespace zucks {

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------

static void SetNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return;
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void SetNoDelay(int fd) {
    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&one, sizeof(one));
}

static std::string JsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if ((unsigned char)c < 0x20) {
                    char b[8];
                    snprintf(b, sizeof(b), "\\u%04x", c);
                    out += b;
                } else out += c;
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// Construction / lifecycle
// ---------------------------------------------------------------------------

Proxy::Proxy(Config cfg,
             AutoDetector* detector,
             RateLimiter* limiter,
             Engine* engine,
             EventSink sink)
    : cfg_(std::move(cfg)),
      detector_(detector),
      limiter_(limiter),
      engine_(engine),
      sink_(std::move(sink)) {}

Proxy::~Proxy() { Stop(); }

void Proxy::Start() {
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        std::cerr << "[" << cfg_.name << "] socket() failed: " << strerror(errno) << "\n";
        return;
    }

    int one = 1;
    setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    SetNonBlocking(listenFd_);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(cfg_.listenPort);
    if (inet_pton(AF_INET, cfg_.listenHost.c_str(), &addr.sin_addr) != 1) {
        addr.sin_addr.s_addr = INADDR_ANY;
    }

    if (bind(listenFd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[" << cfg_.name << "] bind :" << cfg_.listenPort
                  << " failed: " << strerror(errno) << "\n";
        close(listenFd_);
        listenFd_ = -1;
        return;
    }
    if (listen(listenFd_, 256) < 0) {
        std::cerr << "[" << cfg_.name << "] listen failed\n";
        close(listenFd_);
        listenFd_ = -1;
        return;
    }

    running_ = true;
    acceptThread_ = std::thread(&Proxy::AcceptLoop, this);
    std::cout << "[" << cfg_.name << "] Listening on "
              << cfg_.listenHost << ":" << cfg_.listenPort
              << " -> " << cfg_.backendHost << ":" << cfg_.backendPort << "\n";
}

void Proxy::Stop() {
    if (!running_.exchange(false)) return;
    if (listenFd_ >= 0) {
        shutdown(listenFd_, SHUT_RDWR);
        close(listenFd_);
        listenFd_ = -1;
    }
    if (acceptThread_.joinable()) acceptThread_.join();
}

// ---------------------------------------------------------------------------
// Accept loop (epoll for scalable accept + O_NONBLOCK guard)
// ---------------------------------------------------------------------------

void Proxy::AcceptLoop() {
    int epfd = epoll_create1(EPOLL_CLOEXEC);
    if (epfd < 0) {
        std::cerr << "[" << cfg_.name << "] epoll_create1 failed\n";
        return;
    }

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = listenFd_;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listenFd_, &ev);

    epoll_event events[64];

    while (running_.load()) {
        int n = epoll_wait(epfd, events, 64, 250);
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }
        for (int i = 0; i < n; ++i) {
            if (events[i].data.fd != listenFd_) continue;

            // Drain all pending connections in this wake-up
            while (true) {
                sockaddr_in cli{};
                socklen_t len = sizeof(cli);
                int fd = accept(listenFd_, (sockaddr*)&cli, &len);
                if (fd < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                    if (errno == EINTR) continue;
                    break;
                }
                SetNoDelay(fd);

                char ipbuf[64] = {0};
                inet_ntop(AF_INET, &cli.sin_addr, ipbuf, sizeof(ipbuf));
                std::string ip = ipbuf;

                // Ban check
                if (engine_ && engine_->IsBanned(ip)) {
                    close(fd);
                    continue;
                }
                // Rate limit
                if (limiter_ && !limiter_->AllowConnection(ip)) {
                    close(fd);
                    if (sink_) {
                        std::ostringstream os;
                        os << R"({"type":"alert","payload":{"ip":")"
                           << JsonEscape(ip)
                           << R"(","reason":"conn-rate-limit","proxy":")"
                           << JsonEscape(cfg_.name) << R"("}})";
                        sink_(os.str());
                    }
                    continue;
                }

                activeConns_.fetch_add(1);
                std::thread([this, fd, ip] {
                    HandleClient(fd, ip);
                    activeConns_.fetch_sub(1);
                    if (limiter_) limiter_->ReleaseConnection(ip);
                }).detach();
            }
        }
    }

    close(epfd);
}

// ---------------------------------------------------------------------------
// Per-connection handler
// ---------------------------------------------------------------------------

bool Proxy::ConnectBackend(int& backendFd) {
    backendFd = socket(AF_INET, SOCK_STREAM, 0);
    if (backendFd < 0) return false;

    sockaddr_in baddr{};
    baddr.sin_family = AF_INET;
    baddr.sin_port = htons(cfg_.backendPort);
    if (inet_pton(AF_INET, cfg_.backendHost.c_str(), &baddr.sin_addr) != 1) {
        close(backendFd);
        return false;
    }
    if (connect(backendFd, (sockaddr*)&baddr, sizeof(baddr)) < 0) {
        close(backendFd);
        return false;
    }
    SetNoDelay(backendFd);
    return true;
}

void Proxy::HandleClient(int clientFd, const std::string& ip) {
    int backendFd = -1;
    if (!ConnectBackend(backendFd)) {
        close(clientFd);
        if (sink_) {
            std::ostringstream os;
            os << R"({"type":"alert","payload":{"ip":")" << JsonEscape(ip)
               << R"(","reason":"backend-unreachable","proxy":")" << JsonEscape(cfg_.name) << R"("}})";
            sink_(os.str());
        }
        return;
    }

    // Pre-auth timeout: small recv timeout while we peek for version detection
    timeval tv{};
    tv.tv_sec = std::min<uint32_t>(cfg_.preAuthTimeoutSec, 5);
    setsockopt(clientFd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Peek first packet for version detection
    uint8_t peek[64];
    ssize_t peekN = recv(clientFd, peek, sizeof(peek), MSG_PEEK);
    if (peekN <= 0) {
        close(clientFd); close(backendFd);
        return;
    }

    auto profileOpt = detector_ ? detector_->Detect(peek, (size_t)peekN) : std::nullopt;
    if (!profileOpt) {
        if (sink_) {
            std::ostringstream os;
            os << R"({"type":"alert","payload":{"ip":")" << JsonEscape(ip)
               << R"(","reason":"unknown-version","proxy":")" << JsonEscape(cfg_.name) << R"("}})";
            sink_(os.str());
        }
        close(clientFd); close(backendFd);
        return;
    }
    VersionProfile profile = *profileOpt;

    if (sink_) {
        std::ostringstream os;
        os << R"({"type":"version-detected","payload":{"ip":")" << JsonEscape(ip)
           << R"(","version":")" << JsonEscape(profile.id)
           << R"(","proxy":")" << JsonEscape(cfg_.name) << R"("}})";
        sink_(os.str());
    }

    // Remove pre-auth timeout for the long-lived stream
    tv.tv_sec = 0; tv.tv_usec = 0;
    setsockopt(clientFd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Bidirectional forward + inline inspection (client -> backend side)
    struct ConnCtx {
        std::atomic<bool> closed{false};
        uint8_t buf[16384];
        size_t reassembled = 0;
        uint8_t expectedFrags = 0;
        uint8_t fragCount = 0;
    };
    ConnCtx ctx;

    auto shutdownBoth = [&]() {
        if (!ctx.closed.exchange(true)) {
            shutdown(clientFd, SHUT_RDWR);
            shutdown(backendFd, SHUT_RDWR);
        }
    };

    // ---- client -> backend (inspected) ----
    std::thread forwarder([&] {
        while (!ctx.closed.load()) {
            ssize_t r = recv(clientFd, ctx.buf, sizeof(ctx.buf), 0);
            if (r <= 0) { shutdownBoth(); break; }

            totalBps_.fetch_add((uint64_t)r, std::memory_order_relaxed);
            totalPps_.fetch_add(1, std::memory_order_relaxed);

            if (limiter_ && !limiter_->RecordTraffic(ip, (size_t)r)) {
                if (engine_) engine_->Report(ip, Violation::RateLimitExceeded, "rate-limit");
                if (sink_) {
                    std::ostringstream os;
                    os << R"({"type":"alert","payload":{"ip":")" << JsonEscape(ip)
                       << R"(","reason":"rate-limit","proxy":")" << JsonEscape(cfg_.name) << R"("}})";
                    sink_(os.str());
                }
                shutdownBoth();
                break;
            }

            // Protocol inspection
            if (r >= profile.headerSize) {
                // magic check
                bool magicOk = true;
                if (!profile.magic.empty()) {
                    if ((size_t)r < profile.magic.size()) magicOk = false;
                    else {
                        for (size_t i = 0; i < profile.magic.size(); ++i) {
                            if (ctx.buf[i] != profile.magic[i]) { magicOk = false; break; }
                        }
                    }
                }
                if (!magicOk) {
                    if (engine_) engine_->Report(ip, Violation::InvalidPacket, "bad-magic");
                    if (sink_) {
                        std::ostringstream os;
                        os << R"({"type":"alert","payload":{"ip":")" << JsonEscape(ip)
                           << R"(","reason":"bad-magic","proxy":")" << JsonEscape(cfg_.name) << R"("}})";
                        sink_(os.str());
                    }
                    shutdownBoth();
                    break;
                }

                // fragment reassembly guard
                if (profile.fragTotal.offset >= 0 && profile.fragTotal.size == 1 &&
                    profile.fragIndex.offset >= 0 && profile.fragIndex.size == 1) {
                    uint8_t ft = ctx.buf[profile.fragTotal.offset];
                    uint8_t fi = ctx.buf[profile.fragIndex.offset];

                    if (ft > 1) {
                        if (fi == 1) {
                            ctx.reassembled = (size_t)r;
                            ctx.expectedFrags = ft;
                            ctx.fragCount = 1;
                        } else {
                            ctx.reassembled += (size_t)r;
                            ctx.fragCount++;
                        }
                        if (ctx.reassembled > profile.maxReassembledSize) {
                            if (engine_) engine_->Report(ip, Violation::FragmentOverflow, "fragment-overflow");
                            if (sink_) {
                                std::ostringstream os;
                                os << R"({"type":"alert","payload":{"ip":")" << JsonEscape(ip)
                                   << R"(","reason":"fragment-overflow","proxy":")"
                                   << JsonEscape(cfg_.name) << R"("}})";
                                sink_(os.str());
                            }
                            shutdownBoth();
                            break;
                        }
                        if (ctx.fragCount >= ctx.expectedFrags) {
                            ctx.reassembled = 0;
                            ctx.expectedFrags = 0;
                            ctx.fragCount = 0;
                        }
                    } else {
                        ctx.reassembled = 0;
                        ctx.expectedFrags = 0;
                        ctx.fragCount = 0;
                    }
                }
            }

            // Forward
            size_t off = 0;
            while (off < (size_t)r && !ctx.closed.load()) {
                ssize_t w = send(backendFd, ctx.buf + off, (size_t)r - off, MSG_NOSIGNAL);
                if (w <= 0) { shutdownBoth(); break; }
                off += (size_t)w;
            }
        }
    });

    // ---- backend -> client (passthrough) ----
    std::thread reverse([&] {
        uint8_t b[16384];
        while (!ctx.closed.load()) {
            ssize_t r = recv(backendFd, b, sizeof(b), 0);
            if (r <= 0) { shutdownBoth(); break; }
            size_t off = 0;
            while (off < (size_t)r && !ctx.closed.load()) {
                ssize_t w = send(clientFd, b + off, (size_t)r - off, MSG_NOSIGNAL);
                if (w <= 0) { shutdownBoth(); break; }
                off += (size_t)w;
            }
        }
    });

    forwarder.join();
    reverse.join();
    close(clientFd);
    close(backendFd);
}

} // namespace zucks