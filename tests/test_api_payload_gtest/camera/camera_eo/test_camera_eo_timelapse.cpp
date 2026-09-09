/**
 * @file test_camera_eo_timelapse.cpp
 * @brief EO interval-capture flow based on
 *        examples/camera_time_lapse_photography.cpp.
 */

#include "camera_eo_test_helpers.h"

#include <chrono>
#include <cstdint>
#include <thread>

namespace cet = camera_eo_test;

namespace {

constexpr float kCaptureIntervalSeconds = 2.0F;

bool waitForImageStatus(bool active, int timeoutMs,
                        cet::CaptureStatus* output = nullptr) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        cet::CaptureStatus current;
        if (cet::readCaptureStatus(current, 1000) &&
            ((current.image != 0) == active)) {
            if (output != nullptr) *output = current;
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    return false;
}

}  // namespace

class CameraEoTimelapseTest : public cet::CameraEoTest {};
/*
It verifies:
- Storage availability and idle capture state
- Switching to image mode
- Starting a 2-second interval capture
- Accepted/in-progress ACK when provided
- At least two image_count increments
- Explicit stop and return to idle
- Automatic source/mode restoration through the existing fixture
*/
TEST_F(CameraEoTimelapseTest,
       ExampleFlow_IntervalCaptureProducesMultipleImagesThenStops) {
    double availableMb = -1;
    ASSERT_TRUE(cet::checkStorageReady(availableMb, 10.0, 4000))
        << "EO timelapse storage is not ready; available=" << availableMb
        << " MB.";

    cet::CaptureStatus before;
    ASSERT_TRUE(cet::readCaptureStatus(before, 3000));
    ASSERT_EQ(before.video, 0) << "Recording must be idle before timelapse.";
    ASSERT_EQ(before.image, 0) << "Image capture must be idle before timelapse.";

    ASSERT_TRUE(cet::setCameraModeStateOrAck(CAMERA_MODE_IMAGE));
    // The interval-capture example defaults to IR; this EO test explicitly
    // selects EO so the test exercises the EO capability.
    ASSERT_TRUE(cet::setUint32Param(PAYLOAD_CAMERA_VIEW_SRC,
                                    PAYLOAD_CAMERA_VIEW_EO));
    std::this_thread::sleep_for(std::chrono::seconds(1));

    const uint64_t ackSeq = getCommandAckSeq(MAV_CMD_IMAGE_START_CAPTURE);
    g_payload->setPayloadCameraCaptureImage(kCaptureIntervalSeconds);

    AckInfo ack;
    const bool receivedAck = waitForCommandAck(
        MAV_CMD_IMAGE_START_CAPTURE, ackSeq, ack, 2500);
    if (receivedAck) {
        ASSERT_TRUE(ack.result == MAV_RESULT_ACCEPTED ||
                    ack.result == MAV_RESULT_IN_PROGRESS)
            << "Payload rejected EO timelapse; ACK result="
            << static_cast<int>(ack.result);
    }

    // The command starts an indefinite interval capture because the SDK leaves
    // the MAVLink image-count field at zero. Observe two completed images before
    // stopping it explicitly.
    const double expectedCount = before.count + 2;
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::seconds(20);
    bool sawBusy = false;
    cet::CaptureStatus last = before;
    while (std::chrono::steady_clock::now() < deadline &&
           last.count < expectedCount) {
        if (cet::readCaptureStatus(last, 1500)) {
            sawBusy = sawBusy || last.image != 0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    g_payload->setPayloadCameraStopImage();
    const bool returnedIdle = waitForImageStatus(false, 8000, &last);

    EXPECT_GE(last.count, expectedCount)
        << "EO timelapse did not produce two images. baseline=" << before.count
        << ", final=" << last.count << ", sawBusy=" << sawBusy
        << ", receivedAck=" << receivedAck;
    EXPECT_TRUE(returnedIdle)
        << "Image status did not return to idle after stopping timelapse.";
}
