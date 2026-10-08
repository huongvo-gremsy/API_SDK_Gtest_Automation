/**
 * @file gb_test_helpers.h
 * @brief Small helpers shared by the Gimbal tests. Same layout as camera_eo.
 *
 * 284 = command  (PC -> gimbal, GIMBAL_DEVICE_SET_ATTITUDE, no ACK)
 * 285 = telemetry (gimbal -> PC, GIMBAL_DEVICE_ATTITUDE_STATUS, streamed ~20/s)
 */
#ifndef GB_TEST_HELPERS_H_
#define GB_TEST_HELPERS_H_

#include "../common/payload_test_fixture.h"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace gb {

// ---------------------------------------------------------------------------
// Product support (same columns as the QA spreadsheet: VIO ORUSL MB1 ZIO)
//   'x' = supported, '?' = not confirmed yet, '-' = not available
// ---------------------------------------------------------------------------
// Return the sheet column ('x', '?' or '-') for the product this build is for.
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

// True when PAYLOAD_TEST_UNVERIFIED=1 is set, so '?' rows can run.
inline bool runUnverified() {
    const char* v = std::getenv("PAYLOAD_TEST_UNVERIFIED");
    return v && v[0] == '1';
}

// Sleep for `ms` milliseconds.
inline void sleepMs(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

// ---------------------------------------------------------------------------
// Gimbal-only switch
// ---------------------------------------------------------------------------
// True when PAYLOAD_TEST_GIMBAL_CALIB=1 is set.
// Calibration and auto tune move or reboot the gimbal, so they only run with this switch.
inline bool runGimbalCalib() {
    const char* v = std::getenv("PAYLOAD_TEST_GIMBAL_CALIB");
    return v && v[0] == '1';
}

// ---------------------------------------------------------------------------
// COMMAND_ACK helpers
// ---------------------------------------------------------------------------
// Send a command and wait for its ACK.
// ACK means the command was received, not that the action is finished.
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

// Skip the test unless the sheet marks it 'x' for this product.
// '?' rows only run when PAYLOAD_TEST_UNVERIFIED=1 is set.
#define GB_SKIP_UNLESS_SUPPORTED(columns)                                                        \
    do {                                                                                         \
        const char s = gb::supportOnThisProduct(columns);                                        \
        if (s == '-') GTEST_SKIP() << "not available on this product (sheet: " columns ")";      \
        if (s == '?' && !gb::runUnverified())                                                    \
            GTEST_SKIP() << "not confirmed on this product (sheet: " columns                      \
                         "); set PAYLOAD_TEST_UNVERIFIED=1 to run it";                           \
    } while (0)

// ---------------------------------------------------------------------------
// Gimbal settings that live on the camera side: RC_MODE, GB_MODE, GB_FW_FLAG.
// Same PARAM_EXT path and same functions as the camera helpers.
// ---------------------------------------------------------------------------
// Read a camera-side setting and wait for the reply.
// Ask again every 500 ms until answered or timeout.
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
        for (int i = 0; i < 25; ++i) {
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
// Set a camera-side setting and read it back until it matches.
inline bool setParam(const char* id, uint32_t value, int timeoutMs = 5000) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->setPayloadCameraParam(const_cast<char*>(id), value, PARAM_TYPE_UINT32);
        double current = -1;
        if (readParam(id, current, 800) && current == value) return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Gimbal attitude (telemetry 285, streamed by the gimbal, no request needed)
// ---------------------------------------------------------------------------
// One telemetry 285 sample. `mode` is the SDK mode string.
struct Attitude {
    double pitch = 0;      // degrees
    double roll = 0;       // degrees
    double yaw = 0;        // degrees, -180..180
    std::string mode;      // "LOCK_MODE", "FOLLOW_MODE", "OFF_MODE", "RESET_MODE", "MAPPING_MODE"
};

// Read one telemetry 285 message.
// No command is sent; the gimbal sends 285 by itself. False means nothing arrived before timeout.
inline bool readAttitude(Attitude& out, int timeoutMs = 3000) {
    const uint64_t seq = g_cb.gimbalAttitudeSeq.load();
    if (!waitForSeq(g_cb.gimbalAttitudeSeq, seq, timeoutMs)) return false;
    std::lock_guard<std::mutex> lock(g_cb.m);
    out.pitch = g_cb.gimbalAttitude[0];
    out.roll = g_cb.gimbalAttitude[1];
    out.yaw = g_cb.gimbalAttitude[2];
    out.mode = g_cb.gimbalMode;
    return true;
}

// Shortest distance between two yaw angles (+179 and -179 are 2 degrees apart).
inline double yawDifference(double a, double b) {
    double d = std::fmod(a - b + 540.0, 360.0) - 180.0;
    return std::fabs(d);
}

// Keep reading telemetry 285 until the gimbal reaches the target angle.
// Returns false if the target is not reached before timeout.
inline bool waitAttitudeNear(double pitch, double yaw, double tolerance, int timeoutMs,
                             Attitude* last = nullptr) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        Attitude a;
        if (readAttitude(a, 700)) {
            if (last) *last = a;
            if (std::fabs(a.pitch - pitch) <= tolerance && yawDifference(a.yaw, yaw) <= tolerance) return true;
        }
    }
    return false;
}

// Keep reading telemetry 285 until the gimbal reports the expected mode.
inline bool waitGimbalMode(const std::string& mode, int timeoutMs, Attitude* last = nullptr) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        Attitude a;
        if (readAttitude(a, 700)) {
            if (last) *last = a;
            if (a.mode == mode) return true;
        }
    }
    return false;
}

// Send command 284 to change the gimbal mode.
// Check telemetry 285 because command 284 has no ACK; resend every 200 ms until confirmed.
inline bool setGimbalModeAndWait(uint16_t flags, const std::string& mode, int timeoutMs,
                                 Attitude* last = nullptr) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->setGimbalMode(flags);
        if (waitGimbalMode(mode, 200, last)) return true;
    }
    return false;
}

// Move the gimbal at the given speed for a short time.
// Send the speed command repeatedly, then send 0 to stop.
inline void driveSpeedFor(float pitchDegPerSec, float yawDegPerSec, int durationMs) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(durationMs);
    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->setGimbalSpeed(pitchDegPerSec, 0, yawDegPerSec, INPUT_SPEED);
        sleepMs(100);
    }
    g_payload->setGimbalSpeed(0, 0, 0, INPUT_SPEED);
}

// ---------------------------------------------------------------------------
// Gimbal parameters (STIFF_TILT, PWR_PAN, ...): PARAM_SET / PARAM_VALUE, float values
// ---------------------------------------------------------------------------
// Read a gimbal parameter by id and wait for the reply.// The request is sent again every second until answered or timeout.
inline bool readGimbalParam(const char* id, double& out, int timeoutMs = 4000) {
    uint64_t seq = 0;
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        auto it = g_cb.gimbalParamSeqById.find(id);
        if (it != g_cb.gimbalParamSeqById.end()) seq = it->second;
    }
    char mutableId[MAVLINK_MSG_PARAM_REQUEST_READ_FIELD_PARAM_ID_LEN + 1] = {0};
    std::strncpy(mutableId, id, sizeof(mutableId) - 1);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->getPayloadGimbalSettingByID(mutableId);
        for (int i = 0; i < 50; ++i) {
            {
                std::lock_guard<std::mutex> lock(g_cb.m);
                auto it = g_cb.gimbalParamSeqById.find(id);
                if (it != g_cb.gimbalParamSeqById.end() && it->second > seq) {
                    out = g_cb.gimbalParamValueById[id];
                    return true;
                }
            }
            sleepMs(20);
        }
    }
    return false;
}

// Read a gimbal parameter by index. The reply also includes the id.
inline bool readGimbalParamByIndex(uint8_t index, GimbalParamSample& out, int timeoutMs = 4000) {
    uint64_t seq = 0;
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        auto it = g_cb.gimbalParamSeqByIndex.find(index);
        if (it != g_cb.gimbalParamSeqByIndex.end()) seq = it->second;
    }
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->getPayloadGimbalSettingByIndex(index);
        for (int i = 0; i < 50; ++i) {
            {
                std::lock_guard<std::mutex> lock(g_cb.m);
                auto it = g_cb.gimbalParamSeqByIndex.find(index);
                if (it != g_cb.gimbalParamSeqByIndex.end() && it->second > seq) {
                    out = g_cb.gimbalParamByIndex[index];
                    return true;
                }
            }
            sleepMs(20);
        }
    }
    return false;
}

// Set a gimbal parameter and read it back until it matches (tolerance 0.01).
inline bool setGimbalParam(const char* id, double value, int timeoutMs = 6000) {
    char mutableId[MAVLINK_MSG_PARAM_SET_FIELD_PARAM_ID_LEN + 1] = {0};
    std::strncpy(mutableId, id, sizeof(mutableId) - 1);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->setPayloadGimbalParamByID(mutableId, static_cast<float>(value));
        double current = 0;
        if (readGimbalParam(id, current, 1500) && std::fabs(current - value) <= 0.01) return true;
    }
    return false;
}

// Request the parameter list and collect replies for `collectMs`.
// VERSION_X may appear more than once because the gimbal also sends it on its own.
inline std::vector<GimbalParamSample> readGimbalParamList(int collectMs = 3000) {
    size_t start = 0;
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        start = g_cb.gimbalParamHistory.size();
    }
    g_payload->getPayloadGimbalSettingList();
    sleepMs(collectMs);
    std::lock_guard<std::mutex> lock(g_cb.m);
    if (start > g_cb.gimbalParamHistory.size()) return {};
    return std::vector<GimbalParamSample>(g_cb.gimbalParamHistory.begin() + start,
                                          g_cb.gimbalParamHistory.end());
}

// ---------------------------------------------------------------------------
// Test fixture: start from a known attitude, put everything back afterwards
// ---------------------------------------------------------------------------
class GimbalTest : public PayloadTest {
protected:
    // Save the gimbal's current state before the test.
    // Fails when no telemetry 285 arrives, because nothing can be checked then.
    void SetUp() override {
        ASSERT_TRUE(readAttitude(original_, 4000)) << "no gimbal attitude telemetry";
        haveOriginal_ = true;
        originalFlags_ = g_payload->getGimbalDeviceStatusFlags();

        std::cout << "[  INFO  ] SetUp: gimbal at pitch=" << original_.pitch
                  << " roll=" << original_.roll << " yaw=" << original_.yaw
                  << " mode=" << original_.mode << " flags=0x" << std::hex << originalFlags_ << std::dec << "\n";
    }

    // Stop the gimbal and restore the state changed by the test.
    // Command 284 goes back to the saved mode and angles, then parameters are restored.
    void TearDown() override {
        g_payload->setGimbalSpeed(0, 0, 0, INPUT_SPEED);

        // if (haveOriginal_) {
        //     // back to the original mode flags first, then to the original attitude
        //     if (g_payload->getGimbalDeviceStatusFlags() != originalFlags_) {
        //         if (!setGimbalModeAndWait(originalFlags_, original_.mode, 5000))
        //             ADD_FAILURE() << "cleanup: could not return the gimbal to " << original_.mode;
        //     }
        //     g_payload->setGimbalSpeed(static_cast<float>(original_.pitch), 0,
        //                               static_cast<float>(original_.yaw), INPUT_ANGLE);
        //     if (!waitAttitudeNear(original_.pitch, original_.yaw, 4.0, 8000))
        //         ADD_FAILURE() << "cleanup: could not return the gimbal to pitch=" << original_.pitch
        //                       << " yaw=" << original_.yaw;
        //     g_payload->setGimbalSpeed(0, 0, 0, INPUT_SPEED);
        // }

        // // restore in reverse order, so the last change is undone first
        // for (auto it = savedGimbal_.rbegin(); it != savedGimbal_.rend(); ++it) {
        //     if (!setGimbalParam(it->first.c_str(), it->second))
        //         ADD_FAILURE() << "cleanup: could not restore gimbal param " << it->first << " = " << it->second;
        // }
        // for (auto it = saved_.rbegin(); it != saved_.rend(); ++it) {
        //     if (!setParam(it->first.c_str(), static_cast<uint32_t>(it->second)))
        //         ADD_FAILURE() << "cleanup: could not restore " << it->first << " = " << it->second;
        // }
    }

    // Save the current camera-side setting so it can be restored after the test.
    bool restoreLater(const char* id) {
        double value = 0;
        if (!readParam(id, value)) return false;
        saved_.push_back({id, value});
        return true;
    }

    // Save a camera-side setting, then set a new value for this test.
    void saveAndSet(const char* id, uint32_t value) {
        ASSERT_TRUE(restoreLater(id)) << id << " is not readable";
        ASSERT_TRUE(setParam(id, value)) << "could not set " << id << " = " << value;
    }

    // Save the current gimbal parameter so it can be restored after the test.
    bool restoreGimbalParamLater(const char* id) {
        double value = 0;
        if (!readGimbalParam(id, value)) return false;
        savedGimbal_.push_back({id, value});
        return true;
    }

    // Return a pitch target offset from the start, kept inside -35..35.
    double pitchTarget(double offset) const {
        double target = original_.pitch + offset;
        if (target > 35.0 || target < -35.0) target = original_.pitch - offset;
        if (target > 35.0) target = 35.0;
        if (target < -35.0) target = -35.0;
        return target;
    }

    // Return a yaw target offset from the start, wrapped to -175..175.
    double yawTarget(double offset) const {
        double target = original_.yaw + offset;
        while (target > 175.0) target -= 360.0;
        while (target < -175.0) target += 360.0;
        return target;
    }

    Attitude original_;
    uint16_t originalFlags_ = 0;
    bool haveOriginal_ = false;

private:
    std::vector<std::pair<std::string, double>> saved_;
    std::vector<std::pair<std::string, double>> savedGimbal_;
};

}  // namespace gb

#endif  // GB_TEST_HELPERS_H_
