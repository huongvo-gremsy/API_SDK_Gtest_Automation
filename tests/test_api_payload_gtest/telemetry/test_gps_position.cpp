/**
 * @file test_gps_position.cpp
 * @brief Packet-level tests for sendPayloadGPSPosition().
 */

#include "udp_telemetry_capture.h"

#include <cstdint>
#include <limits>

namespace tt = telemetry_test;

class GpsPositionTest : public tt::TelemetryPacketTest {
protected:
    void expectPacketEquals(const mavlink_global_position_int_t& expected) {
        mavlink_message_t message{};
        ASSERT_TRUE(capture_->sendAndCapture(
            MAVLINK_MSG_ID_GLOBAL_POSITION_INT,
            [&](PayloadSdkInterface& sdk) { sdk.sendPayloadGPSPosition(expected); },
            message));
        EXPECT_EQ(message.sysid, 1);
        EXPECT_EQ(message.compid, MAV_COMP_ID_ONBOARD_COMPUTER3);

        mavlink_global_position_int_t actual{};
        mavlink_msg_global_position_int_decode(&message, &actual);
        EXPECT_EQ(actual.time_boot_ms, expected.time_boot_ms);
        EXPECT_EQ(actual.lat, expected.lat);
        EXPECT_EQ(actual.lon, expected.lon);
        EXPECT_EQ(actual.alt, expected.alt);
        EXPECT_EQ(actual.relative_alt, expected.relative_alt);
        EXPECT_EQ(actual.vx, expected.vx);
        EXPECT_EQ(actual.vy, expected.vy);
        EXPECT_EQ(actual.vz, expected.vz);
        EXPECT_EQ(actual.hdg, expected.hdg);
    }
};

TEST_F(GpsPositionTest, EncodesExactGlobalPositionInt) {
    mavlink_global_position_int_t value{};
    value.time_boot_ms = 123456789u;
    value.lat = 107762090;       // 10.7762090 deg
    value.lon = 1067009810;      // 106.7009810 deg
    value.alt = 15250;
    value.relative_alt = 4250;
    value.vx = 321;
    value.vy = -123;
    value.vz = 45;
    value.hdg = 27123;
    expectPacketEquals(value);
}

TEST_F(GpsPositionTest, NegativeLatitudeLongitudeAndAltitudePreserved) {
    mavlink_global_position_int_t value{};
    value.time_boot_ms = 42;
    value.lat = -338688000;
    value.lon = -1512093000;
    value.alt = -430000;
    value.relative_alt = -12000;
    value.vx = -327;
    value.vy = 654;
    value.vz = -25;
    value.hdg = 0;
    expectPacketEquals(value);
}

TEST_F(GpsPositionTest, TimestampVelocityAndHeadingBoundariesPreserved) {
    mavlink_global_position_int_t value{};
    value.time_boot_ms = std::numeric_limits<uint32_t>::max();
    value.lat = 900000000;
    value.lon = -1800000000;
    value.alt = std::numeric_limits<int32_t>::max();
    value.relative_alt = std::numeric_limits<int32_t>::min();
    value.vx = std::numeric_limits<int16_t>::max();
    value.vy = std::numeric_limits<int16_t>::min();
    value.vz = 0;
    value.hdg = std::numeric_limits<uint16_t>::max();  // unknown heading
    expectPacketEquals(value);
}
