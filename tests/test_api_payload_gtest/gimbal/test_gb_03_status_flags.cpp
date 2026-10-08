/**
 * Sheet row 3: Doc co trang thai gimbal - getGimbalDeviceStatusFlags()
 * Support: VIO x | ORUSL x | MB1 x | ZIO x
 * Example: (none, the flags are read inside examples/gimbal_set_mode.cpp)
 */

#include "gb_test_helpers.h"

using namespace gb;

// getGimbalDeviceStatusFlags() returns the flags the SDK saved from the last
// GIMBAL_DEVICE_ATTITUDE_STATUS packet. It does not send anything.
class GB_StatusFlags : public GimbalTest {
protected:
    void SetUp() override {
        GB_SKIP_UNLESS_SUPPORTED("xxxx");

        GimbalTest::SetUp();
    }
};

// Check that the flags are readable, decode to a known mode, and are stable.
TEST_F(GB_StatusFlags, GetGimbalDeviceStatusFlags) {
    // One attitude packet must have arrived, otherwise the flags are still 0.
    Attitude attitude;

    ASSERT_TRUE(readAttitude(attitude))
        << "no gimbal attitude telemetry";

    uint16_t flags = g_payload->getGimbalDeviceStatusFlags();

    std::cout << "[  INFO  ] flags = 0x" << std::hex << flags << std::dec << "\n";
    std::cout << "[  INFO  ]   RETRACT (off)  = " << ((flags & GIMBAL_DEVICE_FLAGS_RETRACT) != 0) << "\n";
    std::cout << "[  INFO  ]   NEUTRAL (reset)= " << ((flags & GIMBAL_DEVICE_FLAGS_NEUTRAL) != 0) << "\n";
    std::cout << "[  INFO  ]   ROLL_LOCK      = " << ((flags & GIMBAL_DEVICE_FLAGS_ROLL_LOCK) != 0) << "\n";
    std::cout << "[  INFO  ]   PITCH_LOCK     = " << ((flags & GIMBAL_DEVICE_FLAGS_PITCH_LOCK) != 0) << "\n";
    std::cout << "[  INFO  ]   YAW_LOCK       = " << ((flags & GIMBAL_DEVICE_FLAGS_YAW_LOCK) != 0) << "\n";
    std::cout << "[  INFO  ]   mapping bit 14 = " << ((flags & (1 << 14)) != 0) << "\n";
    std::cout << "[  INFO  ]   SDK mode string= " << attitude.mode << "\n";

    // The gimbal must be powered on while the tests run.
    EXPECT_EQ(flags & GIMBAL_DEVICE_FLAGS_RETRACT, 0)
        << "gimbal reports RETRACT (OFF mode)";

    // The mode string the SDK derives must match the YAW_LOCK bit.
    if ((flags & GIMBAL_DEVICE_FLAGS_YAW_LOCK) != 0) {
        EXPECT_EQ(attitude.mode, "LOCK_MODE");
    }

    if ((flags & (GIMBAL_DEVICE_FLAGS_YAW_LOCK | GIMBAL_DEVICE_FLAGS_RETRACT |
                  GIMBAL_DEVICE_FLAGS_NEUTRAL | (1 << 14))) == 0) {
        EXPECT_EQ(attitude.mode, "FOLLOW_MODE");
    }

    // A second read without any command must give the same flags.
    sleepMs(500);

    ASSERT_TRUE(readAttitude(attitude));

    uint16_t flagsAgain = g_payload->getGimbalDeviceStatusFlags();

    EXPECT_EQ(flagsAgain, flags)
        << "flags changed without a command: 0x" << std::hex << flags << " -> 0x" << flagsAgain << std::dec;

    std::cout << "[  INFO  ] flags 0.5 s later = 0x" << std::hex << flagsAgain << std::dec << "\n";
}
