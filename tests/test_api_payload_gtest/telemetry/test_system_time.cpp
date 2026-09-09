/**
 * @file test_system_time.cpp
 * @brief Packet-level tests for sendPayloadSystemTime().
 */

#include "udp_telemetry_capture.h"

#include <cstdint>
#include <limits>

namespace tt = telemetry_test;

class SystemTimeTest : public tt::TelemetryPacketTest {
protected:
    void expectPacketEquals(const mavlink_system_time_t& expected) {
        mavlink_message_t message{};
        ASSERT_TRUE(capture_->sendAndCapture(
            MAVLINK_MSG_ID_SYSTEM_TIME,
            [&](PayloadSdkInterface& sdk) { sdk.sendPayloadSystemTime(expected); },
            message));
        mavlink_system_time_t actual{};
        mavlink_msg_system_time_decode(&message, &actual);
        EXPECT_EQ(actual.time_unix_usec, expected.time_unix_usec);
        EXPECT_EQ(actual.time_boot_ms, expected.time_boot_ms);
    }
};

TEST_F(SystemTimeTest, EncodesUnixAndBootTimeExactly) {
    mavlink_system_time_t value{};
    value.time_unix_usec = 1787200000123456ULL;
    value.time_boot_ms = 987654321u;
    expectPacketEquals(value);
}

TEST_F(SystemTimeTest, ZeroValuesArePreserved) {
    mavlink_system_time_t value{};
    expectPacketEquals(value);
}

TEST_F(SystemTimeTest, MaximumTimestampsAreNotTruncated) {
    mavlink_system_time_t value{};
    value.time_unix_usec = std::numeric_limits<uint64_t>::max();
    value.time_boot_ms = std::numeric_limits<uint32_t>::max();
    expectPacketEquals(value);
}
