/**
 * Sheet row 2: Doi che do gimbal - setGimbalMode()
 * Support: VIO x | ORUSL x | MB1 x | ZIO x
 * Example: examples/gimbal_set_mode.cpp
 */

#include "gb_test_helpers.h"

using namespace gb;

// The mode is a set of GIMBAL_DEVICE_FLAGS bits sent in GIMBAL_DEVICE_SET_ATTITUDE.
// The gimbal reports the flags back and the SDK turns them into a mode string.
//   LOCK   = YAW_LOCK bit set
//   FOLLOW = YAW_LOCK bit clear (and no RETRACT / NEUTRAL / mapping bit)
// OFF (RETRACT) and RESET (NEUTRAL) move the gimbal, so they are not tested here.
class GB_Mode : public GimbalTest {
protected:
    void SetUp() override {
        GB_SKIP_UNLESS_SUPPORTED("xxxx");
        #if defined (MB1)
        // Set Gimbal Flag Onboard Computer
        ASSERT_TRUE(gb::setParam(PAYLOAD_CAMERA_GIMBAL_FW_FLAG, 1)) << "Failed to set "<< PAYLOAD_CAMERA_GIMBAL_FW_FLAG<< " = 1";
        #endif
        GimbalTest::SetUp();
    }
    void TearDown() override {
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        GimbalTest::TearDown();
    }
};

// Check that setGimbalMode() switches to LOCK and back to FOLLOW.
TEST_F(GB_Mode, SetGimbalMode_LockThenFollow) {
    uint16_t flags = g_payload->getGimbalDeviceStatusFlags();

    std::cout << "[  INFO  ] flags before: 0x" << std::hex << flags << std::dec
              << " (mode " << original_.mode << ")\n";

    // LOCK: set YAW_LOCK, clear RETRACT, NEUTRAL and the mapping bit, like the example.
    uint16_t lockFlags = flags;

    lockFlags = lockFlags | GIMBAL_DEVICE_FLAGS_YAW_LOCK;
    lockFlags = lockFlags & ~GIMBAL_DEVICE_FLAGS_RETRACT;
    lockFlags = lockFlags & ~GIMBAL_DEVICE_FLAGS_NEUTRAL;
    lockFlags = lockFlags & ~(1 << 14);

    std::cout << "[  INFO  ] setGimbalMode(0x" << std::hex << lockFlags << std::dec << "): sending every 200 ms (LOCK)\n";

    Attitude last;

    bool locked = setGimbalModeAndWait(lockFlags, "LOCK_MODE", 5000, &last);

    EXPECT_TRUE(locked)
        << "gimbal did not report LOCK_MODE (last mode " << last.mode << ")";

    std::cout << "[  INFO  ] mode reported: " << last.mode
              << ", flags 0x" << std::hex << g_payload->getGimbalDeviceStatusFlags() << std::dec << "\n";

    sleepMs(1000);

    // FOLLOW: clear YAW_LOCK.
    uint16_t followFlags = lockFlags & ~GIMBAL_DEVICE_FLAGS_YAW_LOCK;

    std::cout << "[  INFO  ] setGimbalMode(0x" << std::hex << followFlags << std::dec << "): sending every 200 ms (FOLLOW)\n";

    bool following = setGimbalModeAndWait(followFlags, "FOLLOW_MODE", 5000, &last);

    EXPECT_TRUE(following)
        << "gimbal did not report FOLLOW_MODE (last mode " << last.mode << ")";

    std::cout << "[  INFO  ] mode reported: " << last.mode
              << ", flags 0x" << std::hex << g_payload->getGimbalDeviceStatusFlags() << std::dec << "\n";
}

TEST_F(GB_Mode, SetGimbalMode_Mapping) {
    uint16_t flags = g_payload->getGimbalDeviceStatusFlags();

    std::cout << "[  INFO  ] flags before: 0x" << std::hex << flags << std::dec
              << " (mode " << original_.mode << ")\n";

    // MAPPING: set the mapping bit (bit 14), clear RETRACT and NEUTRAL, like the example.
    uint16_t mappingFlags = flags;

    mappingFlags = mappingFlags | (1 << 14);
    mappingFlags = mappingFlags & ~GIMBAL_DEVICE_FLAGS_RETRACT;
    mappingFlags = mappingFlags & ~GIMBAL_DEVICE_FLAGS_NEUTRAL;

    std::cout << "[  INFO  ] setGimbalMode(0x" << std::hex << mappingFlags << std::dec
              << "): sending every 200 ms (MAPPING)\n";

    Attitude last;

    bool mapped = setGimbalModeAndWait(mappingFlags, "MAPPING_MODE", 5000, &last);

    EXPECT_TRUE(mapped)
        << "gimbal did not report MAPPING_MODE (last mode " << last.mode << ")";

    std::cout << "[  INFO  ] mode reported: " << last.mode
              << ", flags 0x" << std::hex << g_payload->getGimbalDeviceStatusFlags() << std::dec << "\n";
}

TEST_F(GB_Mode, SetGimbalMode_Off) {
    uint16_t flags = g_payload->getGimbalDeviceStatusFlags();

    std::cout << "[  INFO  ] flags before: 0x" << std::hex << flags << std::dec
              << " (mode " << original_.mode << ")\n";

    // OFF: set RETRACT, clear the mapping bit and NEUTRAL, like the example.
    uint16_t offFlags = flags;

    offFlags = offFlags | GIMBAL_DEVICE_FLAGS_RETRACT;
    offFlags = offFlags & ~(1 << 14);
    offFlags = offFlags & ~GIMBAL_DEVICE_FLAGS_NEUTRAL;

    std::cout << "[  INFO  ] setGimbalMode(0x" << std::hex << offFlags << std::dec
              << "): sending every 200 ms (OFF)\n";

    Attitude last;

    bool off = setGimbalModeAndWait(offFlags, "OFF_MODE", 5000, &last);
    
    EXPECT_TRUE(off)
        << "gimbal did not report OFF_MODE (last mode " << last.mode << ")";

    std::cout << "[  INFO  ] mode reported: " << last.mode
              << ", flags 0x" << std::hex << g_payload->getGimbalDeviceStatusFlags() << std::dec << "\n";
    
}


TEST_F(GB_Mode, SetGimbalMode_ReturnHome) {
    uint16_t flags = g_payload->getGimbalDeviceStatusFlags();

    std::cout << "[  INFO  ] flags before: 0x" << std::hex << flags << std::dec
              << " (mode " << original_.mode << ")\n";

    // RETURN HOME: unlike YAW_LOCK, NEUTRAL is edge-triggered on this gimbal
    // (per the vendor example: set, hold ~100 ms, then clear — not held on).
    // Loop the whole pulse, not just the final "cleared" state, so a dropped
    // command doesn't leave the test stuck.
    uint16_t homeSetFlags = flags;
    homeSetFlags = homeSetFlags | GIMBAL_DEVICE_FLAGS_NEUTRAL;
    homeSetFlags = homeSetFlags & ~GIMBAL_DEVICE_FLAGS_RETRACT;
    homeSetFlags = homeSetFlags & ~(1 << 14);

    uint16_t homeClearFlags = homeSetFlags & ~GIMBAL_DEVICE_FLAGS_NEUTRAL;

    std::cout << "[  INFO  ] pulsing NEUTRAL: set 0x" << std::hex << homeSetFlags
              << ", then clear 0x" << homeClearFlags << std::dec << " (RETURN HOME)\n";

    Attitude last;
    bool home = false;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(5000);
    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->setGimbalMode(homeSetFlags);
        sleepMs(100);
        g_payload->setGimbalMode(homeClearFlags);
        if (waitGimbalMode("RESET_MODE", 700, &last)) {
            home = true;
            break;
        }
    }

    EXPECT_TRUE(home)
        << "gimbal did not report RESET_MODE after return-home (last mode " << last.mode << ")";

    std::cout << "[  INFO  ] mode reported: " << last.mode
              << ", flags 0x" << std::hex << g_payload->getGimbalDeviceStatusFlags() << std::dec << "\n";
}