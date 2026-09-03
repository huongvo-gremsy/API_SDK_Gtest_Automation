/**
 * @file test_restart_apps.cpp
 * @brief Tests setPayloadRestartApps() encoding and opt-in hardware recovery.
 */

#include "system_test_helpers.h"
#include "../telemetry/udp_telemetry_capture.h"

#include <cstdint>
#include <iostream>

namespace st = system_test;
namespace tt = telemetry_test;

struct RestartAppCase {
    uint8_t id;
    const char* name;
};

std::ostream& operator<<(std::ostream& os, const RestartAppCase& app) {
    return os << app.name << "(id=" << static_cast<int>(app.id) << ")";
}

class RestartAppEncodingTest
    : public tt::TelemetryPacketTest,
      public ::testing::WithParamInterface<RestartAppCase> {};

TEST_P(RestartAppEncodingTest, EncodesExpectedApplicationId) {
    const auto app = GetParam();
    mavlink_message_t message{};
    ASSERT_TRUE(capture_->sendAndCapture(
        MAVLINK_MSG_ID_COMMAND_LONG,
        [=](PayloadSdkInterface& sdk) { sdk.setPayloadRestartApps(app.id); },
        message));
    mavlink_command_long_t command{};
    mavlink_msg_command_long_decode(&message, &command);
    EXPECT_EQ(command.command, MAV_CMD_USER_4);
    EXPECT_FLOAT_EQ(command.param1, 4.0f);
    EXPECT_FLOAT_EQ(command.param2, 4.0f);
    EXPECT_FLOAT_EQ(command.param3, static_cast<float>(app.id));
    EXPECT_FLOAT_EQ(command.param4, 0.0f);
    EXPECT_FLOAT_EQ(command.param5, 0.0f);
    EXPECT_FLOAT_EQ(command.param6, 0.0f);
    EXPECT_FLOAT_EQ(command.param7, 0.0f);
}

INSTANTIATE_TEST_SUITE_P(
    SupportedApps, RestartAppEncodingTest,
    ::testing::Values(RestartAppCase{0, "Payload"},
                      RestartAppCase{1, "Streaming"},
                      RestartAppCase{2, "Tracking"},
                      RestartAppCase{3, "WebUI"},
                      RestartAppCase{4, "Gimbal"}),
    [](const ::testing::TestParamInfo<RestartAppCase>& info) {
        return std::string(info.param.name);
    });

class RestartAppHardwareTest
    : public PayloadTest,
      public ::testing::WithParamInterface<RestartAppCase> {};

TEST_P(RestartAppHardwareTest, DISABLED_Restart_RecoversWithinTimeout) {
    const auto app = GetParam();
    AckInfo ack;
    const bool gotAck = st::sendUser4AndGetAck([=] {
        g_payload->setPayloadRestartApps(app.id);
    }, ack, 4000);
    if (gotAck) EXPECT_TRUE(st::acceptedOrInProgress(ack));
    EXPECT_TRUE(st::waitForCameraAndHeartbeat(30000))
        << app.name << " application did not recover within 30 seconds.";
}

INSTANTIATE_TEST_SUITE_P(
    SupportedApps, RestartAppHardwareTest,
    ::testing::Values(RestartAppCase{0, "Payload"},
                      RestartAppCase{1, "Streaming"},
                      RestartAppCase{2, "Tracking"},
                      RestartAppCase{3, "WebUI"},
                      RestartAppCase{4, "Gimbal"}),
    [](const ::testing::TestParamInfo<RestartAppCase>& info) {
        return std::string(info.param.name);
    });

class RestartAppInvalidIdTest : public PayloadTest {};

TEST_F(RestartAppInvalidIdTest, DISABLED_Invalid255_IsRejectedAndPayloadStaysResponsive) {
    AckInfo ack;
    ASSERT_TRUE(st::sendUser4AndGetAck([] {
        g_payload->setPayloadRestartApps(255);
    }, ack, 4000)) << "No ACK for invalid restart application ID 255.";
    EXPECT_FALSE(st::acceptedOrInProgress(ack));
    EXPECT_TRUE(st::waitForCameraAndHeartbeat(10000));
}
