/**
 * Sheet row 1: Dieu khien goc / toc do 3 truc - setGimbalSpeed() / setGimbalMove_MAVLinkV1()
 * Support: VIO x | ORUSL x | MB1 x | ZIO x
 * Example: examples/gimbal_control_example.cpp, gimbal_move_angle.cpp, gimbal_move_speed.cpp
 */

#include "gb_test_helpers.h"

using namespace gb;

// Motion is verified through the attitude the gimbal streams back.
class GB_Move : public GimbalTest {
protected:
    void SetUp() override {
        GB_SKIP_UNLESS_SUPPORTED("xxxx");

        GimbalTest::SetUp();
    }
};

// Check that setGimbalSpeed(INPUT_ANGLE) moves pitch and yaw to the target.
TEST_F(GB_Move, SetGimbalSpeed_InputAngle) {
    double pitch = pitchTarget(10.0);
    double yaw = yawTarget(-20.0);

    g_payload->setGimbalSpeed(static_cast<float>(pitch), 0, static_cast<float>(yaw), INPUT_ANGLE);

    std::cout << "[  INFO  ] setGimbalSpeed(pitch=" << pitch << ", roll=0, yaw=" << yaw << ", INPUT_ANGLE): sent\n";

    Attitude last;

    bool reached = waitAttitudeNear(pitch, yaw, 4.0, 9000, &last);

    EXPECT_TRUE(reached)
        << "gimbal did not reach pitch=" << pitch << " yaw=" << yaw
        << " (last pitch=" << last.pitch << " yaw=" << last.yaw << ")";

    std::cout << "[  INFO  ] attitude after move: pitch=" << last.pitch
              << " yaw=" << last.yaw << " (target " << pitch << " / " << yaw << ")\n";
}

// Check that setGimbalSpeed(INPUT_SPEED) turns yaw, then stops it.
TEST_F(GB_Move, SetGimbalSpeed_InputSpeed) {
    // Like the example: standard RC mode while driving by speed. TearDown() restores it.
    saveAndSet(PAYLOAD_CAMERA_RC_MODE, PAYLOAD_CAMERA_RC_MODE_STANDARD);

    Attitude before;

    ASSERT_TRUE(readAttitude(before))
        << "no gimbal attitude telemetry";

    std::cout << "[  INFO  ] yaw before: " << before.yaw << "\n";

    // Yaw to the right at 20 deg/s for 2 seconds, then stop.
    // One rate command is applied only briefly, so driveSpeedFor() repeats it every 100 ms.
    std::cout << "[  INFO  ] setGimbalSpeed(0, 0, 20, INPUT_SPEED): sending every 100 ms for 2 s\n";

    driveSpeedFor(0, 20, 2000);

    std::cout << "[  INFO  ] setGimbalSpeed(0, 0, 0, INPUT_SPEED): sent (stop)\n";

    sleepMs(1000);

    Attitude afterRight;

    ASSERT_TRUE(readAttitude(afterRight));

    EXPECT_GT(yawDifference(afterRight.yaw, before.yaw), 5.0)
        << "yaw did not move at 20 deg/s: " << before.yaw << " -> " << afterRight.yaw;

    std::cout << "[  INFO  ] yaw after 2 s right: " << afterRight.yaw << "\n";

    // The gimbal must stay still after the stop command.
    sleepMs(1500);

    Attitude afterStop;

    ASSERT_TRUE(readAttitude(afterStop));

    EXPECT_LT(yawDifference(afterStop.yaw, afterRight.yaw), 3.0)
        << "yaw kept moving after speed 0: " << afterRight.yaw << " -> " << afterStop.yaw;

    std::cout << "[  INFO  ] yaw 1.5 s after stop: " << afterStop.yaw << "\n";

    // Yaw to the left at 20 deg/s for 2 seconds, then stop.
    std::cout << "[  INFO  ] setGimbalSpeed(0, 0, -20, INPUT_SPEED): sending every 100 ms for 2 s\n";

    driveSpeedFor(0, -20, 2000);

    std::cout << "[  INFO  ] setGimbalSpeed(0, 0, 0, INPUT_SPEED): sent (stop)\n";

    sleepMs(1000);

    Attitude afterLeft;

    ASSERT_TRUE(readAttitude(afterLeft));

    EXPECT_GT(yawDifference(afterLeft.yaw, afterStop.yaw), 5.0)
        << "yaw did not move at -20 deg/s: " << afterStop.yaw << " -> " << afterLeft.yaw;

    std::cout << "[  INFO  ] yaw after 2 s left: " << afterLeft.yaw << "\n";
}

// Check that setGimbalMove_MAVLinkV1() is accepted and moves to the target.
// The example notes: payload software v3.1.x and gimbal software >= v7.8.9.
TEST_F(GB_Move, SetGimbalMove_MAVLinkV1) {
    double pitch = pitchTarget(10.0);
    double yaw = yawTarget(15.0);

    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_DO_MOUNT_CONTROL,
            [pitch, yaw] {
                g_payload->setGimbalMove_MAVLinkV1(static_cast<float>(pitch), 0,
                                                   static_cast<float>(yaw), INPUT_ANGLE);
            },
            "mount control v1"
        )
    ) << "setGimbalMove_MAVLinkV1() was not accepted";

    std::cout << "[  INFO  ] setGimbalMove_MAVLinkV1(pitch=" << pitch << ", roll=0, yaw=" << yaw << "): ACK accepted\n";

    Attitude last;

    bool reached = waitAttitudeNear(pitch, yaw, 4.0, 9000, &last);

    EXPECT_TRUE(reached)
        << "gimbal did not reach pitch=" << pitch << " yaw=" << yaw
        << " (last pitch=" << last.pitch << " yaw=" << last.yaw << ")";

    std::cout << "[  INFO  ] attitude after move: pitch=" << last.pitch
              << " yaw=" << last.yaw << " (target " << pitch << " / " << yaw << ")\n";
}
