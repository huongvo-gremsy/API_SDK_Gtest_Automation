#ifndef PAYLOADSDK_TEST_PAYLOAD_EXAMPLE_TEST_HELPERS_H_
#define PAYLOADSDK_TEST_PAYLOAD_EXAMPLE_TEST_HELPERS_H_

#include "../common/payload_test_fixture.h"
#include "../camera/parameters/camera_param_test_helpers.h"
#include "../camera/query/camera_query_test_helpers.h"
#include "../stream/stream_test_helpers.h"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <string>
#include <thread>

namespace payload_example_test {

inline bool setUint32Param(const char* id, uint32_t value,
                           int timeoutMs = 5000) {
    char mutableId[CAM_PARAM_ID_LEN] = {0};
    std::strncpy(mutableId, id, sizeof(mutableId) - 1);
    return setAndVerifyCameraParam(mutableId, value, PARAM_TYPE_UINT32,
                                   value, timeoutMs, 500);
}

inline bool acceptedOrInProgress(const AckInfo& ack) {
    return ack.result == MAV_RESULT_ACCEPTED ||
           ack.result == MAV_RESULT_IN_PROGRESS;
}

inline bool waitForPayloadParamSince(uint16_t index, uint64_t seq,
                                     double& output, int timeoutMs = 4000) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        {
            std::lock_guard<std::mutex> lock(g_cb.m);
            if (g_cb.payloadParamSeqByIndex[index] > seq) {
                output = g_cb.payloadParamValueByIndex[index];
                return true;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}

inline uint64_t payloadParamSeq(uint16_t index) {
    std::lock_guard<std::mutex> lock(g_cb.m);
    return g_cb.payloadParamSeqByIndex[index];
}

inline bool waitForRecordText(uint64_t seq, std::string& output,
                              int timeoutMs = 8000) {
    if (!waitForSeq(g_cb.recordInfoSeq, seq, timeoutMs)) return false;
    std::lock_guard<std::mutex> lock(g_cb.m);
    if (g_cb.lastRecordEvent.load() != PAYLOAD_RECORD_STATUS) return false;
    output = g_cb.lastRecordText;
    return !output.empty();
}

}  // namespace payload_example_test

#endif
