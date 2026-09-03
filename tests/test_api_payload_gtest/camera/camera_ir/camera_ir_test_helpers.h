#ifndef PAYLOADSDK_TEST_CAMERA_IR_TEST_HELPERS_H_
#define PAYLOADSDK_TEST_CAMERA_IR_TEST_HELPERS_H_

#include "../parameters/camera_param_test_helpers.h"
#include "../query/camera_query_test_helpers.h"
#include "../../stream/stream_test_helpers.h"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <thread>

namespace camera_ir_test {

inline bool setUint32Param(const char* id, uint32_t value,
                           int timeoutMs = 5000) {
    char mutableId[CAM_PARAM_ID_LEN] = {0};
    std::strncpy(mutableId, id, sizeof(mutableId) - 1);
    return setAndVerifyCameraParam(mutableId, value, PARAM_TYPE_UINT32,
                                   value, timeoutMs, 500);
}

inline bool setCameraModeStateOrAck(CAMERA_MODE mode, int timeoutMs = 5000) {
    if (setAndVerifyCameraMode(mode, timeoutMs, 500)) return true;
    const uint64_t seq = getCommandAckSeq(MAV_CMD_SET_CAMERA_MODE);
    g_payload->setPayloadCameraMode(mode);
    AckInfo ack;
    return waitForCommandAck(MAV_CMD_SET_CAMERA_MODE, seq, ack, 3000) &&
           (ack.result == MAV_RESULT_ACCEPTED ||
            ack.result == MAV_RESULT_IN_PROGRESS);
}

struct CaptureStatus {
    double image = 0;
    double video = 0;
    double count = 0;
    double recordingMs = 0;
};

inline bool readCaptureStatus(CaptureStatus& value, int timeoutMs = 1500) {
    return getCaptureStatus(value.image, value.video, value.count,
                            value.recordingMs, timeoutMs);
}

inline bool waitForRecording(bool active, int timeoutMs = 8000,
                             CaptureStatus* output = nullptr) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        CaptureStatus current;
        if (readCaptureStatus(current, 1000) &&
            ((current.video != 0) == active)) {
            if (output) *output = current;
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    return false;
}

inline bool stopRecording(int timeoutMs = 8000) {
    CaptureStatus current;
    if (readCaptureStatus(current, 1000) && current.video == 0) return true;
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->setPayloadCameraRecordVideoStop();
        if (waitForRecording(false, 1200)) return true;
    }
    return false;
}

inline bool irStreamIsResponsive(int timeoutMs = 3000) {
    stream_test::Snapshot info;
    return stream_test::getSnapshot(stream_test::kIrStreamId, info, timeoutMs) &&
           stream_test::isValid(info);
}

inline bool sendUser4Accepted(const std::function<void()>& send,
                              int timeoutMs = 3000) {
    const uint64_t seq = getCommandAckSeq(MAV_CMD_USER_4);
    send();
    AckInfo ack;
    return waitForCommandAck(MAV_CMD_USER_4, seq, ack, timeoutMs) &&
           (ack.result == MAV_RESULT_ACCEPTED ||
            ack.result == MAV_RESULT_IN_PROGRESS);
}

class CameraIrTest : public PayloadTest {
protected:
    void SetUp() override {
        if (!irStreamIsResponsive(2500)) {
            GTEST_SKIP() << "IR stream 2 is not available on this payload.";
        }
        setupStarted_ = true;

        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIEW_SRC,
                                          originalView_, 3000));
        haveView_ = true;
        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_RECORD_SRC,
                                          originalRecord_, 3000));
        haveRecord_ = true;
        if (getCameraMode(originalMode_, 3000)) haveMode_ = true;

        ASSERT_TRUE(stopRecording())
            << "Could not stop recording left active by an earlier test.";
        ASSERT_TRUE(setUint32Param(PAYLOAD_CAMERA_VIEW_SRC,
                                   PAYLOAD_CAMERA_VIEW_IR));
        ASSERT_TRUE(setUint32Param(PAYLOAD_CAMERA_RECORD_SRC,
                                   PAYLOAD_CAMERA_RECORD_IR));
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    void TearDown() override {
        // A skipped test has no IR state to restore. This also avoids turning a
        // clean hardware-capability skip into a teardown failure.
        if (!setupStarted_) return;

        g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_STOP);
        g_payload->setPayloadCameraStopImage();
        if (!stopRecording()) {
            ADD_FAILURE() << "IR cleanup could not stop video recording.";
        }

        if (haveRecord_ &&
            !setUint32Param(PAYLOAD_CAMERA_RECORD_SRC,
                            static_cast<uint32_t>(originalRecord_))) {
            ADD_FAILURE() << "Could not restore original record source.";
        }
        if (haveView_ &&
            !setUint32Param(PAYLOAD_CAMERA_VIEW_SRC,
                            static_cast<uint32_t>(originalView_))) {
            ADD_FAILURE() << "Could not restore original view source.";
        }
        if (haveMode_ &&
            !setCameraModeStateOrAck(static_cast<CAMERA_MODE>(
                static_cast<int>(originalMode_)), 3000)) {
            std::cout << "[WARN] Original camera mode restore was not confirmed."
                      << std::endl;
        }
    }

    double originalView_ = 0;
    double originalRecord_ = 0;
    double originalMode_ = 0;
    bool haveView_ = false;
    bool haveRecord_ = false;
    bool haveMode_ = false;
    bool setupStarted_ = false;
};

}  // namespace camera_ir_test

#endif
