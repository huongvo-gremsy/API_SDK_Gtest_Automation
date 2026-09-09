#ifndef PAYLOADSDK_TEST_STREAM_TEST_HELPERS_H_
#define PAYLOADSDK_TEST_STREAM_TEST_HELPERS_H_

#include "../common/payload_test_fixture.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <string>

namespace stream_test {

constexpr uint32_t kDefaultStreamId = 0;
constexpr uint32_t kEoStreamId = 1;
constexpr uint32_t kIrStreamId = 2;

struct Snapshot {
    int event = -1;
    std::string uri;
    uint32_t type = 0;
    uint32_t height = 0;
    uint32_t width = 0;
    uint32_t bitrate = 0;
    uint32_t streamId = 0;
};

inline bool getSnapshot(uint32_t requestedId, Snapshot& out,
                        int timeoutMs = 5000) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    uint64_t seq = g_cb.streamSeq.load();

    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->getPayloadCameraStreamingInformation(requestedId);
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now()).count();
        if (remaining <= 0 ||
            !waitForSeq(g_cb.streamSeq, seq,
                        static_cast<int>(remaining > 700 ? 700 : remaining))) {
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(g_cb.m);
            out.event = g_cb.lastStreamEvent.load();
            out.uri = g_cb.lastStreamUri;
            out.type = static_cast<uint32_t>(g_cb.streamValues[0]);
            out.height = static_cast<uint32_t>(g_cb.streamValues[1]);
            out.width = static_cast<uint32_t>(g_cb.streamValues[2]);
            out.bitrate = static_cast<uint32_t>(g_cb.streamValues[3]);
            out.streamId = static_cast<uint32_t>(g_cb.streamValues[4]);
        }
        if (requestedId == kDefaultStreamId || out.streamId == requestedId) {
            return true;
        }
        seq = g_cb.streamSeq.load();
    }
    return false;
}

inline bool isValid(const Snapshot& value) {
    return value.event == PAYLOAD_CAM_STREAMINFO &&
           value.type < VIDEO_STREAM_TYPE_ENUM_END &&
           value.width > 0 && value.height > 0 && value.bitrate > 0 &&
           std::isfinite(static_cast<double>(value.width)) &&
           std::isfinite(static_cast<double>(value.height));
}

inline bool sendUser4AndWaitForAcceptedAck(const std::function<void()>& send,
                                            int timeoutMs = 3000) {
    const uint64_t seq = getCommandAckSeq(MAV_CMD_USER_4);
    send();
    AckInfo ack;
    return waitForCommandAck(MAV_CMD_USER_4, seq, ack, timeoutMs) &&
           (ack.result == MAV_RESULT_ACCEPTED ||
            ack.result == MAV_RESULT_IN_PROGRESS);
}

inline bool sendRateAndWaitForAcceptedAck(const std::function<void()>& send,
                                           int timeoutMs = 3000) {
    const uint64_t seq = getCommandAckSeq(MAV_CMD_SET_MESSAGE_INTERVAL);
    send();
    AckInfo ack;
    return waitForCommandAck(MAV_CMD_SET_MESSAGE_INTERVAL, seq, ack,
                             timeoutMs) &&
           (ack.result == MAV_RESULT_ACCEPTED ||
            ack.result == MAV_RESULT_IN_PROGRESS);
}

}  // namespace stream_test

#endif  // PAYLOADSDK_TEST_STREAM_TEST_HELPERS_H_
