#include "gb_test_helpers.h"

using namespace gb;

// Plain fixture, no parameterization: this is one fixed sequence, not a
// data-driven test over a list of rows. (WithParamInterface<GimbalParamRow>
// belongs on a TEST_P test with an INSTANTIATE_TEST_SUITE_P; it doesn't
// apply to a single TEST_F like this one, and GimbalParamRow isn't a type
// gb_test_helpers.h defines.)
class GB_ControlExample : public GimbalTest {};

TEST_F(GB_ControlExample, TestGimbalControlExample) {
    // Mirrors the vendor "Set gimbal mode" example: cycle FOLLOW -> LOCK ->
    // FOLLOW -> MAPPING -> RESET -> OFF, driving by speed and by angle and
    // re-centering each axis in between.
    //
    // Difference from the vendor example: its setRecenterYaw()/
    // setRecenterPitch() busy-wait on `while (fabs(currentYaw) >= 0.2);`
    // with no timeout and no synchronization with the callback thread that
    // updates currentYaw/currentPitch -- that can spin forever or race.
    // This test uses waitAttitudeNear() instead, which polls telemetry 285
    // and bounds every wait with a timeout.

    Attitude last;

    auto recenterYaw = [&](double holdPitch, int timeoutMs) {
        g_payload->setGimbalSpeed(0, 0, 0, INPUT_SPEED);
        sleepMs(1500);
        g_payload->setGimbalSpeed(static_cast<float>(holdPitch), 0, 0, INPUT_ANGLE);
        return waitAttitudeNear(holdPitch, 0.0, 2.0, timeoutMs, &last);
    };

    auto recenterPitch = [&](double holdYaw, int timeoutMs) {
        g_payload->setGimbalSpeed(0, 0, 0, INPUT_SPEED);
        sleepMs(1500);
        g_payload->setGimbalSpeed(0, 0, static_cast<float>(holdYaw), INPUT_ANGLE);
        return waitAttitudeNear(0.0, holdYaw, 2.0, timeoutMs, &last);
    };

    // ---- FOLLOW mode ------------------------------------------------
    // std::cout << "[  INFO  ] set mode FOLLOW\n";
    // ASSERT_TRUE(setParam(PAYLOAD_CAMERA_GIMBAL_MODE, PAYLOAD_CAMERA_GIMBAL_MODE_FOLLOW))
    //     << "could not set GB_MODE to FOLLOW";
    // EXPECT_TRUE(waitGimbalMode("FOLLOW_MODE", 5000, &last))
    //     << "gimbal did not report FOLLOW_MODE (last mode " << last.mode << ")";


    uint16_t flags = g_payload->getGimbalDeviceStatusFlags();

    std::cout << "[  INFO  ] flags before: 0x" << std::hex << flags << std::dec
            << " (mode " << original_.mode << ")\n";

    // FOLLOW: clear YAW_LOCK, RETRACT, NEUTRAL and the mapping bit, like the example.
    uint16_t followFlags = flags;
    followFlags = followFlags & ~GIMBAL_DEVICE_FLAGS_YAW_LOCK;
    followFlags = followFlags & ~GIMBAL_DEVICE_FLAGS_RETRACT;
    followFlags = followFlags & ~GIMBAL_DEVICE_FLAGS_NEUTRAL;
    followFlags = followFlags & ~(1 << 14);

    std::cout << "[  INFO  ] setGimbalMode(0x" << std::hex << followFlags << std::dec
            << "): sending every 200 ms (FOLLOW)\n";

    // Attitude last;
    bool following = setGimbalModeAndWait(followFlags, "FOLLOW_MODE", 5000, &last);

    EXPECT_TRUE(following)
        << "gimbal did not report FOLLOW_MODE (last mode " << last.mode << ")";

    std::cout << "[  INFO  ] mode reported: " << last.mode
            << ", flags 0x" << std::hex << g_payload->getGimbalDeviceStatusFlags() << std::dec << "\n";
    sleepMs(1000);
            //

    std::cout << "[  INFO  ] drive pitch=-20 deg/s, yaw=50 deg/s for 3 s\n";
    driveSpeedFor(-20, 50, 3000);

    EXPECT_TRUE(recenterYaw(last.pitch, 6000)) << "yaw did not re-center (last yaw " << last.yaw << ")";
    EXPECT_TRUE(recenterPitch(last.yaw, 6000)) << "pitch did not re-center (last pitch " << last.pitch << ")";

    std::cout << "[  INFO  ] move to pitch=-40, yaw=90\n";
    g_payload->setGimbalSpeed(-40, 0, 90, INPUT_ANGLE);
    EXPECT_TRUE(waitAttitudeNear(-40.0, 90.0, 4.0, 6000, &last))
        << "gimbal did not reach pitch=-40 yaw=90 (last pitch=" << last.pitch << " yaw=" << last.yaw << ")";

    EXPECT_TRUE(recenterPitch(last.yaw, 6000)) << "pitch did not re-center after move";
    EXPECT_TRUE(recenterYaw(last.pitch, 6000)) << "yaw did not re-center after move";

    // ---- LOCK mode ----------------------------------------------------
    // std::cout << "[  INFO  ] set mode LOCK\n";
    // ASSERT_TRUE(setParam(PAYLOAD_CAMERA_GIMBAL_MODE, PAYLOAD_CAMERA_GIMBAL_MODE_LOCK))
    //     << "could not set GB_MODE to LOCK";
    // EXPECT_TRUE(waitGimbalMode("LOCK_MODE", 5000, &last))
    //     << "gimbal did not report LOCK_MODE (last mode " << last.mode << ")";

    // LOCK: set YAW_LOCK, clear RETRACT, NEUTRAL and the mapping bit, like the example.
    flags = g_payload->getGimbalDeviceStatusFlags();
    uint16_t lockFlags = flags;
    lockFlags = lockFlags | GIMBAL_DEVICE_FLAGS_YAW_LOCK;
    lockFlags = lockFlags & ~GIMBAL_DEVICE_FLAGS_RETRACT;
    lockFlags = lockFlags & ~GIMBAL_DEVICE_FLAGS_NEUTRAL;
    lockFlags = lockFlags & ~(1 << 14);

    std::cout << "[  INFO  ] setGimbalMode(0x" << std::hex << lockFlags << std::dec
            << "): sending every 200 ms (LOCK)\n";

    bool locked = setGimbalModeAndWait(lockFlags, "LOCK_MODE", 5000, &last);

    EXPECT_TRUE(locked)
        << "gimbal did not report LOCK_MODE (last mode " << last.mode << ")";

    std::cout << "[  INFO  ] mode reported: " << last.mode
            << ", flags 0x" << std::hex << g_payload->getGimbalDeviceStatusFlags() << std::dec << "\n";
    // 
    std::cout << "[  INFO  ] drive pitch=20 deg/s, yaw=-50 deg/s for 3 s\n";
    driveSpeedFor(20, -50, 3000);

    EXPECT_TRUE(recenterPitch(last.yaw, 6000)) << "pitch did not re-center (LOCK)";
    EXPECT_TRUE(recenterYaw(last.pitch, 6000)) << "yaw did not re-center (LOCK)";

    std::cout << "[  INFO  ] move to pitch=40, yaw=-90\n";
    g_payload->setGimbalSpeed(40, 0, -90, INPUT_ANGLE);
    EXPECT_TRUE(waitAttitudeNear(40.0, -90.0, 4.0, 6000, &last))
        << "gimbal did not reach pitch=40 yaw=-90 (last pitch=" << last.pitch << " yaw=" << last.yaw << ")";

    EXPECT_TRUE(recenterPitch(last.yaw, 6000)) << "pitch did not re-center after move (LOCK)";
    EXPECT_TRUE(recenterYaw(last.pitch, 6000)) << "yaw did not re-center after move (LOCK)";

    // ---- back to FOLLOW -------------------------------------------------
    flags = g_payload->getGimbalDeviceStatusFlags();
    uint16_t followFlags2 = flags;
    followFlags2 = followFlags2 & ~GIMBAL_DEVICE_FLAGS_YAW_LOCK;
    followFlags2 = followFlags2 & ~GIMBAL_DEVICE_FLAGS_RETRACT;
    followFlags2 = followFlags2 & ~GIMBAL_DEVICE_FLAGS_NEUTRAL;
    followFlags2 = followFlags2 & ~(1 << 14);

    std::cout << "[  INFO  ] setGimbalMode(0x" << std::hex << followFlags2 << std::dec
              << "): sending every 200 ms (FOLLOW, second pass)\n";

    EXPECT_TRUE(setGimbalModeAndWait(followFlags2, "FOLLOW_MODE", 5000, &last))
        << "gimbal did not report FOLLOW_MODE on return (last mode " << last.mode << ")";

    std::cout << "[  INFO  ] mode reported: " << last.mode
              << ", flags 0x" << std::hex << g_payload->getGimbalDeviceStatusFlags() << std::dec << "\n";

    std::cout << "[  INFO  ] drive pitch=-20 deg/s, yaw=50 deg/s for 3 s\n";
    driveSpeedFor(-20, 50, 3000);

    EXPECT_TRUE(recenterYaw(last.pitch, 6000)) << "yaw did not re-center (second FOLLOW pass)";
    EXPECT_TRUE(recenterPitch(last.yaw, 6000)) << "pitch did not re-center (second FOLLOW pass)";

    std::cout << "[  INFO  ] move to pitch=-40, yaw=90\n";
    g_payload->setGimbalSpeed(-40, 0, 90, INPUT_ANGLE);
    EXPECT_TRUE(waitAttitudeNear(-40.0, 90.0, 4.0, 6000, &last))
        << "gimbal did not reach pitch=-40 yaw=90 on second pass";

    EXPECT_TRUE(recenterPitch(last.yaw, 6000)) << "pitch did not re-center after second move";
    EXPECT_TRUE(recenterYaw(last.pitch, 6000)) << "yaw did not re-center after second move";

    // ---- MAPPING ----------------------------------------------------------
    flags = g_payload->getGimbalDeviceStatusFlags();
    uint16_t mappingFlags = flags;
    mappingFlags = mappingFlags | (1 << 14);
    mappingFlags = mappingFlags & ~GIMBAL_DEVICE_FLAGS_RETRACT;
    mappingFlags = mappingFlags & ~GIMBAL_DEVICE_FLAGS_NEUTRAL;

    std::cout << "[  INFO  ] setGimbalMode(0x" << std::hex << mappingFlags << std::dec
              << "): sending every 200 ms (MAPPING)\n";

    EXPECT_TRUE(setGimbalModeAndWait(mappingFlags, "MAPPING_MODE", 5000, &last))
        << "gimbal did not report MAPPING_MODE (last mode " << last.mode << ")";

    std::cout << "[  INFO  ] mode reported: " << last.mode
              << ", flags 0x" << std::hex << g_payload->getGimbalDeviceStatusFlags() << std::dec << "\n";

    // ---- RESET (return home) -----------------------------------------------
    // NEUTRAL is edge-triggered on this gimbal (set, hold ~100 ms, then
    // clear -- not held on), same as SetGimbalMode_ReturnHome. Loop the
    // whole pulse, not just the final "cleared" state, so a dropped command
    // doesn't leave the test stuck.
    flags = g_payload->getGimbalDeviceStatusFlags();
    uint16_t homeSetFlags = flags;
    homeSetFlags = homeSetFlags | GIMBAL_DEVICE_FLAGS_NEUTRAL;
    homeSetFlags = homeSetFlags & ~GIMBAL_DEVICE_FLAGS_RETRACT;
    homeSetFlags = homeSetFlags & ~(1 << 14);
    uint16_t homeClearFlags = homeSetFlags & ~GIMBAL_DEVICE_FLAGS_NEUTRAL;

    std::cout << "[  INFO  ] pulsing NEUTRAL: set 0x" << std::hex << homeSetFlags
              << ", then clear 0x" << homeClearFlags << std::dec << " (RESET)\n";

    bool home = false;
    const auto resetDeadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(5000);
    while (std::chrono::steady_clock::now() < resetDeadline) {
        g_payload->setGimbalMode(homeSetFlags);
        sleepMs(100);
        g_payload->setGimbalMode(homeClearFlags);
        if (waitGimbalMode("RESET_MODE", 700, &last)) {
            home = true;
            break;
        }
    }
    EXPECT_TRUE(home)
        << "gimbal did not report RESET_MODE (last mode " << last.mode << ")";

    std::cout << "[  INFO  ] mode reported: " << last.mode
              << ", flags 0x" << std::hex << g_payload->getGimbalDeviceStatusFlags() << std::dec << "\n";

    // ---- OFF ----------------------------------------------------------------
    flags = g_payload->getGimbalDeviceStatusFlags();
    uint16_t offFlags = flags;
    offFlags = offFlags | GIMBAL_DEVICE_FLAGS_RETRACT;
    offFlags = offFlags & ~GIMBAL_DEVICE_FLAGS_NEUTRAL;
    offFlags = offFlags & ~(1 << 14);

    std::cout << "[  INFO  ] setGimbalMode(0x" << std::hex << offFlags << std::dec
              << "): sending every 200 ms (OFF)\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    EXPECT_TRUE(setGimbalModeAndWait(offFlags, "OFF_MODE", 5000, &last))
        << "gimbal did not report OFF_MODE (last mode " << last.mode << ")";

    std::cout << "[  INFO  ] mode reported: " << last.mode
              << ", flags 0x" << std::hex << g_payload->getGimbalDeviceStatusFlags() << std::dec << "\n";

    // Safety net: TearDown() restores via this same 284-flags path
    // (setGimbalModeAndWait), so this leaves the gimbal in a state it can
    // actually drive out of before cleanup runs.
    std::cout << "[  INFO  ] restoring mode FOLLOW before teardown\n";
    flags = g_payload->getGimbalDeviceStatusFlags();
    uint16_t restoreFlags = flags;
    restoreFlags = restoreFlags & ~GIMBAL_DEVICE_FLAGS_YAW_LOCK;
    restoreFlags = restoreFlags & ~GIMBAL_DEVICE_FLAGS_RETRACT;
    restoreFlags = restoreFlags & ~GIMBAL_DEVICE_FLAGS_NEUTRAL;
    restoreFlags = restoreFlags & ~(1 << 14);
    setGimbalModeAndWait(restoreFlags, "FOLLOW_MODE", 5000, &last);
}