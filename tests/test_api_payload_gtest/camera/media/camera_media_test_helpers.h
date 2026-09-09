#ifndef PAYLOADSDK_TEST_CAMERA_MEDIA_TEST_HELPERS_H_
#define PAYLOADSDK_TEST_CAMERA_MEDIA_TEST_HELPERS_H_

#include "../query/camera_query_test_helpers.h"

#include <chrono>
#include <thread>

inline bool captureImageAndVerify(int timeoutMs = 8000,
                                  int retryIntervalMs = 1000) {
    double imageBefore = 0;
    double videoBefore = 0;
    double countBefore = 0;
    double recordingBefore = 0;
    if (!getCaptureStatus(imageBefore, videoBefore, countBefore,
                          recordingBefore, retryIntervalMs)) {
        return false;
    }

    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->setPayloadCameraCaptureImage();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        double imageAfter = 0;
        double videoAfter = 0;
        double countAfter = 0;
        double recordingAfter = 0;
        if (getCaptureStatus(imageAfter, videoAfter, countAfter,
                             recordingAfter, retryIntervalMs) &&
            countAfter > countBefore) {
            return true;
        }
    }
    return false;
}

#endif
