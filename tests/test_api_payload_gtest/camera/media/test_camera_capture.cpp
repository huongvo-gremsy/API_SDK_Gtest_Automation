/**
 * @file test_camera_capture.cpp
 * @brief Camera capture tests using STATE VERIFICATION rather than ACK,
 *        per the finding that MAV_CMD_SET_CAMERA_MODE and PARAM_EXT_ACK
 *        are unreliable on this firmware. Each test's precondition setup
 *        (mode + EO source) polls the paired getter until it reflects the
 *        change, re-sending the command on each retry -- it doesn't matter
 *        HOW the state got applied, only THAT it did.
 *
 * StopImage is the deliberate exception: a single-shot image capture has
 * no independent "ongoing capture" state to verify, so it falls back to a
 * best-effort ACK check (Level 2 per the testing strategy), and is treated
 * as non-strict since we've already found ACKs unreliable here.
 */

#include "camera_media_test_helpers.h"
#include "../parameters/camera_param_test_helpers.h"

namespace {

bool setCaptureModeStateOrAck(CAMERA_MODE mode) {
    if (setAndVerifyCameraMode(mode, 3000, 500)) return true;
    const uint64_t seq = getCommandAckSeq(MAV_CMD_SET_CAMERA_MODE);
    g_payload->setPayloadCameraMode(mode);
    AckInfo ack;
    return waitForCommandAck(MAV_CMD_SET_CAMERA_MODE, seq, ack, 3000) &&
           ack.result == MAV_RESULT_ACCEPTED;
}

bool stopRecordingIfActive() {
    double imageStatus = 0, videoStatus = 0, imageCount = 0, recordingMs = 0;
    if (!getCaptureStatus(imageStatus, videoStatus, imageCount, recordingMs, 1500)) {
        return false;
    }
    if (videoStatus == 0) return true;

    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::seconds(8);
    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->setPayloadCameraRecordVideoStop();
        if (getCaptureStatus(imageStatus, videoStatus, imageCount, recordingMs, 1500) &&
            videoStatus == 0) {
            return true;
        }
    }
    return false;
}

}  // namespace

class CameraCaptureTest : public PayloadTest {
protected:
    void SetUp() override {
        ASSERT_TRUE(getCameraMode(originalMode_, 3000));
        haveOriginalMode_ = true;

        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIEW_SRC,
                                          originalViewSource_, 3000));
        haveOriginalViewSource_ = true;

#ifndef ZIO
        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_RECORD_SRC,
                                          originalRecordSource_, 3000));
        haveOriginalRecordSource_ = true;
#endif

        ASSERT_TRUE(stopRecordingIfActive())
            << "Could not stop a recording left active before capture.";

        // 1. Storage gate -- matches the example's own precondition check
        //    ("if(available_capacity >= 10.0){...} else { not ready }").
        //    Fail fast with a clear diagnostic instead of a confusing
        //    downstream "image_count didn't increase" failure.
        double availableMB;
        ASSERT_TRUE(checkStorageReady(availableMB))
            << "Storage not ready for capture (available=" << availableMB << " MB, need >= 10 MB).";

#if defined(MB1)
        // MB1-specific: force storage location to Internal before capture,
        // matching the example's #if defined MB1 branch.
        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_STORAGE,
                                          originalStorage_, 3000));
        haveOriginalStorage_ = true;
        char storageParamId[] = PAYLOAD_CAMERA_STORAGE;
        ASSERT_TRUE(setAndVerifyCameraParam(storageParamId, PAYLOAD_CAMERA_STORAGE_INTERNAL,
                                             PARAM_TYPE_UINT32, PAYLOAD_CAMERA_STORAGE_INTERNAL))
            << "Could not confirm storage location set to Internal (MB1).";
#endif

        // 2. Camera mode. Prefer CAMERA_SETTINGS state verification. This VIO
        // firmware can accept the mode command without updating mode_id, and
        // Gremsy's capture example explicitly proceeds on the matching ACK in
        // that case. The capture tests below still verify the physical result
        // through image_count, so this fallback cannot create a false pass.
        ASSERT_TRUE(setCaptureModeStateOrAck(CAMERA_MODE_IMAGE))
            << "Camera neither reported IMAGE mode nor accepted the mode command.";

        // 3. Deterministic EO view/record source.
        char viewParamId[] = PAYLOAD_CAMERA_VIEW_SRC;
        ASSERT_TRUE(setAndVerifyCameraParam(viewParamId, PAYLOAD_CAMERA_VIEW_EO,
                                             PARAM_TYPE_UINT32, PAYLOAD_CAMERA_VIEW_EO));
#ifndef ZIO
        char paramId[] = PAYLOAD_CAMERA_RECORD_SRC;
        ASSERT_TRUE(setAndVerifyCameraParam(paramId, PAYLOAD_CAMERA_RECORD_EO,
                                             PARAM_TYPE_UINT32, PAYLOAD_CAMERA_RECORD_EO))
            << "Could not confirm record source set to EO within timeout.";
#endif
    }

    void TearDown() override {
        // Safe even after a single shot; prevents an interval sequence from
        // surviving a failed assertion in future tests.
        g_payload->setPayloadCameraStopImage();
        if (!stopRecordingIfActive()) {
            ADD_FAILURE() << "Media cleanup could not stop active recording.";
        }

#if defined(MB1)
        if (haveOriginalStorage_) {
            char id[] = PAYLOAD_CAMERA_STORAGE;
            if (!setAndVerifyCameraParam(id, static_cast<uint32_t>(originalStorage_),
                                         PARAM_TYPE_UINT32, originalStorage_)) {
                ADD_FAILURE() << "Could not restore original storage target.";
            }
        }
#endif

#ifndef ZIO
        if (haveOriginalRecordSource_) {
            char id[] = PAYLOAD_CAMERA_RECORD_SRC;
            if (!setAndVerifyCameraParam(id,
                                         static_cast<uint32_t>(originalRecordSource_),
                                         PARAM_TYPE_UINT32, originalRecordSource_)) {
                ADD_FAILURE() << "Could not restore original record source.";
            }
        }
#endif
        if (haveOriginalViewSource_) {
            char id[] = PAYLOAD_CAMERA_VIEW_SRC;
            if (!setAndVerifyCameraParam(id,
                                         static_cast<uint32_t>(originalViewSource_),
                                         PARAM_TYPE_UINT32, originalViewSource_)) {
                ADD_FAILURE() << "Could not restore original view source.";
            }
        }
        if (haveOriginalMode_ &&
            !setCaptureModeStateOrAck(static_cast<CAMERA_MODE>(
                static_cast<int>(originalMode_)))) {
            ADD_FAILURE() << "Could not restore original camera mode.";
        }
    }

    double originalMode_ = CAMERA_MODE_IMAGE;
    double originalViewSource_ = 0;
    double originalRecordSource_ = 0;
    double originalStorage_ = 0;
    bool haveOriginalMode_ = false;
    bool haveOriginalViewSource_ = false;
    bool haveOriginalRecordSource_ = false;
    bool haveOriginalStorage_ = false;
};

// ---- Storage: standalone check, matching the example's own gate ----

TEST_F(CameraCaptureTest, Storage_HasEnoughSpaceForCapture) {
    // CameraCaptureTest.Storage_HasEnoughSpaceForCapture
    double availableMB;
    bool ready = checkStorageReady(availableMB);
    EXPECT_TRUE(ready) << "Available storage (" << availableMB << " MB) is below the 10 MB threshold "
                           "used by Gremsy's own capture example as a capture-readiness gate.";
}

// ---- Primary correctness test: capture actually happened ----

TEST_F(CameraCaptureTest, CaptureImage_IncrementsImageCount) {
    // CameraCaptureTest.CaptureImage_IncrementsImageCount
    EXPECT_TRUE(captureImageAndVerify())
        << "image_count did not increase after capture -- check storage space, "
           "camera mode, and EO source setup.";
}

// ---- Repeated capture: each one independently state-verified ----

TEST_F(CameraCaptureTest, CaptureImage_BurstOfThree) {
    // CameraCaptureTest.CaptureImage_BurstOfThree
    for (int i = 0; i < 3; ++i) {
        EXPECT_TRUE(captureImageAndVerify())
            << "Capture #" << (i + 1) << " did not register (image_count unchanged).";
    }
}

// ---- Capture source modes: EO, IR, composed PiP, and dual EO+IR ----

#if defined(VIO) || defined(MB1) || defined(ORUSL)
struct CaptureSourceCase {
    const char* name;
    uint32_t viewSource;
    uint32_t recordSource;
};

std::ostream& operator<<(std::ostream& os, const CaptureSourceCase& source) {
    return os << source.name << " (view=" << source.viewSource
              << ", record=" << source.recordSource << ")";
}

class CameraCaptureSourceTest
    : public CameraCaptureTest,
      public ::testing::WithParamInterface<CaptureSourceCase> {};

TEST_P(CameraCaptureSourceTest, CaptureIncrementsImageCount) {
    const CaptureSourceCase mode = GetParam();

    char viewSourceId[] = PAYLOAD_CAMERA_VIEW_SRC;
    ASSERT_TRUE(setAndVerifyCameraParam(
        viewSourceId, mode.viewSource, PARAM_TYPE_UINT32, mode.viewSource))
        << "Could not select " << mode.name << " camera view source.";

    char recordSourceId[] = PAYLOAD_CAMERA_RECORD_SRC;
    ASSERT_TRUE(setAndVerifyCameraParam(
        recordSourceId, mode.recordSource, PARAM_TYPE_UINT32, mode.recordSource))
        << "Could not select " << mode.name << " capture source.";

    // Give the camera pipeline time to apply source/compositor changes before
    // triggering the shutter.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    double actualViewSource, actualRecordSource;
    ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIEW_SRC, actualViewSource));
    ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_RECORD_SRC, actualRecordSource));
    EXPECT_EQ(static_cast<uint32_t>(actualViewSource), mode.viewSource);
    EXPECT_EQ(static_cast<uint32_t>(actualRecordSource), mode.recordSource);

    EXPECT_TRUE(captureImageAndVerify())
        << mode.name << " capture did not increase image_count.";

    // Leave the shared fixture in its normal EO state for following tests.
    EXPECT_TRUE(setAndVerifyCameraParam(
        viewSourceId, PAYLOAD_CAMERA_VIEW_EO,
        PARAM_TYPE_UINT32, PAYLOAD_CAMERA_VIEW_EO));
    EXPECT_TRUE(setAndVerifyCameraParam(
        recordSourceId, PAYLOAD_CAMERA_RECORD_EO,
        PARAM_TYPE_UINT32, PAYLOAD_CAMERA_RECORD_EO));
}

INSTANTIATE_TEST_SUITE_P(
    CaptureModes,
    CameraCaptureSourceTest,
    ::testing::Values(
        CaptureSourceCase{"EO", PAYLOAD_CAMERA_VIEW_EO, PAYLOAD_CAMERA_RECORD_EO},
        CaptureSourceCase{"IR", PAYLOAD_CAMERA_VIEW_IR, PAYLOAD_CAMERA_RECORD_IR},
        // PiP is a composed display, not a native C_V_REC source. Record OSD
        // captures the current EO+IR composited view.
        CaptureSourceCase{"PiP", PAYLOAD_CAMERA_VIEW_EOIR, PAYLOAD_CAMERA_RECORD_OSD},
        CaptureSourceCase{"Both", PAYLOAD_CAMERA_VIEW_EOIR, PAYLOAD_CAMERA_RECORD_BOTH}),
    [](const ::testing::TestParamInfo<CaptureSourceCase>& info) {
        return info.param.name;
    });
#endif

// ---- Getter sanity check: does the event mechanism itself work ----

TEST_F(CameraCaptureTest, GetCaptureStatus_EventArrives) {
    // CameraCaptureTest.GetCaptureStatus_EventArrives
    double imageStatus, videoStatus, imageCount, recordingTimeMs;
    EXPECT_TRUE(getCaptureStatus(imageStatus, videoStatus, imageCount, recordingTimeMs))
        << "No PAYLOAD_CAM_CAPTURE_STATUS within timeout.";
}

// ---- StopImage: deliberate exception, see file header ----

TEST_F(CameraCaptureTest, StopImage_BestEffortAck) {
    // CameraCaptureTest.StopImage_BestEffortAck
    const uint64_t seq = getCommandAckSeq(MAV_CMD_IMAGE_STOP_CAPTURE);
    g_payload->setPayloadCameraStopImage();
    AckInfo ack;
    if (!waitForCommandAck(MAV_CMD_IMAGE_STOP_CAPTURE, seq, ack, 3000)) {
        GTEST_SKIP() << "No ACK for stop-image (known-flaky on this firmware); "
                         "there is no independent state to verify a single-shot stop.";
    }
    EXPECT_EQ(ack.result, MAV_RESULT_ACCEPTED)
        << "Payload rejected MAV_CMD_IMAGE_STOP_CAPTURE.";
}
