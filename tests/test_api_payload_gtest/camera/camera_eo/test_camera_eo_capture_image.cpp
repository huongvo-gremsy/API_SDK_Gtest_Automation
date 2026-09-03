/**
 * @file test_camera_eo_capture_image.cpp
 * @brief Direct EO capture API tests followed by the complete example flow.
 */

#include "camera_eo_test_helpers.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <thread>

namespace cet = camera_eo_test;

namespace {

struct StorageInfo {
    double total = 0;
    double used = 0;
    double available = 0;
    double status = -1;
};

void copyStorageInfo(StorageInfo& info) {
    std::lock_guard<std::mutex> lock(g_cb.m);
    info.total = g_cb.cameraStorageInfo[0];
    info.used = g_cb.cameraStorageInfo[1];
    info.available = g_cb.cameraStorageInfo[2];
    info.status = g_cb.cameraStorageInfo[3];
}

void copyCaptureStatus(cet::CaptureStatus& status) {
    std::lock_guard<std::mutex> lock(g_cb.m);
    status.image = g_cb.cameraCaptureStatus[0];
    status.video = g_cb.cameraCaptureStatus[1];
    status.count = g_cb.cameraCaptureStatus[2];
    status.recordingMs = g_cb.cameraCaptureStatus[3];
}

void copyCameraMode(double& mode) {
    std::lock_guard<std::mutex> lock(g_cb.m);
    mode = g_cb.cameraSettings[0];
}

bool isKnownCameraMode(double mode) {
    const int value = static_cast<int>(mode);
    return value == CAMERA_MODE_IMAGE ||
           value == CAMERA_MODE_VIDEO ||
           value == CAMERA_MODE_IMAGE_SURVEY;
}

bool waitForUint32ParamValue(const char* id, uint32_t expected,
                             int timeoutMs) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        double actual = -1;
        if (getCameraSettingByID(id, actual, 1000) &&
            static_cast<uint32_t>(actual) == expected) {
            return true;
        }
    }
    return false;
}

bool waitForImageCountGreaterThan(double baseline, int timeoutMs,
                                  cet::CaptureStatus& last,
                                  bool* sawBusy = nullptr) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        if (cet::readCaptureStatus(last, 1500)) {
            if (sawBusy != nullptr) *sawBusy = *sawBusy || last.image != 0;
            if (last.count > baseline) return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    return false;
}

bool waitForImageIdle(int timeoutMs, cet::CaptureStatus* output = nullptr) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        cet::CaptureStatus current;
        if (cet::readCaptureStatus(current, 1000) && current.image == 0) {
            if (output != nullptr) *output = current;
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    return false;
}

}  // namespace

class CameraEoCaptureImageTest : public cet::CameraEoTest {};

// Individual direct-API tests come before the complete flow test so a failure
// identifies the specific SDK request that is unavailable.

#ifndef ZIO
TEST_F(CameraEoCaptureImageTest,
       DirectSetPayloadCameraParam_RecordSourceReadBack) {
    double original = -1;
    ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_RECORD_SRC, original, 3000));

    char recordSourceId[] = PAYLOAD_CAMERA_RECORD_SRC;

    // API under test. BOTH is an observable alternate value and does not start
    // recording; the EO fixture restores the user's original value afterward.
    g_payload->setPayloadCameraParam(
        recordSourceId, PAYLOAD_CAMERA_RECORD_BOTH, PARAM_TYPE_UINT32);

    ASSERT_TRUE(waitForUint32ParamValue(
        PAYLOAD_CAMERA_RECORD_SRC, PAYLOAD_CAMERA_RECORD_BOTH, 4000))
        << "setPayloadCameraParam() did not change the record source to BOTH.";

    // Return this test to the EO precondition without hiding the direct API.
    g_payload->setPayloadCameraParam(
        recordSourceId, PAYLOAD_CAMERA_RECORD_EO, PARAM_TYPE_UINT32);
    EXPECT_TRUE(waitForUint32ParamValue(
        PAYLOAD_CAMERA_RECORD_SRC, PAYLOAD_CAMERA_RECORD_EO, 4000))
        << "Could not restore the EO record source after the direct API test; "
        << "value before test=" << original << ".";
}
#endif

TEST_F(CameraEoCaptureImageTest, GetPayloadStorage) {
    const uint64_t seq = g_cb.cameraStorageInfoSeq.load();

    // API under test.
    g_payload->getPayloadStorage();

    ASSERT_TRUE(waitForSeq(g_cb.cameraStorageInfoSeq, seq, 4000))
        << "getPayloadStorage() produced no PAYLOAD_CAM_STORAGE_INFO event.";

    StorageInfo info;
    copyStorageInfo(info);
    EXPECT_TRUE(std::isfinite(info.total));
    EXPECT_TRUE(std::isfinite(info.used));
    EXPECT_TRUE(std::isfinite(info.available));
    EXPECT_GE(info.total, 0.0);
    EXPECT_GE(info.used, 0.0);
    EXPECT_GE(info.available, 0.0);
}

TEST_F(CameraEoCaptureImageTest,
       GetPayloadCaptureStatus) {
    const uint64_t seq = g_cb.cameraCaptureStatusSeq.load();

    // API under test.
    g_payload->getPayloadCaptureStatus();

    ASSERT_TRUE(waitForSeq(g_cb.cameraCaptureStatusSeq, seq, 3000))
        << "getPayloadCaptureStatus() produced no capture-status event.";

    cet::CaptureStatus status;
    copyCaptureStatus(status);
    EXPECT_TRUE(status.image == 0 || status.image == 1);
    EXPECT_TRUE(status.video == 0 || status.video == 1);
    EXPECT_TRUE(std::isfinite(status.count));
    EXPECT_GE(status.count, 0.0);
    EXPECT_EQ(status.count, std::floor(status.count));
    EXPECT_TRUE(std::isfinite(status.recordingMs));
    EXPECT_GE(status.recordingMs, 0.0);
}

TEST_F(CameraEoCaptureImageTest, GetPayloadCameraMode) {
    const uint64_t seq = g_cb.cameraSettingsSeq.load();

    // API under test.
    g_payload->getPayloadCameraMode();

    ASSERT_TRUE(waitForSeq(g_cb.cameraSettingsSeq, seq, 3000))
        << "getPayloadCameraMode() produced no PAYLOAD_CAM_SETTINGS event.";

    double mode = -1;
    copyCameraMode(mode);
    EXPECT_TRUE(isKnownCameraMode(mode))
        << "Payload returned unknown camera mode " << mode << ".";
}

TEST_F(CameraEoCaptureImageTest,
       SetPayloadCameraMode_ImageStateOrAcceptedAck) {
    const uint64_t ackSeq = getCommandAckSeq(MAV_CMD_SET_CAMERA_MODE);

    // API under test.
    g_payload->setPayloadCameraMode(CAMERA_MODE_IMAGE);

    double mode = -1;
    const bool stateChanged = getCameraMode(mode, 4000) &&
                              static_cast<int>(mode) == CAMERA_MODE_IMAGE;
    AckInfo ack;
    const bool accepted = waitForCommandAck(
        MAV_CMD_SET_CAMERA_MODE, ackSeq, ack, 2000) &&
        (ack.result == MAV_RESULT_ACCEPTED ||
         ack.result == MAV_RESULT_IN_PROGRESS);

    EXPECT_TRUE(stateChanged || accepted)
        << "setPayloadCameraMode(IMAGE) produced neither IMAGE readback nor "
           "an accepted command ACK; reported mode="
        << mode << ".";
}

TEST_F(CameraEoCaptureImageTest,
       SetPayloadCameraCaptureImage_IncrementsImageCount) {
    double availableMb = -1;
    ASSERT_TRUE(checkStorageReady(availableMb, 10.0, 4000))
        << "EO capture storage is not ready; available=" << availableMb
        << " MB.";
    ASSERT_TRUE(cet::setCameraModeStateOrAck(CAMERA_MODE_IMAGE));

    cet::CaptureStatus before;
    ASSERT_TRUE(cet::readCaptureStatus(before, 3000));
    ASSERT_EQ(before.video, 0);
    ASSERT_EQ(before.image, 0);

    const uint64_t ackSeq = getCommandAckSeq(MAV_CMD_IMAGE_START_CAPTURE);

    // API under test.
    g_payload->setPayloadCameraCaptureImage();

    AckInfo ack;
    const bool receivedAck = waitForCommandAck(
        MAV_CMD_IMAGE_START_CAPTURE, ackSeq, ack, 2500);
    const bool rejected = receivedAck &&
                          ack.result != MAV_RESULT_ACCEPTED &&
                          ack.result != MAV_RESULT_IN_PROGRESS;

    cet::CaptureStatus last = before;
    bool sawBusy = false;
    const bool increased =
        waitForImageCountGreaterThan(before.count, 20000, last, &sawBusy);

    // Always stop before reporting a nonfatal verification failure.
    g_payload->setPayloadCameraStopImage();
    waitForImageIdle(8000, &last);

    EXPECT_FALSE(rejected)
        << "Payload rejected setPayloadCameraCaptureImage(); ACK result="
        << static_cast<int>(ack.result) << ".";
    EXPECT_TRUE(increased)
        << "Single-image capture did not increment image_count. baseline="
        << before.count << ", final=" << last.count
        << ", sawBusy=" << sawBusy << ", receivedAck=" << receivedAck;
}

TEST_F(CameraEoCaptureImageTest,
       SetPayloadCameraStopImage_IdleCameraRemainsIdle) {
    ASSERT_TRUE(waitForImageIdle(3000));
    const uint64_t ackSeq = getCommandAckSeq(MAV_CMD_IMAGE_STOP_CAPTURE);

    // API under test. Stop is expected to be safe and idempotent while idle.
    g_payload->setPayloadCameraStopImage();

    AckInfo ack;
    const bool receivedAck = waitForCommandAck(
        MAV_CMD_IMAGE_STOP_CAPTURE, ackSeq, ack, 1500);
    cet::CaptureStatus status;
    ASSERT_TRUE(waitForImageIdle(4000, &status));
    EXPECT_EQ(status.image, 0);
    if (receivedAck) {
        EXPECT_TRUE(ack.result == MAV_RESULT_ACCEPTED ||
                    ack.result == MAV_RESULT_IN_PROGRESS)
            << "Stop-image command was rejected; ACK result="
            << static_cast<int>(ack.result) << ".";
    }
}

TEST_F(CameraEoCaptureImageTest, SourceAndImageMode_ReadBack) {
    ASSERT_TRUE(cet::setCameraModeStateOrAck(CAMERA_MODE_IMAGE));
    double view = -1;
    double mode = -1;
    ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIEW_SRC, view, 3000));
    ASSERT_TRUE(getCameraMode(mode, 3000));
    EXPECT_EQ(static_cast<uint32_t>(view), PAYLOAD_CAMERA_VIEW_EO);
    EXPECT_EQ(static_cast<int>(mode), CAMERA_MODE_IMAGE);
}

// Complete example flow, after the individual API checks above.

TEST_F(CameraEoCaptureImageTest, ExampleFlow_CaptureIncrementsImageCount) {
    double availableMb = -1;
    ASSERT_TRUE(checkStorageReady(availableMb, 10.0, 4000))
        << "EO capture storage is not ready; available=" << availableMb
        << " MB.";

    cet::CaptureStatus before;
    ASSERT_TRUE(cet::readCaptureStatus(before, 3000));
    ASSERT_EQ(before.video, 0) << "Recording must be idle before capture.";
    ASSERT_EQ(before.image, 0) << "Image capture must be idle before capture.";

    ASSERT_TRUE(cet::setCameraModeStateOrAck(CAMERA_MODE_IMAGE));
    std::this_thread::sleep_for(std::chrono::seconds(1));

    const uint64_t ackSeq = getCommandAckSeq(MAV_CMD_IMAGE_START_CAPTURE);
    g_payload->setPayloadCameraCaptureImage();

    AckInfo ack;
    const bool receivedAck = waitForCommandAck(
        MAV_CMD_IMAGE_START_CAPTURE, ackSeq, ack, 2500);
    const bool rejected = receivedAck &&
                          ack.result != MAV_RESULT_ACCEPTED &&
                          ack.result != MAV_RESULT_IN_PROGRESS;

    bool sawBusy = false;
    cet::CaptureStatus last = before;
    const bool increased =
        waitForImageCountGreaterThan(before.count, 20000, last, &sawBusy);

    g_payload->setPayloadCameraStopImage();
    const bool returnedIdle = waitForImageIdle(8000, &last);

    EXPECT_FALSE(rejected)
        << "Payload rejected EO capture; ACK result="
        << static_cast<int>(ack.result) << ".";
    EXPECT_TRUE(increased)
        << "EO image_count did not increase. baseline=" << before.count
        << ", final=" << last.count << ", sawBusy=" << sawBusy
        << ", receivedAck=" << receivedAck;
    EXPECT_TRUE(returnedIdle)
        << "Image status did not return to idle after the flow stopped capture.";
}
