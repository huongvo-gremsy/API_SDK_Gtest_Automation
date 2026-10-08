/**
 * Sheet row 1: Chup anh EO - setPayloadCameraCaptureImage()
 * Support: VIO x | ORUSL x | MB1 x | ZIO x
 * Example: examples/camera_eo_capture_image.cpp
 */

#include "camera_eo_test_helpers.h"

using namespace eo;

// Capture needs storage; without it the whole suite is skipped.
class EO_CaptureImage : public EoCameraTest {
protected:
    void SetUp() override {
        EO_SKIP_WITHOUT_STORAGE();

        EoCameraTest::SetUp();
    }
};

// Check that there is enough storage for taking a picture.
TEST_F(EO_CaptureImage, StorageHasFreeSpace) {
    double availableMb = 0;

    ASSERT_TRUE(readStorageAvailableMb(availableMb))
        << "getPayloadStorage(): no STORAGE_INFORMATION reply";

    std::cout << "[  INFO  ] storage available: "
              << availableMb << " MB\n";

    EXPECT_GE(availableMb, 10.0)
        << "less than 10 MB free, capture tests need space";
}

// Check that setPayloadCameraCaptureImage() really takes a picture.
TEST_F(EO_CaptureImage, SetPayloadCameraCaptureImage) {
    // Best effort: MB1 never reports the mode back, so the result is only printed.
    bool modeConfirmed = setCameraMode(CAMERA_MODE_IMAGE);

    if (modeConfirmed) {
        std::cout << "[  INFO  ] setPayloadCameraMode(IMAGE): confirmed by the payload\n";
    } else {
        std::cout << "[  INFO  ] setPayloadCameraMode(IMAGE): sent, not confirmed (best effort)\n";
    }

    CaptureStatus before;

    ASSERT_TRUE(readCaptureStatus(before))
        << "getPayloadCaptureStatus(): no reply";

    ASSERT_EQ(before.image, 0)
        << "image capture already in progress";

    ASSERT_EQ(before.video, 0)
        << "video recording in progress";

    std::cout << "[  INFO  ] before capture: image_status=" << before.image
              << " video_status=" << before.video
              << " image_count=" << before.count << "\n";

    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_IMAGE_START_CAPTURE,
            [] {
                g_payload->setPayloadCameraCaptureImage();
            },
            "capture image"
        )
    ) << "setPayloadCameraCaptureImage() was not accepted";

    std::cout << "[  INFO  ] setPayloadCameraCaptureImage(): ACK accepted\n";

    // Check that the number of images increased.
    CaptureStatus last;

    bool imageCountIncreased = waitCaptureStatus(
        [&](const CaptureStatus& status) {
            return status.count > before.count;
        },
        20000,
        &last
    );

    EXPECT_TRUE(imageCountIncreased)
        << "image_count did not increase: before="
        << before.count
        << " last="
        << last.count;

    std::cout << "[  INFO  ] image_count: before=" << before.count
              << " after=" << last.count << "\n";

    // Check that the camera finished capturing.
    bool imageIsIdle = waitCaptureStatus(
        [](const CaptureStatus& status) {
            return status.image == 0;
        },
        8000,
        &last
    );

    EXPECT_TRUE(imageIsIdle)
        << "image_status did not return to idle";

    std::cout << "[  INFO  ] image_status after capture: " << last.image << " (0 = idle)\n";
}

// // Check that stopping an idle camera does not cause a problem.
// TEST_F(EO_CaptureImage, SetPayloadCameraStopImage) {
//     CaptureStatus status;

//     ASSERT_TRUE(waitCaptureStatus(
//         [](const CaptureStatus& status) {
//             return status.image == 0;
//         },
//         5000
//     )) << "camera is not idle before the test";

//     g_payload->setPayloadCameraStopImage();

//     EXPECT_TRUE(waitCaptureStatus(
//         [](const CaptureStatus& status) {
//             return status.image == 0;
//         },
//         5000,
//         &status
//     )) << "camera is not idle after stop command";
// }
