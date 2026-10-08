#include "api_camera_eo_test_helper.h"

namespace cet = camera_eo_test;

class CameraEoCaptureImageTest : public cet::CameraEoTest {};

TEST_F(CameraEoCaptureImageTest,
       SetPayloadCameraCaptureImage_IncrementsImageCount) {
    ASSERT_TRUE(cet::setCameraMode(CAMERA_MODE_IMAGE));

    cet::CaptureStatus before;
    ASSERT_TRUE(cet::readCaptureStatus(before));
    ASSERT_EQ(before.video, 0);
    ASSERT_EQ(before.image, 0);

    const uint64_t ackSeq = getCommandAckSeq(MAV_CMD_IMAGE_START_CAPTURE);
    g_payload->setPayloadCameraCaptureImage();

    AckInfo ack;
    const bool gotAck = waitForCommandAck(
        MAV_CMD_IMAGE_START_CAPTURE, ackSeq, ack, 2500);
    if (gotAck) {
        EXPECT_TRUE(ack.result == MAV_RESULT_ACCEPTED ||
                    ack.result == MAV_RESULT_IN_PROGRESS);
    }

    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::seconds(20);
    cet::CaptureStatus last = before;
    while (std::chrono::steady_clock::now() < deadline &&
           last.count <= before.count) {
        ASSERT_TRUE(cet::readCaptureStatus(last, 1500));
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    g_payload->setPayloadCameraStopImage();
    ASSERT_TRUE(cet::waitForImageState(false));
    EXPECT_GT(last.count, before.count)
        << "image_count did not increase after setPayloadCameraCaptureImage().";
}
