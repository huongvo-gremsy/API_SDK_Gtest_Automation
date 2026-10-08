#ifndef PAYLOADSDK_TEST_GIMBAL_TEST_HELPERS_H_
#define PAYLOADSDK_TEST_GIMBAL_TEST_HELPERS_H_

#include "../common/payload_test_fixture.h"
#include "../camera/parameters/camera_param_test_helpers.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace gimbal_test {

using GimbalParamSample = ::GimbalParamSample;

struct Attitude {
    double pitch = 0;
    double roll = 0;
    double yaw = 0;
    double rateX = 0;
    double rateY = 0;
    double rateZ = 0;
    std::string mode;
};

inline bool getParamById(const std::string& id, GimbalParamSample& out,
                         int timeoutMs = 4000) {
    uint64_t seq = 0;
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        seq = g_cb.gimbalParamSeqById[id];
    }
    char mutableId[MAVLINK_MSG_PARAM_REQUEST_READ_FIELD_PARAM_ID_LEN + 1] = {0};
    std::strncpy(mutableId, id.c_str(), sizeof(mutableId) - 1);
    g_payload->getPayloadGimbalSettingByID(mutableId);

    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        {
            std::lock_guard<std::mutex> lock(g_cb.m);
            if (g_cb.gimbalParamSeqById[id] > seq) {
                out.id = id;
                out.value = g_cb.gimbalParamValueById[id];
                for (const auto& item : g_cb.gimbalParamHistory) {
                    if (item.id == id) out.index = item.index;
                }
                return true;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}

inline bool getParamByIndex(uint8_t index, GimbalParamSample& out,
                            int timeoutMs = 4000) {
    uint64_t seq = 0;
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        seq = g_cb.gimbalParamSeqByIndex[index];
    }
    g_payload->getPayloadGimbalSettingByIndex(index);
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        {
            std::lock_guard<std::mutex> lock(g_cb.m);
            if (g_cb.gimbalParamSeqByIndex[index] > seq) {
                out = g_cb.gimbalParamByIndex[index];
                return true;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}

inline std::vector<GimbalParamSample> getParamList(int timeoutMs = 8000,
                                                    int quietMs = 700) {
    size_t start = 0;
    uint64_t seq = g_cb.gimbalParamSeq.load();
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        start = g_cb.gimbalParamHistory.size();
    }
    g_payload->getPayloadGimbalSettingList();
    if (!waitForSeq(g_cb.gimbalParamSeq, seq, timeoutMs)) return {};

    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(timeoutMs);
    uint64_t observed = g_cb.gimbalParamSeq.load();
    auto quietDeadline = std::chrono::steady_clock::now() +
                         std::chrono::milliseconds(quietMs);
    while (std::chrono::steady_clock::now() < deadline &&
           std::chrono::steady_clock::now() < quietDeadline) {
        const uint64_t current = g_cb.gimbalParamSeq.load();
        if (current != observed) {
            observed = current;
            quietDeadline = std::chrono::steady_clock::now() +
                            std::chrono::milliseconds(quietMs);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    std::lock_guard<std::mutex> lock(g_cb.m);
    if (start > g_cb.gimbalParamHistory.size()) return {};
    return std::vector<GimbalParamSample>(g_cb.gimbalParamHistory.begin() + start,
                                          g_cb.gimbalParamHistory.end());
}

inline bool setParamAndVerify(const std::string& id, double value,
                              int timeoutMs = 5000) {
    char mutableId[MAVLINK_MSG_PARAM_SET_FIELD_PARAM_ID_LEN + 1] = {0};
    std::strncpy(mutableId, id.c_str(), sizeof(mutableId) - 1);
    g_payload->setPayloadGimbalParamByID(mutableId, static_cast<float>(value));
    GimbalParamSample actual;
    return getParamById(id, actual, timeoutMs) &&
           std::fabs(actual.value - value) <= 0.01;
}

inline bool getAttitude(Attitude& out, int timeoutMs = 3000) {
    const uint64_t seq = g_cb.gimbalAttitudeSeq.load();
    if (!waitForSeq(g_cb.gimbalAttitudeSeq, seq, timeoutMs)) {
        return false;
    }
    std::lock_guard<std::mutex> lock(g_cb.m);
    out.pitch = g_cb.gimbalAttitude[0];
    out.roll = g_cb.gimbalAttitude[1];
    out.yaw = g_cb.gimbalAttitude[2];
    out.rateX = g_cb.gimbalAttitude[3];
    out.rateY = g_cb.gimbalAttitude[4];
    out.rateZ = g_cb.gimbalAttitude[5];
    out.mode = g_cb.gimbalMode;
    return std::isfinite(out.pitch) && std::isfinite(out.roll) &&
           std::isfinite(out.yaw);
}

inline double angleDifference(double a, double b) {
    double value = std::fmod(a - b + 540.0, 360.0) - 180.0;
    return std::fabs(value);
}

inline double signedAngleDifference(double current, double previous) {
    return std::fmod(current - previous + 540.0, 360.0) - 180.0;
}

// Observe telemetry only. Unlike waitForAttitudeNear(), this helper never
// sends a second command, so it can verify exactly the API call under test.
inline bool waitForAttitudeNearPassive(double pitch, double yaw,
                                       double tolerance,
                                       int timeoutMs = 7000,
                                       Attitude* output = nullptr) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        Attitude current;
        if (getAttitude(current, 700) &&
            std::fabs(current.pitch - pitch) <= tolerance &&
            angleDifference(current.yaw, yaw) <= tolerance) {
            if (output) *output = current;
            return true;
        }
    }
    return false;
}

inline bool setControlParamAndVerify(const char* id, uint32_t value,
                                     int timeoutMs = 5000) {
    char mutableId[CAM_PARAM_ID_LEN] = {0};
    std::strncpy(mutableId, id, sizeof(mutableId) - 1);
    return setAndVerifyCameraParam(mutableId, value, PARAM_TYPE_UINT32,
                                   value, timeoutMs, 500);
}

inline bool waitForAttitudeNear(double pitch, double yaw, double tolerance,
                                int timeoutMs = 6000) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        Attitude current;
        if (getAttitude(current, 700) &&
            std::fabs(current.pitch - pitch) <= tolerance &&
            angleDifference(current.yaw, yaw) <= tolerance) {
            return true;
        }
        g_payload->setGimbalSpeed(static_cast<float>(pitch), 0,
                                  static_cast<float>(yaw), INPUT_ANGLE);
    }
    return false;
}

inline bool commandAccepted(uint16_t command, const std::function<void()>& send,
                            int timeoutMs = 5000) {
    const uint64_t seq = getCommandAckSeq(command);
    send();
    AckInfo ack;
    return waitForCommandAck(command, seq, ack, timeoutMs) &&
           (ack.result == MAV_RESULT_ACCEPTED ||
            ack.result == MAV_RESULT_IN_PROGRESS);
}

}  // namespace gimbal_test

#endif
