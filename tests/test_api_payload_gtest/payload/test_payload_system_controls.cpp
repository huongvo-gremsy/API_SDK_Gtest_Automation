/** @file test_payload_system_controls.cpp */
#include "../telemetry/udp_telemetry_capture.h"

namespace tt = telemetry_test;

class PayloadSystemControlEncodingTest : public tt::TelemetryPacketTest {
protected:
    template <typename Send>
    mavlink_command_long_t captureCommand(Send send) {
        mavlink_message_t message{};
        EXPECT_TRUE(capture_->sendAndCapture(
            MAVLINK_MSG_ID_COMMAND_LONG, send, message));
        mavlink_command_long_t command{};
        mavlink_msg_command_long_decode(&message, &command);
        return command;
    }
};

TEST_F(PayloadSystemControlEncodingTest, StandbyOnAndOffEncode) {
    for (const bool enabled : {false, true}) {
        const auto command = captureCommand(
            [=](PayloadSdkInterface& sdk) {
                sdk.setPayloadStandbyMode(enabled);
            });
        EXPECT_EQ(command.command, MAV_CMD_USER_4);
        EXPECT_FLOAT_EQ(command.param1, 4);
        EXPECT_FLOAT_EQ(command.param2, 3);
        EXPECT_FLOAT_EQ(command.param3, 0);
        EXPECT_FLOAT_EQ(command.param4, enabled ? 1.0f : 0.0f);
    }
}

TEST_F(PayloadSystemControlEncodingTest, RestartGimbalAppId4Encodes) {
    const auto command = captureCommand([](PayloadSdkInterface& sdk) {
        sdk.setPayloadRestartApps(4);
    });
    EXPECT_EQ(command.command, MAV_CMD_USER_4);
    EXPECT_FLOAT_EQ(command.param1, 4);
    EXPECT_FLOAT_EQ(command.param2, 4);
    EXPECT_FLOAT_EQ(command.param3, 4);
}

class PayloadSystemControlHardwareTest : public PayloadTest {};

TEST_F(PayloadSystemControlHardwareTest,
       DISABLED_StandbyEnableDisable_RecoversHeartbeat) {
    g_payload->setPayloadStandbyMode(true);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    const uint64_t heartbeatSeq = g_cb.heartbeatSeq.load();
    g_payload->setPayloadStandbyMode(false);
    EXPECT_TRUE(waitForSeq(g_cb.heartbeatSeq, heartbeatSeq, 15000));
}

TEST_F(PayloadSystemControlHardwareTest,
       DISABLED_RestartGimbalApp_HeartbeatRecovers) {
    const uint64_t heartbeatSeq = g_cb.heartbeatSeq.load();
    g_payload->setPayloadRestartApps(4);
    EXPECT_TRUE(waitForSeq(g_cb.heartbeatSeq, heartbeatSeq, 30000));
}
