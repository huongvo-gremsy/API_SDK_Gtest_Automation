#ifndef API_CAMERA_IR_TEST_HELPER_H_
#define API_CAMERA_IR_TEST_HELPER_H_

#include "../common/payload_test_fixture.h"

#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <thread>

namespace camera_ir_test {

struct CaptureStatus {
    double image = 0;
    double video = 0;
    double count = 0;
    double recordingMs = 0;
};

struct FovStatus {
    double cameraId = 0;
    double horizontal = 0;
    double vertical = 0;
};

inline bool readFovStatus(FovStatus& status, int timeoutMs = 3000) {
    const uint64_t seq = g_cb.cameraFovSeq.load();
    g_payload->getPayloadCameraFOVStatus(CAMERA_IR);
    if (!waitForSeq(g_cb.cameraFovSeq, seq, timeoutMs)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_cb.m);
    status.cameraId = g_cb.cameraFov[0];
    status.horizontal = g_cb.cameraFov[1];
    status.vertical = g_cb.cameraFov[2];
    std::cout << "readFovStatus: cameraId=" << status.cameraId
              << ", hfov=" << status.horizontal
              << ", vfov=" << status.vertical << std::endl;
    return true;
}

inline bool readCameraParam(const char* id,
                            double& value,
                            int timeoutMs = 3000)
{
    uint64_t startSeq = 0;

    // Get the current sequence number for this specific parameter ID
    {
        std::lock_guard<std::mutex> lock(g_cb.m);

        auto it = g_cb.paramSeqById.find(id);

        if (it != g_cb.paramSeqById.end()) {
            startSeq = it->second;
        }
    }

    // Copy ID into the buffer expected by SDK
    char mutableId[CAM_PARAM_ID_LEN] = {};
    std::strncpy(mutableId, id, sizeof(mutableId) - 1);

    // Request parameter by ID
    g_payload->getPayloadCameraSettingByID(mutableId);

    // Wait for a NEW response for THIS parameter ID
    const auto deadline =
        std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeoutMs);

    while (std::chrono::steady_clock::now() < deadline)
    {
        {
            std::lock_guard<std::mutex> lock(g_cb.m);

            auto seqIt = g_cb.paramSeqById.find(id);
            auto valueIt = g_cb.paramValueById.find(id);

            if (seqIt != g_cb.paramSeqById.end() &&
                valueIt != g_cb.paramValueById.end() &&
                seqIt->second > startSeq)
            {
                value = valueIt->second;
                return true;
            }
        }

        // IMPORTANT:
        // Do not sleep while holding g_cb.m
        std::this_thread::sleep_for(
            std::chrono::milliseconds(20));
    }

    std::cout << "[readCameraParam] Timeout waiting for: "
              << id << std::endl;

    return false;
}

inline bool setCameraParam(const char* id, uint32_t value,
                           int timeoutMs = 4000) {
    // Firmware may not emit a second PARAM_EXT_VALUE for a no-op write.
    double current = -1;
    if (readCameraParam(id, current, 1000) &&
        static_cast<uint32_t>(current) == value) {
        std::cout << "setCameraParam: id=" << id
                  << ", already=" << std::fixed << std::setprecision(2)
                  << current << std::endl;
        return true;
    }

    char mutableId[CAM_PARAM_ID_LEN] = {};
    std::strncpy(mutableId, id, sizeof(mutableId) - 1);
    g_payload->setPayloadCameraParam(mutableId, value, PARAM_TYPE_UINT32);

    double actual = -1;
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        if (readCameraParam(id, actual, 1000) &&
            static_cast<uint32_t>(actual) == value) {
            return true;
        }
    }
    std::cout << "setCameraParam: id=" << id << ", expected="
              << std::fixed << std::setprecision(2) << value
              << ", actual=" << std::fixed << std::setprecision(2) << actual
              << std::endl;
    return false;
}

inline void selectIrView() {
    char sourceId[CAM_PARAM_ID_LEN] = {};
    std::strncpy(sourceId, PAYLOAD_CAMERA_VIEW_SRC, sizeof(sourceId) - 1);
    g_payload->setPayloadCameraParam(sourceId, PAYLOAD_CAMERA_VIEW_IR,
                                     PARAM_TYPE_UINT32);
}

inline bool readCaptureStatus(CaptureStatus& status, int timeoutMs = 3000) {
    const uint64_t seq = g_cb.cameraCaptureStatusSeq.load();
    g_payload->getPayloadCaptureStatus();
    if (!waitForSeq(g_cb.cameraCaptureStatusSeq, seq, timeoutMs)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_cb.m);
    status.image = g_cb.cameraCaptureStatus[0];
    status.video = g_cb.cameraCaptureStatus[1];
    status.count = g_cb.cameraCaptureStatus[2];
    status.recordingMs = g_cb.cameraCaptureStatus[3];

    std::cout << "readCaptureStatus: image=" << status.image
              << ", video=" << status.video
              << ", count=" << status.count
              << ", recordingMs=" << status.recordingMs
              << std::endl;
    return true;
}

inline bool setCameraMode(CAMERA_MODE mode, int timeoutMs = 4000) {
    // A no-op mode change may not generate a status callback.
    const uint64_t currentSeq = g_cb.cameraSettingsSeq.load();
    g_payload->getPayloadCameraMode();
    if (waitForSeq(g_cb.cameraSettingsSeq, currentSeq, 1000)) {
        std::lock_guard<std::mutex> lock(g_cb.m);
        if (static_cast<int>(g_cb.cameraSettings[0]) ==
            static_cast<int>(mode)) {
            return true;
        }
    }

    const uint64_t ackSeq = getCommandAckSeq(MAV_CMD_SET_CAMERA_MODE);
    g_payload->setPayloadCameraMode(mode);
    double actual = -1;
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        const uint64_t seq = g_cb.cameraSettingsSeq.load();
        g_payload->getPayloadCameraMode();
        if (waitForSeq(g_cb.cameraSettingsSeq, seq, 1000)) {
            std::lock_guard<std::mutex> lock(g_cb.m);
            actual = g_cb.cameraSettings[0];
            if (static_cast<int>(actual) == static_cast<int>(mode)) {
                return true;
            }
        }
    }

    AckInfo ack;
    return waitForCommandAck(MAV_CMD_SET_CAMERA_MODE, ackSeq, ack, 1500) &&
           (ack.result == MAV_RESULT_ACCEPTED ||
            ack.result == MAV_RESULT_IN_PROGRESS);
}

inline bool waitForVideoState(bool active, int timeoutMs = 8000,
                              CaptureStatus* output = nullptr) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        CaptureStatus current;
        if (readCaptureStatus(current, 1200) &&
            ((current.video != 0) == active)) {
            if (output) *output = current;
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
    return false;
}

inline bool setAndReadBack(const char* id, uint32_t expected,
                           double& actual, int timeoutMs = 4000) {
    char mutableId[CAM_PARAM_ID_LEN] = {};
    std::strncpy(mutableId, id, sizeof(mutableId) - 1);
    g_payload->setPayloadCameraParam(mutableId, expected, PARAM_TYPE_UINT32);
    if (!readCameraParam(id, actual, timeoutMs)) {
        return false;
    }
    std::cout << "--> Param_id: " << id << ", value: " << std::fixed
              << std::setprecision(2) << actual << std::endl;
    return static_cast<uint32_t>(actual) == expected;
}

class CameraIrTest : public PayloadTest {
protected:
    void SetUp() override {
        // Capture whichever view source was active BEFORE this test forces
        // IR, so it can be restored in TearDown() -- otherwise every test
        // in this file leaves the payload viewing IR for whatever test
        // suite happens to run next, which is exactly the leftover-state
        // class of bug that caused intermittent failures in the stream
        // bitrate tests earlier.
        // g_payload->sdkInitConnection();
        // g_payload->checkPayloadConnection();
        // std::this_thread::sleep_for(std::chrono::milliseconds(300));
        hasOriginalViewSource_ =
            readCameraParam(PAYLOAD_CAMERA_VIEW_SRC, originalViewSource_, 4000);

        if (hasOriginalViewSource_) {
            std::cout << "[SetUp] Original view source: " << originalViewSource_
                      << std::endl;
        } else {
            std::cout << "[SetUp] Could not read original view source before "
                         "switching to IR -- will not attempt to restore it "
                         "in TearDown (rather than guess and possibly leave "
                         "the payload in a WORSE state than before)."
                      << std::endl;
        }

        // C_SOURCE is a command-like source selector on some firmware. The
        // write is accepted, but the firmware does not always emit a
        // PARAM_EXT_VALUE readback for it.
        selectIrView();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    void TearDown() override {
        if (!hasOriginalViewSource_) {
            return; // nothing captured -- see the SetUp() comment on why we don't guess
        }

        const uint32_t restoreValue = static_cast<uint32_t>(originalViewSource_);
        std::cout << "[TearDown] Restoring view source to " << restoreValue
                  << std::endl;

        if (!setCameraParam(PAYLOAD_CAMERA_VIEW_SRC, restoreValue)) {
            std::cout << "[TearDown] WARNING: could not confirm view source "
                         "restored to " << restoreValue << " -- subsequent "
                         "tests in other files may unexpectedly still be "
                         "viewing IR." << std::endl;
        }
    }

private:
    bool hasOriginalViewSource_ = false;
    double originalViewSource_ = 0;
};

}  // namespace camera_ir_test

class CameraIrTest : public camera_ir_test::CameraIrTest {};

#endif