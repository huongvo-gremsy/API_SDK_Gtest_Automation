/**
 * @file payload_test_fixture.h
 * @brief Shared MAVLink connection, callback capture, and synchronization.
 *
 * Feature-specific state verification belongs to each test module's helper.
 */
#ifndef PAYLOAD_TEST_FIXTURE_H_
#define PAYLOAD_TEST_FIXTURE_H_

#include <gtest/gtest.h>
#include <atomic>
#include <mutex>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <thread>
#include <unordered_map>

#include "payloadSdkInterface.h"

#ifndef MAV_RESULT_ACCEPTED
#define MAV_RESULT_ACCEPTED 0
#endif

extern PayloadSdkInterface* g_payload;
extern int g_timeoutMs;

struct AckInfo { uint16_t command = 0; uint8_t result = 0xFF; uint8_t progress = 0; };

struct CameraParamSample {
    uint16_t index = 0;
    std::string id;
    double value = 0;
};

struct GimbalParamSample {
    uint16_t index = 0;
    std::string id;
    double value = 0;
};

struct CallbackState {
    std::mutex m;

    // ---------------- Status callback ----------------
    std::atomic<uint64_t> statusSeq{0};
    std::atomic<int> lastStatusEvent{-1};

    AckInfo lastAck;
    std::unordered_map<uint16_t, uint64_t> ackSeqByCommand;
    std::unordered_map<uint16_t, AckInfo> ackByCommand;

    double statusParams[4] = {0};

    // Keep independent counters and storage for status messages that can be
    // followed immediately by a COMMAND_ACK. Relying on lastStatusEvent for
    // these messages can miss them when the ACK overwrites it before the test
    // thread wakes up.
    std::atomic<uint64_t> cameraSettingsSeq{0};
    double cameraSettings[3] = {0};

    std::atomic<uint64_t> cameraFovSeq{0};
    double cameraFov[3] = {0};

    std::atomic<uint64_t> cameraCaptureStatusSeq{0};
    double cameraCaptureStatus[4] = {0};

    std::atomic<uint64_t> cameraStorageInfoSeq{0};
    double cameraStorageInfo[4] = {0};

    std::atomic<uint64_t> cameraInfoSeq{0};
    uint32_t cameraInfoFlags = 0;

    std::atomic<uint64_t> payloadParamSeq{0};
    std::unordered_map<uint16_t, uint64_t> payloadParamSeqByIndex;
    std::unordered_map<uint16_t, double> payloadParamValueByIndex;


    // ---------------- Parameter callback ----------------
    std::atomic<uint64_t> paramSeq{0};
    std::atomic<int> lastParamEvent{-1};

    std::string lastParamId;
    double paramValues[6] = {0};
    std::unordered_map<std::string, uint64_t> paramSeqById;
    std::unordered_map<std::string, double> paramValueById;
    std::vector<CameraParamSample> cameraParamHistory;

    std::atomic<uint64_t> gimbalParamSeq{0};
    std::unordered_map<std::string, uint64_t> gimbalParamSeqById;
    std::unordered_map<std::string, double> gimbalParamValueById;
    std::unordered_map<uint16_t, uint64_t> gimbalParamSeqByIndex;
    std::unordered_map<uint16_t, GimbalParamSample> gimbalParamByIndex;
    std::vector<GimbalParamSample> gimbalParamHistory;

    std::atomic<uint64_t> gimbalAttitudeSeq{0};
    std::string gimbalMode;
    double gimbalAttitude[6] = {0};


    // ---------------- Stream callback ----------------
    std::atomic<uint64_t> streamSeq{0};
    std::atomic<int> lastStreamEvent{-1};

    std::string lastStreamUri;

    double streamValues[5] = {0};


    // ---------------- Info callback ----------------
    std::atomic<uint64_t> infoSeq{0};
    std::atomic<int> lastInfoEvent{-1};
    std::vector<std::string> lastInfo;

    std::atomic<uint64_t> componentInfoSeq{0};
    std::vector<std::string> componentInfo;

    // ---------------- Record-info callback ----------------
    std::atomic<uint64_t> recordInfoSeq{0};
    std::atomic<int> lastRecordEvent{-1};
    std::string lastRecordText;


    // ---------------- Heartbeat ----------------
    std::atomic<uint64_t> heartbeatSeq{0};
};

extern CallbackState g_cb;

void registerAllCallbacks(PayloadSdkInterface* sdk);

// ---- Low-level building blocks ----

// Poll for a NEW event of a specific type, arriving strictly after sinceSeq.
// 20ms poll window: two different events of the same callback type arriving
// within the same tick can race past each other -- acceptable for typical
// MAVLink response timing on a bench, not a hard real-time guarantee.
bool waitForStatusEvent(int expectedEvent, uint64_t sinceSeq, int timeoutMs = -1);
bool waitForParamEvent(int expectedEvent, uint64_t sinceSeq, int timeoutMs = -1);
bool waitForSeq(std::atomic<uint64_t>& counter, uint64_t sinceSeq, int timeoutMs = -1);

// Command-specific ACK tracking. Unlike lastAck, an unrelated ACK cannot
// replace the ACK being awaited.
uint64_t getCommandAckSeq(uint16_t command);
bool waitForCommandAck(uint16_t command, uint64_t sinceSeq, AckInfo& outAck,
                       int timeoutMs = -1);

// Generic MAVLink command-ACK assertion.
void expectAckedCommand(std::function<void()> sendFn, uint16_t expectedCommand);

// Query COMPONENT_INFORMATION_BASIC. The SDK exposes model name, software
// version, and serial number in that order through the info callback.
bool getComponentInformation(std::vector<std::string>& outInfo,
                             int timeoutMs = -1);

class PayloadTest : public ::testing::Test {};

#endif
