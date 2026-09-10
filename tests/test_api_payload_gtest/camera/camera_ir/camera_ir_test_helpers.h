#ifndef PAYLOADSDK_TEST_CAMERA_IR_TEST_HELPERS_H_
#define PAYLOADSDK_TEST_CAMERA_IR_TEST_HELPERS_H_

#include "../../common/payload_test_fixture.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <thread>

namespace camera_ir_test {

inline bool checkStorageReady(double& outAvailableMb,
                              double minAvailableMb = 10.0,
                              int timeoutMs = 3000) {
    const uint64_t sequence = g_cb.cameraStorageInfoSeq.load();
    g_payload->getPayloadStorage();
    if (!waitForSeq(g_cb.cameraStorageInfoSeq, sequence, timeoutMs)) {
        outAvailableMb = -1;
        return false;
    }
    std::lock_guard<std::mutex> lock(g_cb.m);
    outAvailableMb = g_cb.cameraStorageInfo[2];
    return outAvailableMb >= minAvailableMb;
}

inline bool getCameraSettingByID(const char* id, double& value,
                                 int timeoutMs = 3000) {
    uint64_t sequence = 0;
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        sequence = g_cb.paramSeqById[id];
    }
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->getPayloadCameraSettingByID(const_cast<char*>(id));
        const auto retryDeadline = std::min(
            deadline, std::chrono::steady_clock::now() +
                      std::chrono::milliseconds(500));
        while (std::chrono::steady_clock::now() < retryDeadline) {
            std::lock_guard<std::mutex> lock(g_cb.m);
            if (g_cb.paramSeqById[id] > sequence) {
                value = g_cb.paramValueById.at(id);
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    return false;
}

inline bool setAndVerifyCameraParam(char* id, uint32_t value,
                                    uint8_t type, int timeoutMs = 5000) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->setPayloadCameraParam(id, value, type);
        double actual = 0;
        if (getCameraSettingByID(id, actual, 1000) && actual == value) return true;
    }
    return false;
}

inline bool getCameraMode(double& mode, int timeoutMs = 3000) {
    const uint64_t sequence = g_cb.cameraSettingsSeq.load();
    g_payload->getPayloadCameraMode();
    if (!waitForSeq(g_cb.cameraSettingsSeq, sequence, timeoutMs)) return false;
    std::lock_guard<std::mutex> lock(g_cb.m);
    mode = g_cb.cameraSettings[0];
    return true;
}

inline bool setAndVerifyCameraMode(CAMERA_MODE mode, int timeoutMs = 5000,
                                   int = 500) {
    g_payload->setPayloadCameraMode(mode);
    double actual = -1;
    return getCameraMode(actual, timeoutMs) &&
           static_cast<int>(actual) == static_cast<int>(mode);
}

inline bool getCaptureStatus(double& image, double& video, double& count,
                             double& recordingMs, int timeoutMs = 1500) {
    const uint64_t sequence = g_cb.cameraCaptureStatusSeq.load();
    g_payload->getPayloadCaptureStatus();
    if (!waitForSeq(g_cb.cameraCaptureStatusSeq, sequence, timeoutMs)) return false;
    std::lock_guard<std::mutex> lock(g_cb.m);
    image = g_cb.cameraCaptureStatus[0];
    video = g_cb.cameraCaptureStatus[1];
    count = g_cb.cameraCaptureStatus[2];
    recordingMs = g_cb.cameraCaptureStatus[3];
    return true;
}

inline bool setUint32Param(const char* id, uint32_t value,
                           int timeoutMs = 5000) {
    char mutableId[CAM_PARAM_ID_LEN] = {0};
    std::strncpy(mutableId, id, sizeof(mutableId) - 1);
    return setAndVerifyCameraParam(mutableId, value, PARAM_TYPE_UINT32,
                                   timeoutMs);
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
    const uint64_t sequence = g_cb.streamSeq.load();
    g_payload->getPayloadCameraStreamingInformation(2);
    return waitForSeq(g_cb.streamSeq, sequence, timeoutMs);
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
