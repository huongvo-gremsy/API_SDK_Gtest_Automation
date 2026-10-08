#ifndef PAYLOADSDK_TEST_TRACKING_TEST_HELPERS_H_
#define PAYLOADSDK_TEST_TRACKING_TEST_HELPERS_H_

#include "../common/payload_test_fixture.h"
#include "../camera/parameters/camera_param_test_helpers.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <thread>

namespace tracking_test {

constexpr float kTrackStop = 0.0f;
constexpr float kTrackActive = 1.0f;
constexpr float kTrackEagleEyes = 2.0f;

constexpr int kTrackIdle = 0;
constexpr int kTrackTracked = 1;
constexpr int kTrackLost = 2;

constexpr double kFrameWidth = 1920.0;
constexpr double kFrameHeight = 1080.0;

struct Snapshot {
    double x = 0;
    double y = 0;
    double width = 0;
    double height = 0;
    int status = -1;
};

inline void configureTelemetryRates(uint16_t intervalMs) {
    g_payload->setParamRate(PARAM_TRACK_POS_X, intervalMs);
    g_payload->setParamRate(PARAM_TRACK_POS_Y, intervalMs);
    g_payload->setParamRate(PARAM_TRACK_POS_W, intervalMs);
    g_payload->setParamRate(PARAM_TRACK_POS_H, intervalMs);
    g_payload->setParamRate(PARAM_TRACK_STATUS, intervalMs);
}

inline bool getSnapshot(Snapshot& out, int timeoutMs = 4000) {
    const uint16_t indexes[] = {PARAM_TRACK_POS_X, PARAM_TRACK_POS_Y,
                                PARAM_TRACK_POS_W, PARAM_TRACK_POS_H,
                                PARAM_TRACK_STATUS};
    uint64_t initial[5] = {0};
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        for (int i = 0; i < 5; ++i) {
            initial[i] = g_cb.payloadParamSeqByIndex[indexes[i]];
        }
    }

    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        bool complete = true;
        {
            std::lock_guard<std::mutex> lock(g_cb.m);
            for (int i = 0; i < 5; ++i) {
                if (g_cb.payloadParamSeqByIndex[indexes[i]] <= initial[i]) {
                    complete = false;
                }
            }
            if (complete) {
                out.x = g_cb.payloadParamValueByIndex[PARAM_TRACK_POS_X];
                out.y = g_cb.payloadParamValueByIndex[PARAM_TRACK_POS_Y];
                out.width = g_cb.payloadParamValueByIndex[PARAM_TRACK_POS_W];
                out.height = g_cb.payloadParamValueByIndex[PARAM_TRACK_POS_H];
                out.status = static_cast<int>(
                    g_cb.payloadParamValueByIndex[PARAM_TRACK_STATUS]) & 0xff;
                return true;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}

inline bool waitForStatus(int expected, int timeoutMs = 5000) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        Snapshot current;
        if (getSnapshot(current, 700) && current.status == expected) return true;
    }
    return false;
}

inline bool waitForKnownStatus(int timeoutMs = 4000) {
    Snapshot current;
    return getSnapshot(current, timeoutMs) && current.status >= kTrackIdle &&
           current.status <= kTrackLost;
}

inline bool sendUser4(const std::function<void()>& send, AckInfo& out,
                      int timeoutMs = 2500) {
    const uint64_t seq = getCommandAckSeq(MAV_CMD_USER_4);
    send();
    return waitForCommandAck(MAV_CMD_USER_4, seq, out, timeoutMs);
}

inline bool acceptedOrInProgress(const AckInfo& ack) {
    return ack.result == MAV_RESULT_ACCEPTED ||
           ack.result == MAV_RESULT_IN_PROGRESS;
}

inline bool waitForRoiNear(double x, double y, double width, double height,
                           double tolerance = 96.0, int timeoutMs = 6000) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->setPayloadObjectTrackingPosition(
            static_cast<float>(x), static_cast<float>(y),
            static_cast<float>(width), static_cast<float>(height));
        Snapshot current;
        if (getSnapshot(current, 700) &&
            std::fabs(current.x - x) <= tolerance &&
            std::fabs(current.y - y) <= tolerance &&
            std::fabs(current.width - width) <= tolerance &&
            std::fabs(current.height - height) <= tolerance) {
            return true;
        }
    }
    return false;
}

inline bool setCameraTrackingAlgorithm(uint32_t mode) {
    char id[CAM_PARAM_ID_LEN] = {0};
    std::strncpy(id, PAYLOAD_CAMERA_TRACKING_MODE, sizeof(id) - 1);
    return setAndVerifyCameraParam(id, mode, PARAM_TYPE_UINT32, mode, 5000, 500);
}

class TrackingTestBase : public PayloadTest {
protected:
    void SetUp() override {
        if (!getCameraSettingByID(PAYLOAD_CAMERA_TRACKING_MODE,
                                  originalAlgorithm_, 3000)) {
            GTEST_SKIP() << "Payload does not expose TRACK_MODE.";
        }
        haveOriginalAlgorithm_ = true;
        ASSERT_TRUE(setCameraTrackingAlgorithm(
            PAYLOAD_CAMERA_TRACKING_OBJ_TRACKING));
        configureTelemetryRates(100);
        g_payload->setPayloadObjectTrackingMode(kTrackStop);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    void TearDown() override {
        g_payload->setPayloadObjectTrackingMode(kTrackStop);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        configureTelemetryRates(0);
        if (haveOriginalAlgorithm_) {
            EXPECT_TRUE(setCameraTrackingAlgorithm(
                static_cast<uint32_t>(originalAlgorithm_)))
                << "Could not restore TRACK_MODE.";
        }
    }

    double originalAlgorithm_ = 0;
    bool haveOriginalAlgorithm_ = false;
};

}  // namespace tracking_test

#endif
