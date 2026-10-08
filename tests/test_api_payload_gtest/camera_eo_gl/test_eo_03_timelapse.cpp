/**
 * Sheet row 3: Chup time-lapse / dung chup - setPayloadCameraCaptureImage(interval) + setPayloadCameraStopImage()
 * Support: VIO x | ORUSL x | MB1 x | ZIO x
 * Example: examples/camera_time_lapse_photography.cpp
 *
 * Limitation: the SDK does not deliver CAMERA_IMAGE_CAPTURED and the capture
 * status has no per-image timestamp. The only signal is image_count read by
 * polling getPayloadCaptureStatus(). The interval is therefore verified by
 * counting images over a time window measured on the test machine, with
 * tolerance, not from per-image timestamps.
 */

#include "camera_eo_test_helpers.h"

using namespace eo;

// Time-lapse needs storage; without it the whole suite is skipped.
class EO_TimeLapse : public EoCameraTest {
protected:
    void SetUp() override {
        EO_SKIP_WITHOUT_STORAGE();

        EoCameraTest::SetUp();
    }
};

// Milliseconds elapsed since `start` (measured on the test machine).
static double elapsedMs(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - start).count();
}

// Check that time-lapse captures periodically at the configured interval, keeps
// capturing while running, and stops capturing after setPayloadCameraStopImage().
TEST_F(EO_TimeLapse, IntervalCaptureRunsUntilStopped) {
    const float intervalSec = 2.0f;         // configured interval between two images
    const int intervalMs = 2000;
    const int observationMs = 10000;        // observation window = 5 intervals
    const int expectedImages = observationMs / intervalMs;   // 5
    const int imageTolerance = 1;           // hardware test: allow +-1 image
    const double intervalToleranceSec = 0.6; // and +-0.6 s on the measured interval

    // Best effort: MB1 never reports the mode back, so the result is only printed.
    bool modeConfirmed = setCameraMode(CAMERA_MODE_IMAGE);

    if (modeConfirmed) {
        std::cout << "[  INFO  ] setPayloadCameraMode(IMAGE): confirmed by the payload\n";
    } else {
        std::cout << "[  INFO  ] setPayloadCameraMode(IMAGE): sent, not confirmed (best effort)\n";
    }

    // 1. Initial image count, camera must be idle.
    CaptureStatus before;

    ASSERT_TRUE(readCaptureStatus(before))
        << "getPayloadCaptureStatus(): no reply";

    ASSERT_EQ(before.image, 0)
        << "image capture already in progress";

    std::cout << "[  INFO  ] before time-lapse: image_status=" << before.image
              << " image_count=" << before.count << "\n";

    // 2. Start time-lapse with interval = 2 s.
    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_IMAGE_START_CAPTURE,
            [intervalSec] {
                g_payload->setPayloadCameraCaptureImage(intervalSec);
            },
            "time-lapse start"
        )
    ) << "setPayloadCameraCaptureImage(" << intervalSec << ") was not accepted";

    std::cout << "[  INFO  ] setPayloadCameraCaptureImage(" << intervalSec << "): ACK accepted\n";

    // 3. Verify capture starts: the first image must arrive within a few intervals.
    CaptureStatus first;

    bool firstImage = waitCaptureStatus(
        [&](const CaptureStatus& status) {
            return status.count > before.count;
        },
        intervalMs * 3 + 5000,
        &first
    );

    ASSERT_TRUE(firstImage)
        << "time-lapse did not take the first image (before=" << before.count
        << " last=" << first.count << ")";

    std::chrono::steady_clock::time_point observationStart = std::chrono::steady_clock::now();

    std::cout << "[  INFO  ] first image taken: image_status=" << first.image
              << " image_count=" << first.count << ", observation window starts\n";

    // 4. Observe for 10 s and count the images taken in that window.
    //    Polling every 500 ms is much finer than the 2 s interval.
    CaptureStatus last = first;

    while (elapsedMs(observationStart) < observationMs) {
        sleepMs(500);

        CaptureStatus now;

        if (readCaptureStatus(now, 1000)) {
            last = now;
        }
    }

    double windowSec = elapsedMs(observationStart) / 1000.0;
    int imagesInWindow = static_cast<int>(last.count - first.count);

    std::cout << "[  INFO  ] observation: " << windowSec << " s, image_count " << first.count
              << " -> " << last.count << " (" << imagesInWindow << " images), image_status=" << last.image << "\n";

    EXPECT_GE(imagesInWindow, expectedImages - imageTolerance)
        << "too few images in " << windowSec << " s at " << intervalSec << " s interval: "
        << imagesInWindow << " (expected about " << expectedImages << ")";

    EXPECT_LE(imagesInWindow, expectedImages + imageTolerance)
        << "too many images in " << windowSec << " s at " << intervalSec << " s interval: "
        << imagesInWindow << " (expected about " << expectedImages << ")";

    // 5. Measured interval = window / images. Only an average, see the limitation above.
    if (imagesInWindow > 0) {
        double measuredIntervalSec = windowSec / imagesInWindow;

        std::cout << "[  INFO  ] measured average interval: " << measuredIntervalSec
                  << " s (configured " << intervalSec << " s)\n";

        EXPECT_NEAR(measuredIntervalSec, intervalSec, intervalToleranceSec)
            << "average interval is off by more than " << intervalToleranceSec << " s";
    }

    // 6. Verify time-lapse is still running: one more image within the next interval.
    CaptureStatus more;

    bool stillCapturing = waitCaptureStatus(
        [&](const CaptureStatus& status) {
            return status.count > last.count;
        },
        intervalMs * 2,
        &more
    );

    EXPECT_TRUE(stillCapturing)
        << "time-lapse stopped by itself: image_count stayed " << last.count
        << " for " << intervalMs * 2 << " ms";

    std::cout << "[  INFO  ] still running: image_count " << last.count << " -> " << more.count << "\n";

    // 7. Stop time-lapse.
    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_IMAGE_STOP_CAPTURE,
            [] {
                g_payload->setPayloadCameraStopImage();
            },
            "time-lapse stop"
        )
    ) << "setPayloadCameraStopImage() was not accepted";

    std::cout << "[  INFO  ] setPayloadCameraStopImage(): ACK accepted\n";

    CaptureStatus stopped;

    ASSERT_TRUE(waitCaptureStatus(
        [](const CaptureStatus& status) {
            return status.image == 0;
        },
        8000,
        &stopped
    )) << "image_status did not return to idle after setPayloadCameraStopImage()";

    std::cout << "[  INFO  ] after stop: image_status=" << stopped.image
              << " (0 = idle) image_count=" << stopped.count << "\n";

    // 8. Verify no more images after stop: wait 2 intervals, the count must not move.
    sleepMs(intervalMs * 2 + 1000);

    CaptureStatus after;

    ASSERT_TRUE(readCaptureStatus(after))
        << "getPayloadCaptureStatus(): no reply after stop";

    EXPECT_EQ(after.count, stopped.count)
        << "images are still being captured after stop: " << stopped.count << " -> " << after.count;

    std::cout << "[  INFO  ] image_count " << (intervalMs * 2 + 1000) << " ms after stop: "
              << stopped.count << " -> " << after.count << "\n";

    std::cout << "[  INFO  ] summary: before=" << before.count << " first=" << first.count
              << " end of window=" << last.count << " still running=" << more.count
              << " at stop=" << stopped.count << " after stop=" << after.count << "\n";
}
