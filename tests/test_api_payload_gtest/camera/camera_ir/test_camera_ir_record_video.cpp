/**
 * @file test_camera_ir_record_video.cpp
 * @brief Example-faithful IR recording tests.
 */

#include "camera_ir_test_helpers.h"

namespace cit = camera_ir_test;

class CameraIrRecordVideoTest : public cit::CameraIrTest {};

TEST_F(CameraIrRecordVideoTest, ExampleFlow_StartTimeAdvancesThenStop) {
    double availableMb = -1;
    ASSERT_TRUE(cit::checkStorageReady(availableMb, 10.0, 4000))
        << "IR recording storage is not ready.";
    ASSERT_TRUE(cit::setCameraModeStateOrAck(CAMERA_MODE_VIDEO));

    cit::CaptureStatus idle;
    ASSERT_TRUE(cit::readCaptureStatus(idle, 3000));
    ASSERT_EQ(idle.video, 0);

    g_payload->setPayloadCameraRecordVideoStart();
    ASSERT_TRUE(cit::waitForRecording(true, 8000))
        << "IR video_status never became active.";
    cit::CaptureStatus first;
    ASSERT_TRUE(cit::waitForRecording(true, 3000, &first));

    std::this_thread::sleep_for(std::chrono::seconds(2));
    cit::CaptureStatus later;
    ASSERT_TRUE(cit::waitForRecording(true, 3000, &later));
    EXPECT_GT(later.recordingMs, first.recordingMs);
    std::this_thread::sleep_for(std::chrono::seconds(10));

    g_payload->setPayloadCameraRecordVideoStop();
    EXPECT_TRUE(cit::waitForRecording(false, 2000))
        << "IR video_status did not return to idle.";
}

TEST_F(CameraIrRecordVideoTest, StopWhileIdle_RemainsIdle) {
    ASSERT_TRUE(cit::stopRecording());
    g_payload->setPayloadCameraRecordVideoStop();
    EXPECT_TRUE(cit::waitForRecording(false, 4000));
}
