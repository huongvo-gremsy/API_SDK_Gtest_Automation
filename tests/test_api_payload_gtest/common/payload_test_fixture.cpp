#include "payload_test_fixture.h"
#include <csignal>
#include <cstring>
#include <iostream>

PayloadSdkInterface* g_payload = nullptr;
CallbackState g_cb;
int g_timeoutMs = 3000;

#if (CONTROL_METHOD == CONTROL_UART)
T_ConnInfo s_conn = { CONTROL_UART, payload_uart_port, payload_uart_baud };
#else
T_ConnInfo s_conn = { CONTROL_UDP, udp_ip_target, udp_port_target };
#endif

void registerAllCallbacks(PayloadSdkInterface* sdk) {
    sdk->regPayloadStatusChanged([](int event, double* p) {
        std::lock_guard<std::mutex> lk(g_cb.m);
        g_cb.lastStatusEvent = event;
        switch (event) {
            case PAYLOAD_ACK:
                g_cb.lastAck = { (uint16_t)p[0], (uint8_t)p[1], (uint8_t)p[2] };
                g_cb.ackByCommand[g_cb.lastAck.command] = g_cb.lastAck;
                ++g_cb.ackSeqByCommand[g_cb.lastAck.command];
#ifdef PAYLOAD_TEST_DEBUG_EVENTS
                std::cerr << "[EVENT] PAYLOAD_ACK: command=" << p[0]
                          << " result=" << p[1] << " progress=" << p[2] << "\n";
#endif
                break;
            case PAYLOAD_CAM_STORAGE_INFO:
                for (int i = 0; i < 4; ++i) {
                    g_cb.statusParams[i] = p[i];
                    g_cb.cameraStorageInfo[i] = p[i];
                }
                g_cb.cameraStorageInfoSeq.fetch_add(1);
                break;
            case PAYLOAD_CAM_CAPTURE_STATUS:
                for (int i = 0; i < 4; ++i) {
                    g_cb.statusParams[i] = p[i];
                    g_cb.cameraCaptureStatus[i] = p[i];
                }
                g_cb.cameraCaptureStatusSeq.fetch_add(1);
                break;
            case PAYLOAD_CAM_SETTINGS:
                for (int i = 0; i < 3; ++i) {
                    g_cb.statusParams[i] = p[i];
                    g_cb.cameraSettings[i] = p[i];
                }
                g_cb.cameraSettingsSeq.fetch_add(1);
                break;
            case PAYLOAD_PARAM_CAM_FOV_STATUS:
                for (int i = 0; i < 3; ++i) {
                    g_cb.statusParams[i] = p[i];
                    g_cb.cameraFov[i] = p[i];
                }
                g_cb.cameraFovSeq.fetch_add(1);
                break;
            case PAYLOAD_GB_ATTITUDE:
                for (int i = 0; i < 3; ++i) {
                    g_cb.statusParams[i] = p[i];
                    g_cb.gimbalAttitude[i] = p[i];
                }
                g_cb.gimbalAttitudeSeq.fetch_add(1);
                break;
            case PAYLOAD_CAM_INFO:
                g_cb.statusParams[0] = p[0];
                g_cb.cameraInfoFlags = static_cast<uint32_t>(p[0]);
                g_cb.cameraInfoSeq.fetch_add(1);
                break;
            case PAYLOAD_PARAMS: {
                for (int i = 0; i < 2; ++i) g_cb.statusParams[i] = p[i];
                const uint16_t index = static_cast<uint16_t>(p[0]);
                g_cb.payloadParamValueByIndex[index] = p[1];
                ++g_cb.payloadParamSeqByIndex[index];
                g_cb.payloadParamSeq.fetch_add(1);
                break;
            }
            case PAYLOAD_PARAM_EXT_ACK:
                for (int i = 0; i < 2; ++i) g_cb.statusParams[i] = p[i];
                break;
            default:
                break;
        }
        g_cb.statusSeq.fetch_add(1);
    });

    sdk->regPayloadParamChanged([](int event, char* param_char, double* p) {
        std::lock_guard<std::mutex> lk(g_cb.m);
        g_cb.lastParamEvent = event;
        g_cb.lastParamId = param_char
            ? std::string(param_char, strnlen(param_char, CAM_PARAM_ID_LEN))
            : "";
        int count = (event == PAYLOAD_GB_ATTITUDE) ? 6 : 2;
        for (int i = 0; i < count; ++i) g_cb.paramValues[i] = p[i];
        if (event == PAYLOAD_CAM_PARAMS && param_char) {
            g_cb.paramValueById[g_cb.lastParamId] = p[1];
            ++g_cb.paramSeqById[g_cb.lastParamId];
            g_cb.cameraParamHistory.push_back({
                static_cast<uint16_t>(p[0]), g_cb.lastParamId, p[1]});
            if (g_cb.cameraParamHistory.size() > 1024) {
                g_cb.cameraParamHistory.erase(
                    g_cb.cameraParamHistory.begin(),
                    g_cb.cameraParamHistory.begin() + 512);
            }
        } else if (event == PAYLOAD_GB_PARAMS && param_char && param_char[0]) {
            const std::string id(param_char,
                                 strnlen(param_char, MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN));
            const uint16_t index = static_cast<uint16_t>(p[0]);
            const GimbalParamSample sample{index, id, p[1]};
            g_cb.gimbalParamValueById[id] = p[1];
            ++g_cb.gimbalParamSeqById[id];
            g_cb.gimbalParamByIndex[index] = sample;
            ++g_cb.gimbalParamSeqByIndex[index];
            g_cb.gimbalParamHistory.push_back(sample);
            if (g_cb.gimbalParamHistory.size() > 2048) {
                g_cb.gimbalParamHistory.erase(
                    g_cb.gimbalParamHistory.begin(),
                    g_cb.gimbalParamHistory.begin() + 1024);
            }
            g_cb.gimbalParamSeq.fetch_add(1);
        } else if (event == PAYLOAD_GB_ATTITUDE) {
            g_cb.gimbalMode = param_char ? param_char : "";
            for (int i = 0; i < 6; ++i) g_cb.gimbalAttitude[i] = p[i];
            g_cb.gimbalAttitudeSeq.fetch_add(1);
        }
        g_cb.paramSeq.fetch_add(1);
    });

    sdk->regPayloadStreamChanged([](int event, char* uri, double* p) {
        std::lock_guard<std::mutex> lk(g_cb.m);

        g_cb.lastStreamEvent = event;

        g_cb.lastStreamUri = uri ? uri : "";

        for (int i = 0; i < 5; ++i) {
            g_cb.streamValues[i] = p[i];
        }

    #ifdef PAYLOAD_TEST_DEBUG_EVENTS
        std::cerr
            << "[STREAM EVENT]"
            << " event=" << event
            << " uri=" << g_cb.lastStreamUri
            << " p0=" << p[0]
            << " p1=" << p[1]
            << " p2=" << p[2]
            << " p3=" << p[3]
            << " p4=" << p[4]
            << std::endl;
    #endif

        g_cb.streamSeq.fetch_add(1);
    });

    sdk->regPayloadInfoChanged([](int event, std::vector<std::string> info) {
        std::lock_guard<std::mutex> lk(g_cb.m);
        g_cb.lastInfoEvent = event;
        g_cb.lastInfo = info;
        if (event == PAYLOAD_COMP_INFO) {
            g_cb.componentInfo = info;
            g_cb.componentInfoSeq.fetch_add(1);
        }
        g_cb.infoSeq.fetch_add(1);
    });

    sdk->regPayloadRecordInfoChanged([](int event, char* text, double*) {
        std::lock_guard<std::mutex> lock(g_cb.m);
        g_cb.lastRecordEvent = event;
        g_cb.lastRecordText = text ? text : "";
        g_cb.recordInfoSeq.fetch_add(1);
    });
    sdk->regPayloadHeartbeatChanged([](mavlink_message_t) { g_cb.heartbeatSeq.fetch_add(1); });
}

bool waitForStatusEvent(int expectedEvent, uint64_t sinceSeq, int timeoutMs) {
    if (timeoutMs < 0) timeoutMs = g_timeoutMs;
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - start).count() < timeoutMs) {
        if (g_cb.statusSeq.load() > sinceSeq && g_cb.lastStatusEvent.load() == expectedEvent) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}

bool waitForParamEvent(int expectedEvent, uint64_t sinceSeq, int timeoutMs) {
    if (timeoutMs < 0) timeoutMs = g_timeoutMs;
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - start).count() < timeoutMs) {
        if (g_cb.paramSeq.load() > sinceSeq && g_cb.lastParamEvent.load() == expectedEvent) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}

bool waitForSeq(std::atomic<uint64_t>& counter, uint64_t sinceSeq, int timeoutMs) {
    if (timeoutMs < 0) timeoutMs = g_timeoutMs;
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - start).count() < timeoutMs) {
        if (counter.load() > sinceSeq) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}

uint64_t getCommandAckSeq(uint16_t command) {
    std::lock_guard<std::mutex> lk(g_cb.m);
    const auto it = g_cb.ackSeqByCommand.find(command);
    return it == g_cb.ackSeqByCommand.end() ? 0 : it->second;
}

bool waitForCommandAck(uint16_t command, uint64_t sinceSeq, AckInfo& outAck,
                       int timeoutMs) {
    if (timeoutMs < 0) timeoutMs = g_timeoutMs;
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);

    while (std::chrono::steady_clock::now() < deadline) {
        {
            std::lock_guard<std::mutex> lk(g_cb.m);
            const auto seqIt = g_cb.ackSeqByCommand.find(command);
            if (seqIt != g_cb.ackSeqByCommand.end() && seqIt->second > sinceSeq) {
                outAck = g_cb.ackByCommand.at(command);
                return true;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}

void expectAckedCommand(std::function<void()> sendFn, uint16_t expectedCommand) {
    const uint64_t seq = getCommandAckSeq(expectedCommand);
    sendFn();
    AckInfo ack;
    ASSERT_TRUE(waitForCommandAck(expectedCommand, seq, ack))
        << "No COMMAND_ACK for command " << expectedCommand << " within timeout.";
    EXPECT_EQ(ack.result, MAV_RESULT_ACCEPTED)
        << "Payload rejected command " << expectedCommand
        << " (result=" << static_cast<int>(ack.result) << ").";
}

bool getComponentInformation(std::vector<std::string>& outInfo, int timeoutMs) {
    if (timeoutMs < 0) timeoutMs = g_timeoutMs;
    const uint64_t seq = g_cb.componentInfoSeq.load();
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);

    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->getPayloadComponentBasicInformation();
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now()).count();
        if (remaining <= 0) break;
        if (waitForSeq(g_cb.componentInfoSeq, seq,
                       static_cast<int>(std::min<int64_t>(500, remaining)))) {
            std::lock_guard<std::mutex> lk(g_cb.m);
            outInfo = g_cb.componentInfo;
            return true;
        }
    }
    return false;
}

class PayloadEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        g_payload = new PayloadSdkInterface(s_conn);
        registerAllCallbacks(g_payload);
        g_payload->sdkInitConnection();
        g_payload->checkPayloadConnection();

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    void TearDown() override {
        if (g_payload) { g_payload->sdkQuit(); delete g_payload; g_payload = nullptr; }
    }
};

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new PayloadEnvironment);
    signal(SIGINT, [](int) { if (g_payload) { try { g_payload->sdkQuit(); } catch (...) {} } exit(0); });
    return RUN_ALL_TESTS();
}
