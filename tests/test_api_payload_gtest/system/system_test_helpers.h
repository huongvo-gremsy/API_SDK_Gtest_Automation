#ifndef PAYLOADSDK_TEST_SYSTEM_TEST_HELPERS_H_
#define PAYLOADSDK_TEST_SYSTEM_TEST_HELPERS_H_

#include "../common/payload_test_fixture.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <thread>

namespace system_test {

inline bool sendUser4AndGetAck(const std::function<void()>& send, AckInfo& ack,
                               int timeoutMs = 3000) {
    const uint64_t seq = getCommandAckSeq(MAV_CMD_USER_4);
    send();
    return waitForCommandAck(MAV_CMD_USER_4, seq, ack, timeoutMs);
}

inline bool acceptedOrInProgress(const AckInfo& ack) {
    return ack.result == MAV_RESULT_ACCEPTED ||
           ack.result == MAV_RESULT_IN_PROGRESS;
}

inline bool waitForCameraAndHeartbeat(int timeoutMs = 30000) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    uint64_t heartbeatSeq = g_cb.heartbeatSeq.load();
    bool heartbeatRecovered = false;
    bool cameraRecovered = false;

    while (std::chrono::steady_clock::now() < deadline) {
        if (g_cb.heartbeatSeq.load() > heartbeatSeq) heartbeatRecovered = true;
        uint32_t flags = 0;
        const uint64_t seq = g_cb.cameraInfoSeq.load();
        g_payload->getPayloadCameraInformation();
        if (waitForSeq(g_cb.cameraInfoSeq, seq, 800)) cameraRecovered = true;
        if (heartbeatRecovered && cameraRecovered) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    return false;
}

}  // namespace system_test

#endif
