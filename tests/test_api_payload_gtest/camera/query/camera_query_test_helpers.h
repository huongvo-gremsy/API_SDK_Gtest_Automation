#ifndef PAYLOADSDK_TEST_CAMERA_QUERY_TEST_HELPERS_H_
#define PAYLOADSDK_TEST_CAMERA_QUERY_TEST_HELPERS_H_

#include "../../common/payload_test_fixture.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

// Camera query/state helpers live with the camera-query module. The common
// fixture only captures MAVLink callbacks and exposes synchronization counters.

inline bool getCameraMode(double& outMode, int timeoutMs = -1) {
    const uint64_t seq = g_cb.cameraSettingsSeq.load();
    g_payload->getPayloadCameraMode();
    if (!waitForSeq(g_cb.cameraSettingsSeq, seq, timeoutMs)) return false;
    std::lock_guard<std::mutex> lock(g_cb.m);
    outMode = g_cb.cameraSettings[0];
    return true;
}

inline bool setAndVerifyCameraMode(CAMERA_MODE mode, int timeoutMs = 5000,
                                   int retryIntervalMs = 500) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->setPayloadCameraMode(mode);
        double current = -1;
        if (getCameraMode(current, retryIntervalMs) &&
            static_cast<int>(current) == static_cast<int>(mode)) {
            return true;
        }
    }
    return false;
}

inline bool getCaptureStatus(double& imageStatus, double& videoStatus,
                             double& imageCount, double& recordingTimeMs,
                             int timeoutMs = -1) {
    const uint64_t seq = g_cb.cameraCaptureStatusSeq.load();
    g_payload->getPayloadCaptureStatus();
    if (!waitForSeq(g_cb.cameraCaptureStatusSeq, seq, timeoutMs)) return false;

    std::lock_guard<std::mutex> lock(g_cb.m);
    imageStatus = g_cb.cameraCaptureStatus[0];
    videoStatus = g_cb.cameraCaptureStatus[1];
    imageCount = g_cb.cameraCaptureStatus[2];
    recordingTimeMs = g_cb.cameraCaptureStatus[3];
    std::cout << "[INFO] Capture status: imageStatus=" << imageStatus
              << " videoStatus=" << videoStatus
              << " imageCount=" << imageCount
              << " recordingTimeMs=" << recordingTimeMs << std::endl;
    return true;
}

inline bool checkStorageReady(double& outAvailableMb,
                              double minAvailableMb = 10.0,
                              int timeoutMs = -1) {
    const uint64_t seq = g_cb.cameraStorageInfoSeq.load();
    g_payload->getPayloadStorage();
    if (!waitForSeq(g_cb.cameraStorageInfoSeq, seq, timeoutMs)) {
        outAvailableMb = -1;
        return false;
    }
    std::lock_guard<std::mutex> lock(g_cb.m);
    outAvailableMb = g_cb.cameraStorageInfo[2];
    return outAvailableMb >= minAvailableMb;
}

inline bool getStorageInfo(double& totalCapacity, double& usedCapacity,
                           double& availableCapacity, double& status,
                           int timeoutMs = -1) {
    if (timeoutMs < 0) timeoutMs = g_timeoutMs;
    const uint64_t seq = g_cb.cameraStorageInfoSeq.load();
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);

    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->getPayloadStorage();
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now()).count();
        if (remaining <= 0) break;
        if (waitForSeq(g_cb.cameraStorageInfoSeq, seq,
                       static_cast<int>(std::min<int64_t>(500, remaining)))) {
            std::lock_guard<std::mutex> lock(g_cb.m);
            totalCapacity = g_cb.cameraStorageInfo[0];
            usedCapacity = g_cb.cameraStorageInfo[1];
            availableCapacity = g_cb.cameraStorageInfo[2];
            status = g_cb.cameraStorageInfo[3];
            return true;
        }
    }
    return false;
}

inline bool getCameraFov(camera_type_t cameraType, double& outCameraId,
                         double& outHfov, double& outVfov,
                         int timeoutMs = -1) {
    if (timeoutMs < 0) timeoutMs = g_timeoutMs;
    const uint64_t seq = g_cb.cameraFovSeq.load();
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);

    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->getPayloadCameraFOVStatus(cameraType);
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now()).count();
        if (remaining <= 0) break;
        if (waitForSeq(g_cb.cameraFovSeq, seq,
                       static_cast<int>(std::min<int64_t>(500, remaining)))) {
            std::lock_guard<std::mutex> lock(g_cb.m);
            outCameraId = g_cb.cameraFov[0];
            outHfov = g_cb.cameraFov[1];
            outVfov = g_cb.cameraFov[2];
            return true;
        }
    }
    return false;
}

inline bool getCameraInformation(uint32_t& outFlags, int timeoutMs = -1) {
    const uint64_t seq = g_cb.cameraInfoSeq.load();
    g_payload->getPayloadCameraInformation();
    if (!waitForSeq(g_cb.cameraInfoSeq, seq, timeoutMs)) return false;

    std::lock_guard<std::mutex> lock(g_cb.m);
    outFlags = g_cb.cameraInfoFlags;
    return true;
}

#endif
