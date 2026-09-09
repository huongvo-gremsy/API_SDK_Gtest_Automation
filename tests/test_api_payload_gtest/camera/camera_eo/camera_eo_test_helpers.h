#ifndef PAYLOADSDK_TEST_CAMERA_EO_TEST_HELPERS_H_
#define PAYLOADSDK_TEST_CAMERA_EO_TEST_HELPERS_H_

#include "../../common/payload_test_fixture.h"

#include <chrono>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <thread>

namespace camera_eo_test {

inline bool getCameraSettingByID(const char* paramId, double& outValue,
                                 int timeoutMs = -1) {
    if (timeoutMs < 0) timeoutMs = g_timeoutMs;
    uint64_t seq = 0;
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        const auto it = g_cb.paramSeqById.find(paramId);
        seq = it == g_cb.paramSeqById.end() ? 0 : it->second;
    }
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->getPayloadCameraSettingByID(const_cast<char*>(paramId));
        const auto retryDeadline = std::min(
            deadline, std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(500));
        while (std::chrono::steady_clock::now() < retryDeadline) {
            std::lock_guard<std::mutex> lock(g_cb.m);
            const auto it = g_cb.paramSeqById.find(paramId);
            if (it != g_cb.paramSeqById.end() && it->second > seq) {
                outValue = g_cb.paramValueById.at(paramId);
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    return false;
}

inline bool setAndVerifyCameraParam(char* paramId, uint32_t value,
                                    uint8_t paramType, double expectedValue,
                                    int timeoutMs = 5000,
                                    int retryIntervalMs = 500) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->setPayloadCameraParam(paramId, value, paramType);
        double current = 0;
        if (camera_eo_test::getCameraSettingByID(paramId, current, retryIntervalMs) &&
            current == expectedValue) return true;
    }
    return false;
}

inline bool getCameraMode(double& outMode, int timeoutMs = -1) {
    const uint64_t seq = g_cb.cameraSettingsSeq.load();
    g_payload->getPayloadCameraMode();
    if (!waitForSeq(g_cb.cameraSettingsSeq, seq, timeoutMs)) return false;
    std::lock_guard<std::mutex> lock(g_cb.m);
    outMode = g_cb.cameraSettings[0];
    return true;
}

inline bool setAndVerifyCameraMode(CAMERA_MODE mode, int timeoutMs = 5000,
                                   int retryIntervalMs = 500) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->setPayloadCameraMode(mode);
        double current = -1;
        if (camera_eo_test::getCameraMode(current, retryIntervalMs) &&
            static_cast<int>(current) == static_cast<int>(mode)) return true;
    }
    return false;
}

inline bool getCaptureStatus(double& image, double& video, double& count,
                             double& recordingMs, int timeoutMs = -1) {
    const uint64_t seq = g_cb.cameraCaptureStatusSeq.load();
    g_payload->getPayloadCaptureStatus();
    if (!waitForSeq(g_cb.cameraCaptureStatusSeq, seq, timeoutMs)) return false;
    std::lock_guard<std::mutex> lock(g_cb.m);
    image = g_cb.cameraCaptureStatus[0];
    video = g_cb.cameraCaptureStatus[1];
    count = g_cb.cameraCaptureStatus[2];
    recordingMs = g_cb.cameraCaptureStatus[3];
    return true;
}

inline bool checkStorageReady(double& availableMb,
                              double minimumMb = 10.0,
                              int timeoutMs = -1) {
    const uint64_t seq = g_cb.cameraStorageInfoSeq.load();
    g_payload->getPayloadStorage();
    if (!waitForSeq(g_cb.cameraStorageInfoSeq, seq, timeoutMs)) {
        availableMb = -1;
        return false;
    }
    std::lock_guard<std::mutex> lock(g_cb.m);
    availableMb = g_cb.cameraStorageInfo[2];
    return availableMb >= minimumMb;
}

inline bool getCameraFov(camera_type_t cameraType, double& cameraId,
                         double& horizontal, double& vertical,
                         int timeoutMs = -1) {
    const uint64_t seq = g_cb.cameraFovSeq.load();
    g_payload->getPayloadCameraFOVStatus(cameraType);
    if (!waitForSeq(g_cb.cameraFovSeq, seq, timeoutMs)) return false;
    std::lock_guard<std::mutex> lock(g_cb.m);
    cameraId = g_cb.cameraFov[0];
    horizontal = g_cb.cameraFov[1];
    vertical = g_cb.cameraFov[2];
    return true;
}

inline bool setUint32Param(const char* id, uint32_t value,
                           int timeoutMs = 5000) {
    char mutableId[CAM_PARAM_ID_LEN] = {0};
    std::strncpy(mutableId, id, sizeof(mutableId) - 1);
    return camera_eo_test::setAndVerifyCameraParam(mutableId, value, PARAM_TYPE_UINT32,
                                   value, timeoutMs, 500);
}

inline bool setCameraModeStateOrAck(CAMERA_MODE mode, int timeoutMs = 5000) {
    if (camera_eo_test::setAndVerifyCameraMode(mode, timeoutMs, 500)) return true;
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

inline bool eoStreamIsResponsive(int timeoutMs = 3000) {
    const uint64_t seq = g_cb.streamSeq.load();
    g_payload->getPayloadCameraStreamingInformation(1);
    return waitForSeq(g_cb.streamSeq, seq, timeoutMs);
}

inline bool commandAcceptedOrEoResponsive(
    uint16_t command, const std::function<void()>& send,
    int ackTimeoutMs = 2500) {
    const uint64_t seq = getCommandAckSeq(command);
    send();
    AckInfo ack;
    const bool accepted = waitForCommandAck(command, seq, ack, ackTimeoutMs) &&
                          (ack.result == MAV_RESULT_ACCEPTED ||
                           ack.result == MAV_RESULT_IN_PROGRESS);
    return accepted || eoStreamIsResponsive();
}

class CameraEoTest : public PayloadTest {
protected:
    void SetUp() override {
        if (!eoStreamIsResponsive(2500)) {
            GTEST_SKIP() << "EO stream 1 is not available on this payload.";
        }
        setupStarted_ = true;

        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIEW_SRC,
                                          originalView_, 3000));
        haveView_ = true;
#ifndef ZIO
        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_RECORD_SRC,
                                          originalRecord_, 3000));
        haveRecord_ = true;
#endif
        if (getCameraMode(originalMode_, 3000)) haveMode_ = true;

        ASSERT_TRUE(stopRecording())
            << "Could not stop recording left active by an earlier test.";
        ASSERT_TRUE(setUint32Param(PAYLOAD_CAMERA_VIEW_SRC,
                                   PAYLOAD_CAMERA_VIEW_EO));
#ifndef ZIO
        ASSERT_TRUE(setUint32Param(PAYLOAD_CAMERA_RECORD_SRC,
                                   PAYLOAD_CAMERA_RECORD_EO));
#endif
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    void TearDown() override {
        if (!setupStarted_) return;
        g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_STOP);
        g_payload->setCameraFocus(FOCUS_TYPE_CONTINUOUS, FOCUS_STOP);
        g_payload->setPayloadCameraStopImage();
        if (!stopRecording()) {
            ADD_FAILURE() << "EO cleanup could not stop video recording.";
        }
#ifndef ZIO
        if (haveRecord_ &&
            !setUint32Param(PAYLOAD_CAMERA_RECORD_SRC,
                            static_cast<uint32_t>(originalRecord_))) {
            ADD_FAILURE() << "Could not restore original record source.";
        }
#endif
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

}  // namespace camera_eo_test

#endif
