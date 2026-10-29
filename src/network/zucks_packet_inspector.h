#pragma once
#include <cstdint>
#include <string>
#include <mutex>
#include <unordered_map>

namespace zucks {

// Generic packet header — adaptable to any protocol
struct ZucksPacketHeader {
    uint8_t  magicByte;
    uint16_t totalSize;
    uint8_t  category;
    uint8_t  subCategory;
    uint8_t  command;
    uint8_t  fragmentTotal;
    uint8_t  fragmentIndex;
} __attribute__((packed));

class ZucksPacketInspector {
public:
    ZucksPacketInspector();
    ~ZucksPacketInspector();

    void SetMaxReassembledSize(size_t bytes) { maxReassembled_ = bytes; }
    void SetMagicByte(uint8_t b) { expectedMagic_ = b; }

    // 0 = OK, -1 = reject
    int Inspect(const uint8_t* data, size_t len,
                uint32_t sessionId, std::string& reason);

    void ResetSession(uint32_t sessionId);

private:
    struct SessionState {
        size_t accumulated = 0;
        uint8_t expectedTotal = 0;
        uint8_t receivedCount = 0;
    };
    size_t maxReassembled_ = 17408;
    uint8_t expectedMagic_ = 0xF1;
    std::mutex mutex_;
    std::unordered_map<uint32_t, SessionState> sessions_;
};

} // namespace zucks