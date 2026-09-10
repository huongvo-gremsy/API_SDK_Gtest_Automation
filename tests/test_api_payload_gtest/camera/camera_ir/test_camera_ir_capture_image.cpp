/**
 * @file test_camera_ir_capture_image.cpp
 * @brief Example-faithful IR image capture tests.
 */

#include "camera_ir_test_helpers.h"

namespace cit = camera_ir_test;

class CameraIrCaptureImageTest : public cit::CameraIrTest {};

TEST_F(CameraIrCaptureImageTest, ExampleFlow_CaptureIncrementsImageCount) {
    // Match camera_ir_capture_image.cpp: begin in VIDEO, verify prerequisites,
    // switch to IMAGE, then trigger an IR capture.
    ASSERT_TRUE(cit::setCameraModeStateOrAck(CAMERA_MODE_VIDEO));

    double availableMb = -1;
    ASSERT_TRUE(cit::checkStorageReady(availableMb, 10.0, 4000))
        << "IR capture storage is not ready; available=" << availableMb << " MB.";

    cit::CaptureStatus before;
    ASSERT_TRUE(cit::readCaptureStatus(before, 3000));
    ASSERT_EQ(before.video, 0) << "Recording must be idle before image capture.";

    double reportedMode = 0;
    ASSERT_TRUE(cit::getCameraMode(reportedMode, 3000));
    ASSERT_TRUE(cit::setCameraModeStateOrAck(CAMERA_MODE_IMAGE));

    // Let the IR pipeline settle after the mode change. In particular, an ACK
    // can arrive before CAMERA_SETTINGS and the capture pipeline are ready.
    std::this_thread::sleep_for(std::chrono::seconds(1));
    if (cit::getCameraMode(reportedMode, 2000)) {
        std::cout << "[INFO] Camera mode before capture: " << reportedMode
                  << " (IMAGE=" << static_cast<int>(CAMERA_MODE_IMAGE) << ")"
                  << std::endl;
        if (static_cast<int>(reportedMode) != CAMERA_MODE_IMAGE) {
            std::cout << "[WARN] Mode command was accepted, but CAMERA_SETTINGS "
                         "still reports a different mode."
                      << std::endl;
        }
    }

    // Match the SDK example: send START_CAPTURE exactly once, then only poll
    // status. Re-sending the command on every poll can restart/queue capture,
    // leaving image_count unchanged until the test is already tearing down.
    const uint64_t ackSeq = getCommandAckSeq(MAV_CMD_IMAGE_START_CAPTURE);
    g_payload->setPayloadCameraCaptureImage();
    AckInfo captureAck;
    const bool receivedAck = waitForCommandAck(
        MAV_CMD_IMAGE_START_CAPTURE, ackSeq, captureAck, 2500);
    if (receivedAck) {
        ASSERT_TRUE(captureAck.result == MAV_RESULT_ACCEPTED ||
                    captureAck.result == MAV_RESULT_IN_PROGRESS)
            << "Payload rejected IR capture; ACK result="
            << static_cast<int>(captureAck.result);
    } else {
        std::cout << "[WARN] No IMAGE_START_CAPTURE ACK; verifying through "
                     "capture status instead."
                  << std::endl;
    }

    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::seconds(20);
    bool increased = false;
    bool sawBusy = false;
    cit::CaptureStatus last = before;
    while (std::chrono::steady_clock::now() < deadline) {
        if (cit::readCaptureStatus(last, 1500)) {
            sawBusy = sawBusy || last.image != 0;
            if (last.count > before.count) {
                increased = true;
                // The SDK currently encodes image count (MAVLink param3) as
                // zero, so explicitly stop after observing the first image.
                g_payload->setPayloadCameraStopImage();
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    EXPECT_TRUE(increased)
        << "IR image_count did not increase after one capture command. "
        << "baseline=" << before.count << ", final=" << last.count
        << ", saw image_status busy=" << sawBusy
        << ", received ACK=" << receivedAck
        << (receivedAck ? ", ACK result=" : "")
        << (receivedAck ? std::to_string(static_cast<int>(captureAck.result))
                        : std::string(""));
}

TEST_F(CameraIrCaptureImageTest, IRViewAndRecordSource_ReadBack) {
    double view = 0, record = 0;
    ASSERT_TRUE(cit::getCameraSettingByID(PAYLOAD_CAMERA_VIEW_SRC, view));
    ASSERT_TRUE(cit::getCameraSettingByID(PAYLOAD_CAMERA_RECORD_SRC, record));
    EXPECT_EQ(static_cast<uint32_t>(view), PAYLOAD_CAMERA_VIEW_IR);
    EXPECT_EQ(static_cast<uint32_t>(record), PAYLOAD_CAMERA_RECORD_IR);
}
