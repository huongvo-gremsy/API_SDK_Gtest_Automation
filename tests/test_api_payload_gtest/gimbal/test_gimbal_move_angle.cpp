/** @file test_gimbal_move_angle.cpp */
#include "gimbal_example_test_helpers.h"

namespace get = gimbal_example_test;
namespace gt = gimbal_test;

class GimbalMoveAngleExampleTest : public get::GimbalMotionFixture {};

TEST_F(GimbalMoveAngleExampleTest, MAVLinkV1_PitchAndYaw_ReachesTarget) {
    const double targetPitch = boundedPitchOffset(
        original_.pitch < 65.0 ? 10.0 : -10.0);
    const double targetYaw = boundedYawOffset(15.0);

    ASSERT_TRUE(gt::commandAccepted(MAV_CMD_DO_MOUNT_CONTROL, [=] {
        g_payload->setGimbalMove_MAVLinkV1(
            static_cast<float>(targetPitch), 0,
            static_cast<float>(targetYaw), INPUT_ANGLE);
    })) << "MAVLink-v1 mount-control command was not accepted.";

    EXPECT_TRUE(gt::waitForAttitudeNearPassive(
        targetPitch, targetYaw, 4.0, 9000))
        << "The v1 angle command was accepted but did not reach its target.";
}

TEST_F(GimbalMoveAngleExampleTest, MAVLinkV1_SecondFartherPosition_ReachesTarget) {
    const double pitchDirection = original_.pitch < 55.0 ? 1.0 : -1.0;
    const double targetPitch = boundedPitchOffset(20.0 * pitchDirection);
    const double targetYaw = boundedYawOffset(-30.0);

    ASSERT_TRUE(gt::commandAccepted(MAV_CMD_DO_MOUNT_CONTROL, [=] {
        g_payload->setGimbalMove_MAVLinkV1(
            static_cast<float>(targetPitch), 0,
            static_cast<float>(targetYaw), INPUT_ANGLE);
    }));
    EXPECT_TRUE(gt::waitForAttitudeNearPassive(
        targetPitch, targetYaw, 5.0, 10000));
}

TEST_F(GimbalMoveAngleExampleTest, MAVLinkV2_PitchAndYaw_ReachesTarget) {
    const double targetPitch = boundedPitchOffset(
        original_.pitch < 65.0 ? 8.0 : -8.0);
    const double targetYaw = boundedYawOffset(20.0);

    // GIMBAL_DEVICE_SET_ATTITUDE is a message, not COMMAND_LONG, so physical
    // telemetry is its verification channel rather than COMMAND_ACK.
    g_payload->setGimbalSpeed(static_cast<float>(targetPitch), 0,
                              static_cast<float>(targetYaw), INPUT_ANGLE);
    EXPECT_TRUE(gt::waitForAttitudeNearPassive(
        targetPitch, targetYaw, 4.0, 9000));
}
