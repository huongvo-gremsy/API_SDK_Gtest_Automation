#ifndef API_CAMERA_EO_TEST_HELPER_H_
#define API_CAMERA_EO_TEST_HELPER_H_

#include "../common/payload_test_fixture.h"

#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

namespace camera_eo_test {

struct CaptureStatus {
    double image = 0;
    double video = 0;
    double count = 0;
    double recordingMs = 0;
};

struct FovStatus {
    double cameraId = 0;
    double horizontal = 0;
    double vertical = 0;
};

inline bool readCaptureStatus(CaptureStatus& status, int timeoutMs = 3000) {
    const uint64_t seq = g_cb.cameraCaptureStatusSeq.load();
    g_payload->getPayloadCaptureStatus();
    if (!waitForSeq(g_cb.cameraCaptureStatusSeq, seq, timeoutMs)) {
        return false;
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
    return true;
}

inline bool readCameraMode(double& mode, int timeoutMs = 3000) {
    const uint64_t seq = g_cb.cameraSettingsSeq.load();
    g_payload->getPayloadCameraMode();
    if (!waitForSeq(g_cb.cameraSettingsSeq, seq, timeoutMs)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_cb.m);
    mode = g_cb.cameraSettings[0];
    return true;
}

inline bool readCameraParam(const char* id, double& value, int timeoutMs = 3000) {
    uint64_t seq = 0;
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        const auto it = g_cb.paramSeqById.find(id);
        seq = it == g_cb.paramSeqById.end() ? 0 : it->second;
    }

    char mutableId[CAM_PARAM_ID_LEN] = {};
    std::strncpy(mutableId, id, sizeof(mutableId) - 1);
    g_payload->getPayloadCameraSettingByID(mutableId);

    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        {
            std::lock_guard<std::mutex> lock(g_cb.m);
            const auto it = g_cb.paramSeqById.find(id);
            if (it != g_cb.paramSeqById.end() && it->second > seq) {
                value = g_cb.paramValueById.at(id);
                // std::cout << "readCameraParam: id=" << id
                //           << ", value=" << std::fixed << std::setprecision(2)
                //           << value << std::endl;
                return true;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}

// inline bool readCameraParam(const char* id, double& value,
//                              int timeoutMs = 3000, bool verbose = false) {
//     uint64_t seq = 0;
//     {
//         std::lock_guard<std::mutex> lock(g_cb.m);
//         const auto it = g_cb.paramSeqById.find(id);
//         seq = it == g_cb.paramSeqById.end() ? 0 : it->second;
//     }

//     char mutableId[CAM_PARAM_ID_LEN] = {};
//     std::strncpy(mutableId, id, sizeof(mutableId) - 1);
//     g_payload->getPayloadCameraSettingByID(mutableId);

//     const auto deadline = std::chrono::steady_clock::now() +
//                           std::chrono::milliseconds(timeoutMs);
//     while (std::chrono::steady_clock::now() < deadline) {
//         {
//             std::lock_guard<std::mutex> lock(g_cb.m);
//             const auto it = g_cb.paramSeqById.find(id);
//             if (it != g_cb.paramSeqById.end() && it->second > seq) {
//                 value = g_cb.paramValueById.at(id);
//                 if (verbose) {
//                     std::cout << "--> Param_id: " << id
//                               << ", value: " << std::fixed
//                               << std::setprecision(2) << value << std::endl;
//                 }
//                 return true;
//             }
//         }
//         std::this_thread::sleep_for(std::chrono::milliseconds(20));
//     }
//     return false;
// }

inline bool setCameraParam(const char* id, uint32_t value, int timeoutMs = 4000) {
    char mutableId[CAM_PARAM_ID_LEN] = {};
    std::strncpy(mutableId, id, sizeof(mutableId) - 1);
    g_payload->setPayloadCameraParam(mutableId, value, PARAM_TYPE_UINT32);

    double actual = -1;
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        if (readCameraParam(id, actual, 1000) &&
            static_cast<uint32_t>(actual) == value) {
            return true;
        }
    }
    return false;
}

inline bool setCameraMode(CAMERA_MODE mode, int timeoutMs = 4000) {
    g_payload->setPayloadCameraMode(mode);
    double actual = -1;
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        if (readCameraMode(actual, 1000) &&
            static_cast<int>(actual) == static_cast<int>(mode)) {
            return true;
        }
    }

    const uint64_t seq = getCommandAckSeq(MAV_CMD_SET_CAMERA_MODE);
    g_payload->setPayloadCameraMode(mode);
    AckInfo ack;
    return waitForCommandAck(MAV_CMD_SET_CAMERA_MODE, seq, ack, 1500) &&
           (ack.result == MAV_RESULT_ACCEPTED ||
            ack.result == MAV_RESULT_IN_PROGRESS);
}

inline bool commandAcceptedOrInProgress(uint16_t command,
                                        const std::function<void()>& sendFn,
                                        int timeoutMs = 2500) {
    const uint64_t seq = getCommandAckSeq(command);
    sendFn();

    AckInfo ack;
    return waitForCommandAck(command, seq, ack, timeoutMs) &&
           (ack.result == MAV_RESULT_ACCEPTED ||
            ack.result == MAV_RESULT_IN_PROGRESS);
}

inline bool sendAndAcceptIfAcked(uint16_t command,
                                 const std::function<void()>& sendFn,
                                 int timeoutMs = 1500) {
    const uint64_t seq = getCommandAckSeq(command);
    sendFn();

    AckInfo ack;
    if (!waitForCommandAck(command, seq, ack, timeoutMs)) {
        return true;
    }
    return ack.result == MAV_RESULT_ACCEPTED ||
           ack.result == MAV_RESULT_IN_PROGRESS;
}

inline bool readFovStatus(camera_type_t cameraType, FovStatus& status,
                          int timeoutMs = 3000) {
    const uint64_t seq = g_cb.cameraFovSeq.load();
    g_payload->getPayloadCameraFOVStatus(cameraType);
    if (!waitForSeq(g_cb.cameraFovSeq, seq, timeoutMs)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_cb.m);
    status.cameraId = g_cb.cameraFov[0];
    status.horizontal = g_cb.cameraFov[1];
    status.vertical = g_cb.cameraFov[2];
    std::cout << "readFovStatus: cameraId=" << status.cameraId
              << ", hfov=" << status.horizontal
              << ", vfov=" << status.vertical
              << std::endl;
    return true;
}

inline bool waitForImageState(bool active, int timeoutMs = 8000,
                              CaptureStatus* output = nullptr) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        CaptureStatus current;
        if (readCaptureStatus(current, 1200) &&
            ((current.image != 0) == active)) {
            if (output) *output = current;
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
    return false;
}

inline bool waitForVideoState(bool active, int timeoutMs = 8000,
                              CaptureStatus* output = nullptr) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        CaptureStatus current;
        if (readCaptureStatus(current, 1200) &&
            ((current.video != 0) == active)) {
            if (output) *output = current;
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
    return false;
}

class CameraEoTest : public PayloadTest {
protected:
    void SetUp() override {
        ASSERT_TRUE(readCameraParam(PAYLOAD_CAMERA_VIEW_SRC, originalView_));
        haveView_ = true;
#ifndef ZIO
        ASSERT_TRUE(readCameraParam(PAYLOAD_CAMERA_RECORD_SRC, originalRecord_));
        haveRecord_ = true;
#endif
        if (readCameraMode(originalMode_)) {
            haveMode_ = true;
        }

        CaptureStatus status;
        ASSERT_TRUE(readCaptureStatus(status));
        if (status.video != 0) {
            g_payload->setPayloadCameraRecordVideoStop();
            ASSERT_TRUE(waitForVideoState(false));
        }
        if (status.image != 0) {
            g_payload->setPayloadCameraStopImage();
            ASSERT_TRUE(waitForImageState(false));
        }

        ASSERT_TRUE(setCameraParam(PAYLOAD_CAMERA_VIEW_SRC,
                                   PAYLOAD_CAMERA_VIEW_EO));
#ifndef ZIO
        ASSERT_TRUE(setCameraParam(PAYLOAD_CAMERA_RECORD_SRC,
                                   PAYLOAD_CAMERA_RECORD_EO));
#endif
    }

    void TearDown() override {
        g_payload->setPayloadCameraStopImage();
        g_payload->setPayloadCameraRecordVideoStop();
        waitForImageState(false, 5000);
        waitForVideoState(false, 5000);

#ifndef ZIO
        if (haveRecord_) {
            EXPECT_TRUE(setCameraParam(PAYLOAD_CAMERA_RECORD_SRC,
                                       static_cast<uint32_t>(originalRecord_)));
        }
#endif
        if (haveView_) {
            EXPECT_TRUE(setCameraParam(PAYLOAD_CAMERA_VIEW_SRC,
                                       static_cast<uint32_t>(originalView_)));
        }
        if (haveMode_) {
            EXPECT_TRUE(setCameraMode(static_cast<CAMERA_MODE>(
                static_cast<int>(originalMode_))));
        }
    }

    double originalView_ = 0;
    double originalRecord_ = 0;
    double originalMode_ = CAMERA_MODE_IMAGE;
    bool haveView_ = false;
    bool haveRecord_ = false;
    bool haveMode_ = false;
};

}  // namespace camera_eo_test

#endif

