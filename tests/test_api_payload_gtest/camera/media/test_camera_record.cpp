/**
 * @file test_camera_record.cpp
 * @brief Video-recording tests verified through CAMERA_CAPTURE_STATUS.
 *
 * Recording commands can be ACKed before the media pipeline changes state.
 * These tests therefore require video_status to become active/idle and use
 * recording_time_ms to prove that an active recording is progressing.
 */

#include "camera_media_test_helpers.h"
#include "../parameters/camera_param_test_helpers.h"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

namespace {

struct CaptureStatus {
    double imageStatus = 0;
    double videoStatus = 0;
    double imageCount = 0;
    double recordingTimeMs = 0;
};

bool readCaptureStatus(CaptureStatus& status, int timeoutMs = 1000) {
    return getCaptureStatus(status.imageStatus, status.videoStatus,
                            status.imageCount, status.recordingTimeMs,
                            timeoutMs);
}

bool waitForRecordingState(bool active, int timeoutMs, CaptureStatus* result = nullptr) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    CaptureStatus current;
    while (std::chrono::steady_clock::now() < deadline) {
        if (readCaptureStatus(current, 800) && ((current.videoStatus != 0) == active)) {
            if (result) *result = current;
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    return false;
}

bool startRecordingAndVerify(int timeoutMs = 8000) {
    g_payload->setPayloadCameraRecordVideoStart();
    if (waitForRecordingState(true, timeoutMs / 2)) return true;

    // One retry handles a command lost while the camera pipeline is switching
    // source/mode. Correctness still depends on observed active state.
    g_payload->setPayloadCameraRecordVideoStart();
    return waitForRecordingState(true, timeoutMs - timeoutMs / 2);
}

bool stopRecordingAndVerify(int timeoutMs = 8000) {
    CaptureStatus current;
    if (readCaptureStatus(current, 1000) && current.videoStatus == 0) return true;

    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->setPayloadCameraRecordVideoStop();
        if (waitForRecordingState(false, 1500)) return true;
    }
    return false;
}

bool setVideoModeStateOrAck() {
    if (setAndVerifyCameraMode(CAMERA_MODE_VIDEO, 3000, 500)) return true;

    const uint64_t seq = getCommandAckSeq(MAV_CMD_SET_CAMERA_MODE);
    g_payload->setPayloadCameraMode(CAMERA_MODE_VIDEO);
    AckInfo ack;
    return waitForCommandAck(MAV_CMD_SET_CAMERA_MODE, seq, ack, 3000) &&
           ack.result == MAV_RESULT_ACCEPTED;
}

}  // namespace

class CameraRecordTest : public PayloadTest {
protected:
    void SetUp() override {
        double availableMB = -1;
        ASSERT_TRUE(checkStorageReady(availableMB, 10.0, 3000))
            << "Storage is not ready for recording (available="
            << availableMB << " MB).";

        ASSERT_TRUE(getCameraMode(originalMode_, 3000))
            << "Could not read the original camera mode.";

        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIEW_SRC,
                                          originalViewSource_, 3000))
            << "Could not read the original camera view source.";
        haveOriginalViewSource_ = true;

#ifndef ZIO
        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_RECORD_SRC,
                                          originalRecordSource_, 3000))
            << "Could not read the original camera record source.";
        haveOriginalRecordSource_ = true;
#endif

        // Recover safely if a previous interrupted test left recording active.
        ASSERT_TRUE(stopRecordingAndVerify())
            << "Could not bring video_status to idle before the test.";
        ASSERT_TRUE(setVideoModeStateOrAck())
            << "Camera neither reported VIDEO mode nor accepted the mode command.";
    }

    void TearDown() override {
        if (!stopRecordingAndVerify()) {
            ADD_FAILURE() << "Cleanup could not stop video recording.";
        }

#ifndef ZIO
        if (haveOriginalRecordSource_) {
            char id[] = PAYLOAD_CAMERA_RECORD_SRC;
            if (!setAndVerifyCameraParam(
                    id, static_cast<uint32_t>(originalRecordSource_),
                    PARAM_TYPE_UINT32, originalRecordSource_)) {
                ADD_FAILURE() << "Could not restore original record source.";
            }
        }
#endif

        if (haveOriginalViewSource_) {
            char id[] = PAYLOAD_CAMERA_VIEW_SRC;
            if (!setAndVerifyCameraParam(
                    id, static_cast<uint32_t>(originalViewSource_),
                    PARAM_TYPE_UINT32, originalViewSource_)) {
                ADD_FAILURE() << "Could not restore original view source.";
            }
        }

        if (!setAndVerifyCameraMode(static_cast<CAMERA_MODE>(
                static_cast<int>(originalMode_)), 3000, 500)) {
            // Mode readback is known not to update on some firmware. Restore
            // is still sent above; do not hide a recording test result for it.
            g_payload->setPayloadCameraMode(static_cast<CAMERA_MODE>(
                static_cast<int>(originalMode_)));
            std::cout << "[WARN] Original mode restore sent but not reflected "
                         "in CAMERA_SETTINGS.mode_id."
                      << std::endl;
        }
    }

    void selectSource(uint32_t viewSource, uint32_t recordSource) {
        char viewId[] = PAYLOAD_CAMERA_VIEW_SRC;
        ASSERT_TRUE(setAndVerifyCameraParam(viewId, viewSource,
                                             PARAM_TYPE_UINT32, viewSource))
            << "Could not set camera view source to " << viewSource << ".";

#ifndef ZIO
        char recordId[] = PAYLOAD_CAMERA_RECORD_SRC;
        ASSERT_TRUE(setAndVerifyCameraParam(recordId, recordSource,
                                             PARAM_TYPE_UINT32, recordSource))
            << "Could not set camera record source to " << recordSource << ".";
#else
        (void)recordSource;
#endif
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    void verifyRecordCycle(uint32_t viewSource, uint32_t recordSource,
                           const char* sourceName) {
        selectSource(viewSource, recordSource);
        ASSERT_TRUE(startRecordingAndVerify())
            << sourceName << " recording never became active.";

        CaptureStatus active;
        ASSERT_TRUE(waitForRecordingState(true, 3000, &active));
        EXPECT_NE(active.videoStatus, 0);

        ASSERT_TRUE(stopRecordingAndVerify())
            << sourceName << " recording did not return to idle.";
    }

    double originalMode_ = CAMERA_MODE_VIDEO;
    double originalViewSource_ = 0;
    double originalRecordSource_ = 0;
    bool haveOriginalViewSource_ = false;
    bool haveOriginalRecordSource_ = false;
};

TEST_F(CameraRecordTest, GetCaptureStatus_InitiallyIdle) {
    CaptureStatus status;
    ASSERT_TRUE(readCaptureStatus(status, 3000))
        << "No CAMERA_CAPTURE_STATUS response.";
    EXPECT_EQ(status.videoStatus, 0)
        << "Recording was active after fixture cleanup.";
}

TEST_F(CameraRecordTest, StartEO_VideoStatusBecomesActive) {
#ifndef ZIO
    verifyRecordCycle(PAYLOAD_CAMERA_VIEW_EO, PAYLOAD_CAMERA_RECORD_EO, "EO");
#else
    verifyRecordCycle(PAYLOAD_CAMERA_VIEW_EO, 0, "EO");
#endif
}

#ifndef ZIO
TEST_F(CameraRecordTest, StartIR_VideoStatusBecomesActive) {
    verifyRecordCycle(PAYLOAD_CAMERA_VIEW_IR, PAYLOAD_CAMERA_RECORD_IR, "IR");
}

TEST_F(CameraRecordTest, StartBoth_VideoStatusBecomesActive) {
    verifyRecordCycle(PAYLOAD_CAMERA_VIEW_EOIR, PAYLOAD_CAMERA_RECORD_BOTH, "Both");
}

TEST_F(CameraRecordTest, StartPiP_VideoStatusBecomesActive) {
    // PiP is a composed view. RECORD_OSD records that composed EO+IR output.
    verifyRecordCycle(PAYLOAD_CAMERA_VIEW_EOIR, PAYLOAD_CAMERA_RECORD_OSD, "PiP");
}
#endif

TEST_F(CameraRecordTest, RecordingTime_Advances) {
#ifndef ZIO
    selectSource(PAYLOAD_CAMERA_VIEW_EO, PAYLOAD_CAMERA_RECORD_EO);
#else
    selectSource(PAYLOAD_CAMERA_VIEW_EO, 0);
#endif
    ASSERT_TRUE(startRecordingAndVerify());

    CaptureStatus first;
    ASSERT_TRUE(waitForRecordingState(true, 3000, &first));

    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::seconds(5);
    CaptureStatus later;
    bool advanced = false;
    while (std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (readCaptureStatus(later, 1000) &&
            later.videoStatus != 0 &&
            later.recordingTimeMs > first.recordingTimeMs) {
            advanced = true;
            break;
        }
    }
    EXPECT_TRUE(advanced)
        << "recording_time_ms did not advance from "
        << first.recordingTimeMs << ".";
}

TEST_F(CameraRecordTest, Stop_VideoStatusReturnsIdle) {
#ifndef ZIO
    selectSource(PAYLOAD_CAMERA_VIEW_EO, PAYLOAD_CAMERA_RECORD_EO);
#else
    selectSource(PAYLOAD_CAMERA_VIEW_EO, 0);
#endif
    ASSERT_TRUE(startRecordingAndVerify());
    EXPECT_TRUE(stopRecordingAndVerify())
        << "video_status did not return to idle after stop.";
}

TEST_F(CameraRecordTest, TearDown_StopsInterruptedRecording) {
#ifndef ZIO
    selectSource(PAYLOAD_CAMERA_VIEW_EO, PAYLOAD_CAMERA_RECORD_EO);
#else
    selectSource(PAYLOAD_CAMERA_VIEW_EO, 0);
#endif
    ASSERT_TRUE(startRecordingAndVerify());
    // Deliberately leave it active. TearDown must stop it and reports a test
    // failure if video_status does not return to idle.
}

TEST_F(CameraRecordTest, StartWhileAlreadyRecording_ObservedBehavior) {
#ifndef ZIO
    selectSource(PAYLOAD_CAMERA_VIEW_EO, PAYLOAD_CAMERA_RECORD_EO);
#else
    selectSource(PAYLOAD_CAMERA_VIEW_EO, 0);
#endif
    ASSERT_TRUE(startRecordingAndVerify());

    g_payload->setPayloadCameraRecordVideoStart();
    CaptureStatus status;
    EXPECT_TRUE(waitForRecordingState(true, 3000, &status))
        << "A duplicate start command unexpectedly made recording inactive.";
}
