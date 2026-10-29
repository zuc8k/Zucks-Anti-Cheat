#include "zucks_packet_inspector.h"
#include <cstring>

namespace zucks {

ZucksPacketInspector::ZucksPacketInspector() = default;
ZucksPacketInspector::~ZucksPacketInspector() = default;

int ZucksPacketInspector::Inspect(const uint8_t* data, size_t len,
                                  uint32_t sessionId, std::string& reason) {
    if (len < sizeof(ZucksPacketHeader)) {
        reason = "Packet too small";
        return -1;
    }
    ZucksPacketHeader hdr;
    std::memcpy(&hdr, data, sizeof(hdr));

    if (hdr.magicByte != expectedMagic_) {
        reason = "Invalid magic byte";
        return -1;
    }
    if (hdr.totalSize < sizeof(hdr) || hdr.totalSize > len) {
        reason = "Invalid packet size";
        return -1;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto& st = sessions_[sessionId];

    if (hdr.fragmentTotal > 1) {
        if (hdr.fragmentIndex == 1) {
            st.accumulated = hdr.totalSize;
            st.expectedTotal = hdr.fragmentTotal;
            st.receivedCount = 1;
        } else {
            st.accumulated += hdr.totalSize;
            st.receivedCount++;
        }
        if (st.accumulated > maxReassembled_) {
            reason = "Fragment reassembly overflow";
            sessions_.erase(sessionId);
            return -1;
        }
        if (st.receivedCount >= st.expectedTotal) {
            sessions_.erase(sessionId);
        }
    } else {
        sessions_.erase(sessionId);
    }
    return 0;
}

void ZucksPacketInspector::ResetSession(uint32_t sessionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(sessionId);
}

} // namespace zucks