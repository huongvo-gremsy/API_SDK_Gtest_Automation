/**
 * @file test_standby.cpp
 * @brief Tests setPayloadStandbyMode() encoding and recovery behavior.
 */

#include "system_test_helpers.h"
#include "../telemetry/udp_telemetry_capture.h"

#include <chrono>
#include <iostream>
#include <thread>

namespace st = system_test;
namespace tt = telemetry_test;

class StandbyEncodingTest : public tt::TelemetryPacketTest {};

TEST_F(StandbyEncodingTest, EnableAndDisable_EncodeExpectedUser4Fields) {
    for (const bool enabled : {false, true}) {
        mavlink_message_t message{};
        ASSERT_TRUE(capture_->sendAndCapture(
            MAVLINK_MSG_ID_COMMAND_LONG,
            [=](PayloadSdkInterface& sdk) { sdk.setPayloadStandbyMode(enabled); },
            message));
        mavlink_command_long_t command{};
        mavlink_msg_command_long_decode(&message, &command);
        EXPECT_EQ(command.command, MAV_CMD_USER_4);
        EXPECT_FLOAT_EQ(command.param1, 4.0f);
        EXPECT_FLOAT_EQ(command.param2, 3.0f);
        EXPECT_FLOAT_EQ(command.param3, 0.0f);
        EXPECT_FLOAT_EQ(command.param4, enabled ? 1.0f : 0.0f);
        EXPECT_EQ(command.target_system, PAYLOAD_SYSTEM_ID);
        EXPECT_EQ(command.target_component, PAYLOAD_COMPONENT_ID);
    }
}

class StandbyTest : public PayloadTest {
protected:
    void TearDown() override {
        g_payload->setPayloadStandbyMode(false);
        if (!st::waitForCameraAndHeartbeat(12000)) {
            ADD_FAILURE() << "Payload did not recover after disabling standby.";
        }
    }
};

TEST_F(StandbyTest, Disable_LeavesCameraAndHeartbeatResponsive) {
    AckInfo ack;
    const bool gotAck = st::sendUser4AndGetAck([] {
        g_payload->setPayloadStandbyMode(false);
    }, ack);
    EXPECT_TRUE(st::waitForCameraAndHeartbeat(10000));
    if (gotAck) EXPECT_TRUE(st::acceptedOrInProgress(ack));
}

TEST_F(StandbyTest, EnableThenDisable_RestoresCameraQueries) {
    AckInfo enableAck;
    const bool gotEnableAck = st::sendUser4AndGetAck([] {
        g_payload->setPayloadStandbyMode(true);
    }, enableAck);
    const uint64_t heartbeatBefore = g_cb.heartbeatSeq.load();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "[INFO] Heartbeats observed during standby window: "
              << (g_cb.heartbeatSeq.load() - heartbeatBefore) << std::endl;
    if (gotEnableAck) EXPECT_TRUE(st::acceptedOrInProgress(enableAck));

    AckInfo disableAck;
    const bool gotDisableAck = st::sendUser4AndGetAck([] {
        g_payload->setPayloadStandbyMode(false);
    }, disableAck);
    EXPECT_TRUE(st::waitForCameraAndHeartbeat(12000));
    if (gotDisableAck) EXPECT_TRUE(st::acceptedOrInProgress(disableAck));
}

TEST_F(StandbyTest, RepeatedEnableThenDisable_RemainsRecoverable) {
    g_payload->setPayloadStandbyMode(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    g_payload->setPayloadStandbyMode(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    g_payload->setPayloadStandbyMode(false);
    EXPECT_TRUE(st::waitForCameraAndHeartbeat(12000));
}
