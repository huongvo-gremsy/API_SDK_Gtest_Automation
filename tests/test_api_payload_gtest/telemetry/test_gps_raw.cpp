/**
 * @file test_gps_raw.cpp
 * @brief Packet-level tests for sendPayloadGPSRawInt().
 */

#include "udp_telemetry_capture.h"

#include <cstdint>
#include <limits>

namespace tt = telemetry_test;

class GpsRawTest : public tt::TelemetryPacketTest {
protected:
    void expectPacketEquals(const mavlink_gps_raw_int_t& expected) {
        mavlink_message_t message{};
        ASSERT_TRUE(capture_->sendAndCapture(
            MAVLINK_MSG_ID_GPS_RAW_INT,
            [&](PayloadSdkInterface& sdk) { sdk.sendPayloadGPSRawInt(expected); },
            message));

        mavlink_gps_raw_int_t actual{};
        mavlink_msg_gps_raw_int_decode(&message, &actual);
        EXPECT_EQ(actual.time_usec, expected.time_usec);
        EXPECT_EQ(actual.fix_type, expected.fix_type);
        EXPECT_EQ(actual.lat, expected.lat);
        EXPECT_EQ(actual.lon, expected.lon);
        EXPECT_EQ(actual.alt, expected.alt);
        EXPECT_EQ(actual.eph, expected.eph);
        EXPECT_EQ(actual.epv, expected.epv);
        EXPECT_EQ(actual.vel, expected.vel);
        EXPECT_EQ(actual.cog, expected.cog);
        EXPECT_EQ(actual.satellites_visible, expected.satellites_visible);
        EXPECT_EQ(actual.alt_ellipsoid, expected.alt_ellipsoid);
        EXPECT_EQ(actual.h_acc, expected.h_acc);
        EXPECT_EQ(actual.v_acc, expected.v_acc);
        EXPECT_EQ(actual.vel_acc, expected.vel_acc);
        EXPECT_EQ(actual.hdg_acc, expected.hdg_acc);
        EXPECT_EQ(actual.yaw, expected.yaw);
    }
};

TEST_F(GpsRawTest, EncodesAllBaseAndExtensionFieldsExactly) {
    mavlink_gps_raw_int_t value{};
    value.time_usec = 1787200000123456ULL;
    value.fix_type = GPS_FIX_TYPE_DGPS;
    value.lat = 107762090;
    value.lon = 1067009810;
    value.alt = 15250;
    value.eph = 100;
    value.epv = 150;
    value.vel = 500;
    value.cog = 12345;
    value.satellites_visible = 12;
    value.alt_ellipsoid = 45800;
    value.h_acc = 2000;
    value.v_acc = 3000;
    value.vel_acc = 100;
    value.hdg_acc = 500;
    value.yaw = 12345;
    expectPacketEquals(value);
}

TEST_F(GpsRawTest, NoFixAndUnknownValuesArePreserved) {
    mavlink_gps_raw_int_t value{};
    value.time_usec = 1000;
    value.fix_type = GPS_FIX_TYPE_NO_FIX;
    value.eph = std::numeric_limits<uint16_t>::max();
    value.epv = std::numeric_limits<uint16_t>::max();
    value.vel = std::numeric_limits<uint16_t>::max();
    value.cog = std::numeric_limits<uint16_t>::max();
    value.satellites_visible = std::numeric_limits<uint8_t>::max();
    value.yaw = std::numeric_limits<uint16_t>::max();
    expectPacketEquals(value);
}

TEST_F(GpsRawTest, ThreeDimensionalFixNegativeCoordinatesAndAccuracyPreserved) {
    mavlink_gps_raw_int_t value{};
    value.time_usec = std::numeric_limits<uint64_t>::max();
    value.fix_type = GPS_FIX_TYPE_3D_FIX;
    value.lat = -338688000;
    value.lon = -1512093000;
    value.alt = -125000;
    value.eph = 250;
    value.epv = 400;
    value.vel = 65534;
    value.cog = 35999;
    value.satellites_visible = 32;
    value.alt_ellipsoid = -90000;
    value.h_acc = std::numeric_limits<uint32_t>::max();
    value.v_acc = 123456;
    value.vel_acc = 654321;
    value.hdg_acc = 999999;
    value.yaw = 36000;
    expectPacketEquals(value);
}
