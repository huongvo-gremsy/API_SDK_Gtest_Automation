/** @file test_payload_navigation_time.cpp */
#include "../telemetry/udp_telemetry_capture.h"

#include <chrono>

namespace tt = telemetry_test;

class PayloadGpsExampleTest : public tt::TelemetryPacketTest {};

TEST_F(PayloadGpsExampleTest, NoFixRawThenDgpsRawAndGlobalPosition) {
    const auto now = std::chrono::system_clock::now();
    const uint64_t timeUsec = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()).count());

    mavlink_gps_raw_int_t raw{};
    raw.time_usec = timeUsec;
    raw.fix_type = GPS_FIX_TYPE_NO_FIX;
    raw.lat = 407306100;
    raw.lon = -739352420;
    raw.alt = 50000;
    raw.eph = 100;
    raw.epv = 150;
    raw.satellites_visible = 3;

    mavlink_message_t message{};
    ASSERT_TRUE(capture_->sendAndCapture(
        MAVLINK_MSG_ID_GPS_RAW_INT,
        [&](PayloadSdkInterface& sdk) { sdk.sendPayloadGPSRawInt(raw); },
        message));
    mavlink_gps_raw_int_t decodedRaw{};
    mavlink_msg_gps_raw_int_decode(&message, &decodedRaw);
    EXPECT_EQ(decodedRaw.fix_type, GPS_FIX_TYPE_NO_FIX);
    EXPECT_EQ(decodedRaw.lat, raw.lat);
    EXPECT_EQ(decodedRaw.lon, raw.lon);

    raw.fix_type = GPS_FIX_TYPE_DGPS;
    raw.satellites_visible = 8;
    ASSERT_TRUE(capture_->sendAndCapture(
        MAVLINK_MSG_ID_GPS_RAW_INT,
        [&](PayloadSdkInterface& sdk) { sdk.sendPayloadGPSRawInt(raw); },
        message));
    mavlink_msg_gps_raw_int_decode(&message, &decodedRaw);
    EXPECT_EQ(decodedRaw.fix_type, GPS_FIX_TYPE_DGPS);

    mavlink_global_position_int_t position{};
    position.time_boot_ms = 2000;
    position.lat = raw.lat;
    position.lon = raw.lon;
    position.alt = raw.alt;
    position.hdg = 22500;
    ASSERT_TRUE(capture_->sendAndCapture(
        MAVLINK_MSG_ID_GLOBAL_POSITION_INT,
        [&](PayloadSdkInterface& sdk) { sdk.sendPayloadGPSPosition(position); },
        message));
    mavlink_global_position_int_t decodedPosition{};
    mavlink_msg_global_position_int_decode(&message, &decodedPosition);
    EXPECT_EQ(decodedPosition.lat, position.lat);
    EXPECT_EQ(decodedPosition.lon, position.lon);
    EXPECT_EQ(decodedPosition.alt, position.alt);
    EXPECT_EQ(decodedPosition.hdg, position.hdg);
}

class PayloadSystemTimeExampleTest : public tt::TelemetryPacketTest {};

TEST_F(PayloadSystemTimeExampleTest, CurrentEpochAndIncreasingBootTimeEncode) {
    uint64_t previousUnix = 0;
    for (uint32_t i = 0; i < 3; ++i) {
        mavlink_system_time_t value{};
        value.time_boot_ms = i * 100;
        value.time_unix_usec = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
        mavlink_message_t message{};
        ASSERT_TRUE(capture_->sendAndCapture(
            MAVLINK_MSG_ID_SYSTEM_TIME,
            [&](PayloadSdkInterface& sdk) { sdk.sendPayloadSystemTime(value); },
            message));
        mavlink_system_time_t decoded{};
        mavlink_msg_system_time_decode(&message, &decoded);
        EXPECT_EQ(decoded.time_boot_ms, value.time_boot_ms);
        EXPECT_EQ(decoded.time_unix_usec, value.time_unix_usec);
        EXPECT_GE(decoded.time_unix_usec, previousUnix);
        previousUnix = decoded.time_unix_usec;
    }
}
