/**
 * Sheet row 10: Dua rieng truc yaw / pitch ve tam - setGimbalSpeed() (setRecenterYaw / setRecenterPitch)
 * Support: VIO x | ORUSL x | MB1 x | ZIO x
 * Example: examples/gimbal_control_example.cpp
 */

#include "gb_test_helpers.h"

using namespace gb;

// setRecenterYaw() / setRecenterPitch() are helpers of the example class, not
// SDK APIs. They do: stop (speed 0), wait 1.5 s, then send an INPUT_ANGLE
// command with only the wanted axis set to 0. The tests repeat those steps.
class GB_Recenter : public GimbalTest {
protected:
    void SetUp() override {
        GB_SKIP_UNLESS_SUPPORTED("xxxx");

        GimbalTest::SetUp();
    }
};

// Check that yaw returns to 0 while pitch stays where it is.
TEST_F(GB_Recenter, SetRecenterYaw) {
    // Move away from center first.
    double pitch = pitchTarget(-15.0);
    double yaw = yawTarget(30.0);

    g_payload->setGimbalSpeed(static_cast<float>(pitch), 0, static_cast<float>(yaw), INPUT_ANGLE);

    Attitude last;

    ASSERT_TRUE(waitAttitudeNear(pitch, yaw, 4.0, 9000, &last))
        << "could not move to pitch=" << pitch << " yaw=" << yaw << " before the recenter";

    std::cout << "[  INFO  ] moved to pitch=" << last.pitch << " yaw=" << last.yaw << "\n";

    // Recenter yaw: stop, wait 1.5 s, then yaw = 0 with the current pitch.
    g_payload->setGimbalSpeed(0, 0, 0, INPUT_SPEED);

    sleepMs(1500);

    Attitude now;

    ASSERT_TRUE(readAttitude(now));

    g_payload->setGimbalSpeed(static_cast<float>(now.pitch), 0, 0, INPUT_ANGLE);

    std::cout << "[  INFO  ] setGimbalSpeed(pitch=" << now.pitch << ", roll=0, yaw=0, INPUT_ANGLE): sent (recenter yaw)\n";

    bool centered = waitAttitudeNear(now.pitch, 0.0, 4.0, 9000, &last);

    EXPECT_TRUE(centered)
        << "yaw did not return to 0 (last pitch=" << last.pitch << " yaw=" << last.yaw << ")";

    std::cout << "[  INFO  ] after recenter yaw: pitch=" << last.pitch << " yaw=" << last.yaw << "\n";
}

// Check that pitch returns to 0 while yaw stays where it is.
TEST_F(GB_Recenter, SetRecenterPitch) {
    // Move away from center first.
    double pitch = pitchTarget(-20.0);
    double yaw = yawTarget(20.0);

    g_payload->setGimbalSpeed(static_cast<float>(pitch), 0, static_cast<float>(yaw), INPUT_ANGLE);

    Attitude last;

    ASSERT_TRUE(waitAttitudeNear(pitch, yaw, 4.0, 9000, &last))
        << "could not move to pitch=" << pitch << " yaw=" << yaw << " before the recenter";

    std::cout << "[  INFO  ] moved to pitch=" << last.pitch << " yaw=" << last.yaw << "\n";

    // Recenter pitch: stop, wait 1.5 s, then pitch = 0 with the current yaw.
    g_payload->setGimbalSpeed(0, 0, 0, INPUT_SPEED);

    sleepMs(1500);

    Attitude now;

    ASSERT_TRUE(readAttitude(now));

    g_payload->setGimbalSpeed(0, 0, static_cast<float>(now.yaw), INPUT_ANGLE);

    std::cout << "[  INFO  ] setGimbalSpeed(pitch=0, roll=0, yaw=" << now.yaw << ", INPUT_ANGLE): sent (recenter pitch)\n";

    bool centered = waitAttitudeNear(0.0, now.yaw, 4.0, 9000, &last);

    EXPECT_TRUE(centered)
        << "pitch did not return to 0 (last pitch=" << last.pitch << " yaw=" << last.yaw << ")";

    std::cout << "[  INFO  ] after recenter pitch: pitch=" << last.pitch << " yaw=" << last.yaw << "\n";
}
