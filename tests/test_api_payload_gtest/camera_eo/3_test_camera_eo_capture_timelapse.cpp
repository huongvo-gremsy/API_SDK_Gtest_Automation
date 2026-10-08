#include "api_camera_eo_test_helper.h"

#include <chrono>


namespace cet = camera_eo_test;

class CameraEoCaptureTimelapseTest : public cet::CameraEoTest {};
class ManualCameraEoCaptureTimelapseTest : public testing::Test {};

TEST_F(CameraEoCaptureTimelapseTest,
       SetPayloadCameraCaptureImageIntervalThenStopImage_StopsSequence) {
    constexpr float kIntervalSeconds = 2.0F;

    ASSERT_TRUE(cet::setCameraMode(CAMERA_MODE_IMAGE));
    cet::CaptureStatus before;
    ASSERT_TRUE(cet::readCaptureStatus(before));
    ASSERT_EQ(before.video, 0);
    ASSERT_EQ(before.image, 0);

    const uint64_t startAckSeq = getCommandAckSeq(MAV_CMD_IMAGE_START_CAPTURE);
    g_payload->setPayloadCameraCaptureImage(kIntervalSeconds);

    AckInfo startAck;
    if (waitForCommandAck(MAV_CMD_IMAGE_START_CAPTURE, startAckSeq,
                           startAck, 2500)) {
        EXPECT_TRUE(startAck.result == MAV_RESULT_ACCEPTED ||
                    startAck.result == MAV_RESULT_IN_PROGRESS);
    }

    cet::CaptureStatus active;
    ASSERT_TRUE(cet::waitForImageState(true, 8000, &active))
        << "Interval capture did not become active.";

    const auto captureDeadline = std::chrono::steady_clock::now() +
                                 std::chrono::seconds(10);
    cet::CaptureStatus later = active;
    while (std::chrono::steady_clock::now() < captureDeadline &&
           later.count <= before.count) {
        ASSERT_TRUE(cet::readCaptureStatus(later));
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    const uint64_t stopAckSeq = getCommandAckSeq(MAV_CMD_IMAGE_STOP_CAPTURE);
    g_payload->setPayloadCameraStopImage();
    ASSERT_TRUE(cet::waitForImageState(false))
        << "setPayloadCameraStopImage() did not return image status to idle.";

    AckInfo stopAck;
    if (waitForCommandAck(MAV_CMD_IMAGE_STOP_CAPTURE, stopAckSeq,
                           stopAck, 1500)) {
        EXPECT_TRUE(stopAck.result == MAV_RESULT_ACCEPTED ||
                    stopAck.result == MAV_RESULT_IN_PROGRESS);
    }

    EXPECT_GT(later.count, before.count)
        << "Interval capture did not produce an image before it was stopped.";
}
// Manual test for camera capture image interval then stop image
TEST_F(ManualCameraEoCaptureTimelapseTest, Manual_Camera_Capture_Image_Interval) {
    g_payload->setPayloadCameraCaptureImage(2.0F);
    std::this_thread::sleep_for(std::chrono::seconds(10));
    g_payload->setPayloadCameraStopImage();
    std::this_thread::sleep_for(std::chrono::seconds(2));
}
TEST_F(ManualCameraEoCaptureTimelapseTest, Manual_Camera_Capture_Image_Interval_Stop) {
    // g_payload->setPayloadCameraCaptureImage(2.0F);
    // std::this_thread::sleep_for(std::chrono::seconds(10));
    g_payload->setPayloadCameraStopImage();
    std::this_thread::sleep_for(std::chrono::seconds(2));
}
TEST_F(ManualCameraEoCaptureTimelapseTest, Manual_Get_Camera_Capture_Status) {
    cet::CaptureStatus status;

    const uint64_t seq = g_cb.cameraCaptureStatusSeq.load();
    g_payload->getPayloadCaptureStatus();
    if (!waitForSeq(g_cb.cameraCaptureStatusSeq, seq, 3000)) {
        return;
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
}