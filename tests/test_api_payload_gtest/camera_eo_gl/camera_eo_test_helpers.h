/**
 * @file camera_eo_test_helpers.h
 * @brief Small helpers shared by the Camera EO tests.

 */
#ifndef CAMERA_EO_TEST_HELPERS_H_
#define CAMERA_EO_TEST_HELPERS_H_

#include "../common/payload_test_fixture.h"

#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace eo {

// ---------------------------------------------------------------------------
// Product support (same columns as the QA spreadsheet: VIO ORUSL MB1 ZIO)
//   'x' = supported, '?' = not confirmed yet, '-' = not available
// ---------------------------------------------------------------------------
inline char supportOnThisProduct(const char* columns) {
#if defined(VIO)
    return columns[0];
#elif defined(ORUSL)
    return columns[1];
#elif defined(MB1)
    return columns[2];
#elif defined(ZIO)
    return columns[3];
#else
    (void)columns;
    return '-';
#endif
}

// '?' rows only run when PAYLOAD_TEST_UNVERIFIED=1 is set in the environment.
inline bool runUnverified() {
    const char* v = std::getenv("PAYLOAD_TEST_UNVERIFIED");
    return v && v[0] == '1';
}

inline void sleepMs(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

// ---------------------------------------------------------------------------
// COMMAND_ACK helpers
// ---------------------------------------------------------------------------
// Sends a command and waits for its COMMAND_ACK. Returns true when the payload
// answered ACCEPTED (or IN_PROGRESS). `what` is only used in the messages.
// Input:
// - command: command cần kiểm tra ACK
// - send: function dùng để gửi command/API
// - what: tên mô tả để in log
// - timeoutMs: thời gian tối đa chờ ACK, mặc định 3000 ms
// Output:
// - true: ACK = ACCEPTED hoặc IN_PROGRESS
// - false: không nhận được ACK hoặc ACK bị reject
inline bool sendAndWaitAck(uint16_t command, const std::function<void()>& send,
                           const std::string& what, int timeoutMs = 3000) {
    const uint64_t seq = getCommandAckSeq(command);
    send();
    AckInfo ack;
    if (!waitForCommandAck(command, seq, ack, timeoutMs)) {
        std::cout << "[  INFO  ] " << what << ": no COMMAND_ACK within " << timeoutMs << " ms\n";
        return false;
    }
    const bool ok = ack.result == MAV_RESULT_ACCEPTED || ack.result == MAV_RESULT_IN_PROGRESS;
    if (!ok) std::cout << "[  INFO  ] " << what << ": ACK result=" << int(ack.result) << "\n";
    return ok;
}

// Same as above but as a gtest expectation.
#define EO_EXPECT_ACCEPTED(command, sendCall, what) \
    EXPECT_TRUE(eo::sendAndWaitAck((command), [&] { sendCall; }, (what))) << (what) << " was not accepted"

// Skip the current test unless the spreadsheet marks it 'x' for this product.
#define EO_SKIP_UNLESS_SUPPORTED(columns)                                                        \
    do {                                                                                         \
        const char s = eo::supportOnThisProduct(columns);                                        \
        if (s == '-') GTEST_SKIP() << "not available on this product (sheet: " columns ")";      \
        if (s == '?' && !eo::runUnverified())                                                    \
            GTEST_SKIP() << "not confirmed on this product (sheet: " columns                      \
                         "); set PAYLOAD_TEST_UNVERIFIED=1 to run it";                           \
    } while (0)

// ---------------------------------------------------------------------------
// Camera parameters (C_SOURCE, C_V_FLIP, C_G_ISO, ...)
// ---------------------------------------------------------------------------
inline bool readParam(const char* id, double& out, int timeoutMs = 3000) {
    uint64_t seq = 0;
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        auto it = g_cb.paramSeqById.find(id);
        if (it != g_cb.paramSeqById.end()) seq = it->second;
    }
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->getPayloadCameraSettingByID(const_cast<char*>(id));
        for (int i = 0; i < 25; ++i) {      // wait up to 500 ms, then ask again
            {
                std::lock_guard<std::mutex> lock(g_cb.m);
                auto it = g_cb.paramSeqById.find(id);
                if (it != g_cb.paramSeqById.end() && it->second > seq) {
                    out = g_cb.paramValueById[id];
                    return true;
                }
            }
            sleepMs(20);
        }
    }
    return false;
}

// setPayloadCameraParam() and read the value back until it matches.
inline bool setParam(const char* id, uint32_t value, int timeoutMs = 5000) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->setPayloadCameraParam(const_cast<char*>(id), value, PARAM_TYPE_UINT32);
        double current = -1;
        if (readParam(id, current, 800) && current == value) return true;
    }
    return false;
}

// Reads the record source C_V_REC as the payload reports it.
// Output: 0 = both EO and IR, 1 = EO, 2 = IR, 5 = OSD. Returns false when
// the payload does not answer within `timeoutMs`.
inline bool readRecordSource(double& out, int timeoutMs = 3000) {
    return readParam(PAYLOAD_CAMERA_RECORD_SRC, out, timeoutMs);
}

// Live record source REC_SRC: the source the recorder is really using now.
// C_V_REC is only the stored setting. On VIO the two were seen to differ when
// C_V_REC changes right after C_SOURCE, and the file on the card then follows
// REC_SRC, not C_V_REC. Read like EO_ZOOM: requestParamValue() by index.
inline bool readLiveRecordSource(double& out, int timeoutMs = 2000) {
    const uint16_t index = PARAM_CAM_REC_SOURCE;
    uint64_t seq = 0;
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        auto it = g_cb.payloadParamSeqByIndex.find(index);
        if (it != g_cb.payloadParamSeqByIndex.end()) seq = it->second;
    }
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->requestParamValue(static_cast<uint8_t>(index));
        for (int i = 0; i < 25; ++i) {
            {
                std::lock_guard<std::mutex> lock(g_cb.m);
                auto it = g_cb.payloadParamSeqByIndex.find(index);
                if (it != g_cb.payloadParamSeqByIndex.end() && it->second > seq) {
                    out = g_cb.payloadParamValueByIndex[index];
                    return true;
                }
            }
            sleepMs(20);
        }
    }
    return false;
}

// Waits until the live REC_SRC equals `wanted`. When it does not, the recorder
// is frozen: on VIO this happens after a VIDEO_STOP_CAPTURE sent while idle and
// only a real recording (or the REC button) clears it. Best effort: prints what
// happened and never fails the test by itself.
inline bool applyRecordSource(uint32_t wanted, int timeoutMs = 3000) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    double live = -1;
    bool answered = false;
    while (std::chrono::steady_clock::now() < deadline) {
        answered = readLiveRecordSource(live, 800);
        if (answered && live == wanted) return true;
        sleepMs(300);
    }
    if (!answered) {
        std::cout << "[  INFO  ] REC_SRC not answered, record source not verified\n";
        return false;
    }
    // A C_V_REC change that arrives while the recorder is busy (right after a
    // stop, or right after a C_SOURCE change) is dropped, and a set with the same
    // value is ignored. So switch to another source and back once.
    std::cout << "[  INFO  ] REC_SRC still " << live << " after " << timeoutMs << " ms (wanted " << wanted
              << "), sending C_V_REC once more\n";
    const uint32_t other = (wanted == PAYLOAD_CAMERA_RECORD_EO) ? PAYLOAD_CAMERA_RECORD_IR : PAYLOAD_CAMERA_RECORD_EO;
    setParam(PAYLOAD_CAMERA_RECORD_SRC, other);
    setParam(PAYLOAD_CAMERA_RECORD_SRC, wanted);
    const auto retry = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < retry) {
        if (readLiveRecordSource(live, 800) && live == wanted) return true;
        sleepMs(300);
    }
    std::cout << "[  INFO  ] REC_SRC is " << live << " but C_V_REC=" << wanted
              << ": recorder frozen (VIDEO_STOP sent while idle?), record once or press REC to clear\n";
    return false;
}

// ---------------------------------------------------------------------------
// Capture status (image_status, video_status, image_count, recording_time_ms)
// ---------------------------------------------------------------------------
struct CaptureStatus {
    double image = 0;      // 0 idle, 1 capturing, 2 time-lapse
    double video = 0;      // 0 idle, 1 recording
    double count = 0;      // number of images taken
    double recordMs = 0;   // elapsed recording time
};

// Like the Gremsy examples, the request is sent again every 500 ms until the
// payload answers or `timeoutMs` passes, because this payload drops requests.
inline bool readCaptureStatus(CaptureStatus& out, int timeoutMs = 3000) {
    const uint64_t seq = g_cb.cameraCaptureStatusSeq.load();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    bool answered = false;
    while (!answered && std::chrono::steady_clock::now() < deadline) {
        g_payload->getPayloadCaptureStatus();
        answered = waitForSeq(g_cb.cameraCaptureStatusSeq, seq, 500);
    }
    if (!answered) return false;
    std::lock_guard<std::mutex> lock(g_cb.m);
    out.image = g_cb.cameraCaptureStatus[0];
    out.video = g_cb.cameraCaptureStatus[1];
    out.count = g_cb.cameraCaptureStatus[2];
    out.recordMs = g_cb.cameraCaptureStatus[3];
    return true;
}

// Polls the capture status until `condition` is true. `last` receives the final sample.
inline bool waitCaptureStatus(const std::function<bool(const CaptureStatus&)>& condition,
                              int timeoutMs, CaptureStatus* last = nullptr) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        CaptureStatus s;
        if (readCaptureStatus(s, 1000)) {
            if (last) *last = s;
            if (condition(s)) return true;
        }
        sleepMs(400);
    }
    return false;
}

// Stops image capture / video recording only when they are running, then waits
// until both are idle. The stop commands are never sent blindly: on VIO a
// VIDEO_STOP_CAPTURE received while idle freezes the record source (REC_SRC no
// longer follows C_V_REC until a real recording is made), so an idle camera is
// left alone. When the capture status is not answered there is nothing to
// stop and nothing to check, so it only prints a note and returns true.
inline bool stopAllCapture(int timeoutMs = 8000) {
    CaptureStatus s;
    if (!readCaptureStatus(s)) {
        std::cout << "[  INFO  ] stopAllCapture: capture status not answered, nothing sent\n";
        return true;
    }
    if (s.image == 0 && s.video == 0) return true;
    if (s.image != 0) g_payload->setPayloadCameraStopImage();
    if (s.video != 0) g_payload->setPayloadCameraRecordVideoStop();
    return waitCaptureStatus([](const CaptureStatus& s) { return s.image == 0 && s.video == 0; }, timeoutMs);
}

// ---------------------------------------------------------------------------
// Storage and camera mode
// ---------------------------------------------------------------------------
inline bool readStorageAvailableMb(double& out, int timeoutMs = 3000) {
    const uint64_t seq = g_cb.cameraStorageInfoSeq.load();
    g_payload->getPayloadStorage();
    if (!waitForSeq(g_cb.cameraStorageInfoSeq, seq, timeoutMs)) return false;
    std::lock_guard<std::mutex> lock(g_cb.m);
    out = g_cb.cameraStorageInfo[2];
    return true;
}

// True when the payload reports a ready storage (STORAGE_INFORMATION status 2 = READY).
// Without storage the payload cannot capture or record, and often does not
// answer CAMERA_CAPTURE_STATUS at all.
// The request is sent up to 3 times, because this payload drops a reply now and then.
inline bool hasStorage(int timeoutMs = 3000) {
    for (int attempt = 0; attempt < 3; attempt++) {
        const uint64_t seq = g_cb.cameraStorageInfoSeq.load();
        g_payload->getPayloadStorage();
        if (waitForSeq(g_cb.cameraStorageInfoSeq, seq, timeoutMs)) {
            std::lock_guard<std::mutex> lock(g_cb.m);
            const double totalMb = g_cb.cameraStorageInfo[0];
            const double status = g_cb.cameraStorageInfo[3];
            return totalMb > 0 && status == 2;
        }
    }
    std::cout << "[  INFO  ] getPayloadStorage(): no STORAGE_INFORMATION reply in 3 tries\n";
    return false;
}

// Skip the current test when the payload has no storage (no capture, no record).
#define EO_SKIP_WITHOUT_STORAGE()                                                                \
    do {                                                                                         \
        if (!eo::hasStorage())                                                                   \
            GTEST_SKIP() << "payload has no ready storage, capture/record is not possible";      \
    } while (0)

// CAMERA_SETTINGS: [0] mode, [1] zoom level, [2] focus level
inline bool readCameraSettings(double& mode, double& zoomLevel, double& focusLevel, int timeoutMs = 2000) {
    const uint64_t seq = g_cb.cameraSettingsSeq.load();
    g_payload->getPayloadCameraMode();
    if (!waitForSeq(g_cb.cameraSettingsSeq, seq, timeoutMs)) return false;
    std::lock_guard<std::mutex> lock(g_cb.m);
    mode = g_cb.cameraSettings[0];
    zoomLevel = g_cb.cameraSettings[1];
    focusLevel = g_cb.cameraSettings[2];
    return true;
}

// Sends the camera mode and reports whether the payload confirmed it.
// Some firmware (MB1) never reports the mode back, so callers treat this as
// best effort and do not fail on it.
inline bool setCameraMode(CAMERA_MODE mode, int timeoutMs = 4000) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->setPayloadCameraMode(mode);
        double m = -1, z = 0, f = 0;
        if (readCameraSettings(m, z, f, 800) && int(m) == int(mode)) return true;
    }
    std::cout << "[  INFO  ] camera mode " << int(mode) << " was not reported back by the payload\n";
    return false;
}

// ---------------------------------------------------------------------------
// Zoom level (payload param EO_ZOOM / IR_ZOOM, the same value the examples print)
// ---------------------------------------------------------------------------
inline bool readZoomLevel(camera_type_t camera, double& out, int timeoutMs = 2000) {
    const uint16_t index = (camera == CAMERA_EO) ? PARAM_EO_ZOOM_LEVEL : PARAM_IR_ZOOM_LEVEL;
    uint64_t seq = 0;
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        auto it = g_cb.payloadParamSeqByIndex.find(index);
        if (it != g_cb.payloadParamSeqByIndex.end()) seq = it->second;
    }
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->requestParamValue(static_cast<uint8_t>(index));
        for (int i = 0; i < 25; ++i) {
            {
                std::lock_guard<std::mutex> lock(g_cb.m);
                auto it = g_cb.payloadParamSeqByIndex.find(index);
                if (it != g_cb.payloadParamSeqByIndex.end() && it->second > seq) {
                    out = g_cb.payloadParamValueByIndex[index];
                    return true;
                }
            }
            sleepMs(20);
        }
    }
    return false;
}

// Polls the zoom level until `condition(level)` is true.
inline bool waitZoomLevel(camera_type_t camera, const std::function<bool(double)>& condition,
                          int timeoutMs, double* last = nullptr) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        double z = 0;
        if (readZoomLevel(camera, z, 1200)) {
            if (last) *last = z;
            if (condition(z)) return true;
        }
        sleepMs(250);
    }
    return false;
}

inline bool waitZoomNear(camera_type_t camera, double target, double tolerance, int timeoutMs, double* last = nullptr) {
    return waitZoomLevel(camera, [&](double z) { return z >= target - tolerance && z <= target + tolerance; }, timeoutMs, last);
}

inline bool waitZoomIncreased(camera_type_t camera, double from, int timeoutMs, double* last = nullptr) {
    return waitZoomLevel(camera, [&](double z) { return z > from + 0.1; }, timeoutMs, last);
}

inline bool waitZoomDecreased(camera_type_t camera, double from, int timeoutMs, double* last = nullptr) {
    return waitZoomLevel(camera, [&](double z) { return z < from - 0.1; }, timeoutMs, last);
}

// ---------------------------------------------------------------------------
// Test fixture: every EO test starts with EO as the active view/record source
// and leaves the camera idle with the original settings restored.
// ---------------------------------------------------------------------------
class EoCameraTest : public PayloadTest {
protected:
    void SetUp() override {
        // The camera must be idle. A payload without storage does not answer
        // CAMERA_CAPTURE_STATUS at all, so the idle check only runs when it answers.
        CaptureStatus status;
        captureStatusAvailable_ = readCaptureStatus(status, 3000);
        if (captureStatusAvailable_) {
            ASSERT_TRUE(stopAllCapture()) << "camera is busy: image/video capture could not be stopped";
        } else {
            std::cout << "[  INFO  ] getPayloadCaptureStatus(): no reply in 3 s (payload busy or no storage), idle check skipped\n";
        }

        saveAndSet(PAYLOAD_CAMERA_VIEW_SRC, PAYLOAD_CAMERA_VIEW_EO);
#ifndef ZIO
        saveAndSet(PAYLOAD_CAMERA_RECORD_SRC, PAYLOAD_CAMERA_RECORD_EO);
        applyRecordSource(PAYLOAD_CAMERA_RECORD_EO);
#endif
        sleepMs(500);
    }

    void TearDown() override {
        g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_STOP);
        g_payload->setCameraFocus(FOCUS_TYPE_CONTINUOUS, FOCUS_STOP);
        if (captureStatusAvailable_ && !stopAllCapture())
            ADD_FAILURE() << "cleanup: could not stop image/video capture";

        // restore in reverse order, so the last change is undone first
        for (auto it = saved_.rbegin(); it != saved_.rend(); ++it) {
            if (!setParam(it->first.c_str(), static_cast<uint32_t>(it->second)))
                ADD_FAILURE() << "cleanup: could not restore " << it->first << " = " << it->second;
        }
    }

    // Remembers the current value of `id` so TearDown() puts it back.
    // Returns false when the parameter cannot be read on this payload.
    bool restoreLater(const char* id) {
        double value = 0;
        if (!readParam(id, value)) return false;
        saved_.push_back({id, value});
        return true;
    }

    // restoreLater() + setParam(); fails the test when either step fails.
    void saveAndSet(const char* id, uint32_t value) {
        ASSERT_TRUE(restoreLater(id)) << id << " is not readable";
        ASSERT_TRUE(setParam(id, value)) << "could not set " << id << " = " << value;
    }

private:
    std::vector<std::pair<std::string, double>> saved_;
    bool captureStatusAvailable_ = false;
};

}  // namespace eo

#endif  // CAMERA_EO_TEST_HELPERS_H_
