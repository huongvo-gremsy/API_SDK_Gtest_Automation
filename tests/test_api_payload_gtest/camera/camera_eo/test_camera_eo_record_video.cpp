/**
 * @file test_camera_eo_record_video.cpp
 * @brief Direct EO record-video API tests followed by the complete example flow.
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

bool isAccepted(const AckInfo& ack) {
    return ack.result == MAV_RESULT_ACCEPTED ||
           ack.result == MAV_RESULT_IN_PROGRESS;
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

}  // namespace

class CameraEoRecordVideoTest : public cet::CameraEoTest {};

// Direct-API tests are intentionally placed before the complete flow test so
// a failure identifies the exact SDK request that is unavailable.

TEST_F(CameraEoRecordVideoTest,
       SetPayloadCameraParam_ViewSourceReadBack) {
    char viewSourceId[] = PAYLOAD_CAMERA_VIEW_SRC;

    // API under test.
    g_payload->setPayloadCameraParam(
        viewSourceId, PAYLOAD_CAMERA_VIEW_EO, PARAM_TYPE_UINT32);

    EXPECT_TRUE(waitForUint32ParamValue(
        PAYLOAD_CAMERA_VIEW_SRC, PAYLOAD_CAMERA_VIEW_EO, 4000))
        << "setPayloadCameraParam() did not set the view source to EO.";
}

#ifndef ZIO
TEST_F(CameraEoRecordVideoTest,
       SetPayloadCameraParam_RecordSourceReadBack) {
    char recordSourceId[] = PAYLOAD_CAMERA_RECORD_SRC;

    // API under test. BOTH is an observable value used by the SDK example.
    g_payload->setPayloadCameraParam(
        recordSourceId, PAYLOAD_CAMERA_RECORD_BOTH, PARAM_TYPE_UINT32);

    ASSERT_TRUE(waitForUint32ParamValue(
        PAYLOAD_CAMERA_RECORD_SRC, PAYLOAD_CAMERA_RECORD_BOTH, 4000))
        << "setPayloadCameraParam() did not set the record source to BOTH.";

    // Restore the EO precondition for the remainder of this test.
    g_payload->setPayloadCameraParam(
        recordSourceId, PAYLOAD_CAMERA_RECORD_EO, PARAM_TYPE_UINT32);
    EXPECT_TRUE(waitForUint32ParamValue(
        PAYLOAD_CAMERA_RECORD_SRC, PAYLOAD_CAMERA_RECORD_EO, 4000));
}
#endif

TEST_F(CameraEoRecordVideoTest, GetPayloadStorage) {
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

TEST_F(CameraEoRecordVideoTest, GetPayloadCaptureStatus) {
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

TEST_F(CameraEoRecordVideoTest, GetPayloadCameraMode) {
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

TEST_F(CameraEoRecordVideoTest,
       SetPayloadCameraMode_VideoStateOrAcceptedAck) {
    const uint64_t ackSeq = getCommandAckSeq(MAV_CMD_SET_CAMERA_MODE);

    // API under test.
    g_payload->setPayloadCameraMode(CAMERA_MODE_VIDEO);

    double mode = -1;
    const bool stateChanged = getCameraMode(mode, 4000) &&
                              static_cast<int>(mode) == CAMERA_MODE_VIDEO;
    AckInfo ack;
    const bool receivedAck = waitForCommandAck(
        MAV_CMD_SET_CAMERA_MODE, ackSeq, ack, 2000);

    EXPECT_TRUE(stateChanged || (receivedAck && isAccepted(ack)))
        << "setPayloadCameraMode(VIDEO) produced neither VIDEO readback nor "
           "an accepted command ACK; reported mode="
        << mode << ".";
}

TEST_F(CameraEoRecordVideoTest,
       SetPayloadCameraRecordVideoStart_ActivatesAndAdvancesTime) {
    double availableMb = -1;
    ASSERT_TRUE(checkStorageReady(availableMb, 10.0, 4000))
        << "EO recording storage is not ready; available=" << availableMb
        << " MB.";
    ASSERT_TRUE(cet::setCameraModeStateOrAck(CAMERA_MODE_VIDEO));
    ASSERT_TRUE(cet::stopRecording());

    const uint64_t ackSeq = getCommandAckSeq(MAV_CMD_VIDEO_START_CAPTURE);

    // API under test.
    g_payload->setPayloadCameraRecordVideoStart();

    cet::CaptureStatus first;
    const bool becameActive = cet::waitForRecording(true, 8000, &first);
    AckInfo ack;
    const bool receivedAck = waitForCommandAck(
        MAV_CMD_VIDEO_START_CAPTURE, ackSeq, ack, 1500);
    AckInfo stopAck;

    const uint64_t seq =
        getCommandAckSeq(MAV_CMD_VIDEO_STOP_CAPTURE);

    cet::CaptureStatus later = first;
    if (becameActive) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        cet::waitForRecording(true, 3000, &later);
    }

    // Always stop before reporting nonfatal verification failures.
    g_payload->setPayloadCameraRecordVideoStop();
    // EXPECT_TRUE(waitForCommandAck(
    //     MAV_CMD_VIDEO_STOP_CAPTURE,
    //     seq,
    //     stopAck,
    //     2000))
    //     << "No COMMAND_ACK for MAV_CMD_VIDEO_STOP_CAPTURE.";
    const bool returnedIdle = cet::waitForRecording(false, 8000);

    EXPECT_TRUE(becameActive)
        << "setPayloadCameraRecordVideoStart() never made video_status active.";
    if (receivedAck) {
        EXPECT_TRUE(isAccepted(ack))
            << "Start-record command was rejected; ACK result="
            << static_cast<int>(ack.result) << ".";
    }
    if (becameActive) {
        EXPECT_GT(later.recordingMs, first.recordingMs)
            << "Recording time did not advance while video_status was active.";
    }
    EXPECT_TRUE(returnedIdle) << "Cleanup could not stop the recording.";
}

TEST_F(CameraEoRecordVideoTest,
       SetPayloadCameraRecordVideoStop_ReturnsToIdle) {
    double availableMb = -1;
    ASSERT_TRUE(checkStorageReady(availableMb, 10.0, 4000))
        << "EO recording storage is not ready; available=" << availableMb
        << " MB.";
    ASSERT_TRUE(cet::setCameraModeStateOrAck(CAMERA_MODE_VIDEO));
    ASSERT_TRUE(cet::stopRecording());

    // Arrange an active recording so the stop API has an observable effect.
    g_payload->setPayloadCameraRecordVideoStart();
    ASSERT_TRUE(cet::waitForRecording(true, 8000))
        << "Could not start the prerequisite recording.";

    const uint64_t ackSeq = getCommandAckSeq(MAV_CMD_VIDEO_STOP_CAPTURE);

    // API under test.
    g_payload->setPayloadCameraRecordVideoStop();

    const bool returnedIdle = cet::waitForRecording(false, 8000);
    AckInfo ack;
    const bool receivedAck = waitForCommandAck(
        MAV_CMD_VIDEO_STOP_CAPTURE, ackSeq, ack, 1500);

    EXPECT_TRUE(returnedIdle)
        << "setPayloadCameraRecordVideoStop() did not return video_status to idle.";
    if (receivedAck) {
        EXPECT_TRUE(isAccepted(ack))
            << "Stop-record command was rejected; ACK result="
            << static_cast<int>(ack.result) << ".";
    }
}

TEST_F(CameraEoRecordVideoTest, ExampleFlow_StartTimeAdvancesThenStop) {
    // 1. Select the EO view source exactly as the SDK example does.
    char viewSourceId[] = PAYLOAD_CAMERA_VIEW_SRC;
    g_payload->setPayloadCameraParam(
        viewSourceId, PAYLOAD_CAMERA_VIEW_EO, PARAM_TYPE_UINT32);
    ASSERT_TRUE(waitForUint32ParamValue(
        PAYLOAD_CAMERA_VIEW_SRC, PAYLOAD_CAMERA_VIEW_EO, 4000));

#ifndef ZIO
    // 2. The example records both sources on payloads that support it.
    char recordSourceId[] = PAYLOAD_CAMERA_RECORD_SRC;
    g_payload->setPayloadCameraParam(
        recordSourceId, PAYLOAD_CAMERA_RECORD_BOTH, PARAM_TYPE_UINT32);
    ASSERT_TRUE(waitForUint32ParamValue(
        PAYLOAD_CAMERA_RECORD_SRC, PAYLOAD_CAMERA_RECORD_BOTH, 4000));
#endif

    // 3. Query storage directly and require enough room for a short recording.
    uint64_t seq = g_cb.cameraStorageInfoSeq.load();
    g_payload->getPayloadStorage();
    ASSERT_TRUE(waitForSeq(g_cb.cameraStorageInfoSeq, seq, 4000));
    StorageInfo storage;
    copyStorageInfo(storage);
    ASSERT_GE(storage.available, 10.0)
        << "EO recording storage is not ready.";

    // 4. Query the initial capture state directly.
    seq = g_cb.cameraCaptureStatusSeq.load();
    g_payload->getPayloadCaptureStatus();
    ASSERT_TRUE(waitForSeq(g_cb.cameraCaptureStatusSeq, seq, 3000));
    cet::CaptureStatus idle;
    copyCaptureStatus(idle);
    ASSERT_EQ(idle.video, 0);

    // 5. Read and, when necessary, change the camera mode directly.
    seq = g_cb.cameraSettingsSeq.load();
    g_payload->getPayloadCameraMode();
    ASSERT_TRUE(waitForSeq(g_cb.cameraSettingsSeq, seq, 3000));
    double mode = -1;
    copyCameraMode(mode);
    if (static_cast<int>(mode) != CAMERA_MODE_VIDEO) {
        const uint64_t ackSeq = getCommandAckSeq(MAV_CMD_SET_CAMERA_MODE);
        g_payload->setPayloadCameraMode(CAMERA_MODE_VIDEO);
        const bool stateChanged = getCameraMode(mode, 4000) &&
                                  static_cast<int>(mode) == CAMERA_MODE_VIDEO;
        AckInfo ack;
        const bool receivedAck = waitForCommandAck(
            MAV_CMD_SET_CAMERA_MODE, ackSeq, ack, 2000);
        ASSERT_TRUE(stateChanged || (receivedAck && isAccepted(ack)))
            << "Could not select VIDEO mode.";
    }

    // 6. Start, observe time advancing, then stop the recording.
    g_payload->setPayloadCameraRecordVideoStart();
    cet::CaptureStatus first;
    ASSERT_TRUE(cet::waitForRecording(true, 8000, &first))
        << "video_status never became active.";

    std::this_thread::sleep_for(std::chrono::seconds(3));
    cet::CaptureStatus later;
    ASSERT_TRUE(cet::waitForRecording(true, 3000, &later));

    g_payload->setPayloadCameraRecordVideoStop();
    const bool returnedIdle = cet::waitForRecording(false, 8000);

    EXPECT_GT(later.recordingMs, first.recordingMs);
    EXPECT_TRUE(returnedIdle) << "video_status did not return to idle.";
}


// Manual test
class ManualCameraEoRecordVideoTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use your existing SDK initialization here if needed.
    }

    void TearDown() override {
        // Cleanup if needed.
    }
};
TEST_F(ManualCameraEoRecordVideoTest, setPayloadCameraModeManual) {
    const uint64_t seq = g_cb.cameraSettingsSeq.load();

    g_payload->setPayloadCameraMode(CAMERA_MODE_VIDEO);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    g_payload->getPayloadCameraMode();
    ASSERT_TRUE(waitForSeq(
        g_cb.cameraSettingsSeq,
        seq,
        4000
    )) << "No PAYLOAD_CAM_SETTINGS callback received.";
    std::lock_guard<std::mutex> lk(g_cb.m);

    std::cout
        << "[TEST] mode_id="
        << g_cb.cameraSettings[0]
        << ", zoomLevel="
        << g_cb.cameraSettings[1]
        << ", focusLevel="
        << g_cb.cameraSettings[2]
        << std::endl;

    EXPECT_EQ(
        static_cast<int>(g_cb.cameraSettings[0]),
        static_cast<int>(CAMERA_MODE_VIDEO)
    );
}

TEST_F(ManualCameraEoRecordVideoTest, setPayloadStartRecordingManual) {
    g_payload->setPayloadCameraRecordVideoStart();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    // g_payload->getPayloadCaptureStatus();

}

TEST_F(ManualCameraEoRecordVideoTest, GetPayloadCaptureStatusManual) {
    g_payload->getPayloadCaptureStatus();
    std::this_thread::sleep_for(std::chrono::seconds(1));
}


TEST_F(ManualCameraEoRecordVideoTest, setPayloadStopRecordingManual) {
    g_payload->setPayloadCameraRecordVideoStop();
    std::this_thread::sleep_for(std::chrono::seconds(1));
}