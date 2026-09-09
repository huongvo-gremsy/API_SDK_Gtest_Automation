/** @file test_gimbal_move_speed.cpp */
#include "gimbal_example_test_helpers.h"

#include <chrono>
#include <thread>

namespace get = gimbal_example_test;
namespace gt = gimbal_test;

class GimbalMoveSpeedExampleTest : public get::GimbalMotionFixture {
protected:
    void SetUp() override {
        GimbalMotionFixture::SetUp();
        if (::testing::Test::HasFatalFailure()) return;
        ASSERT_TRUE(gt::setControlParamAndVerify(
            PAYLOAD_CAMERA_RC_MODE, PAYLOAD_CAMERA_RC_MODE_STANDARD));
        // RC_MODE writes alter the SDK's cached mode; set FOLLOW last so the
        // outgoing v2 flags use the intended frame.
        ASSERT_TRUE(gt::setControlParamAndVerify(
            PAYLOAD_CAMERA_GIMBAL_MODE,
            PAYLOAD_CAMERA_GIMBAL_MODE_FOLLOW));
    }
};

TEST_F(GimbalMoveSpeedExampleTest, ExampleFlow_RightThenLeft20DegPerSecond) {
    gt::Attitude start;
    ASSERT_TRUE(gt::getAttitude(start));

    g_payload->setGimbalSpeed(0, 0, 20, INPUT_SPEED);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    g_payload->setGimbalSpeed(0, 0, 0, INPUT_SPEED);
    gt::Attitude afterPositive;
    ASSERT_TRUE(gt::getAttitude(afterPositive));
    const double positiveDelta =
        gt::signedAngleDifference(afterPositive.yaw, start.yaw);
    EXPECT_GT(std::fabs(positiveDelta), 5.0)
        << "Positive yaw-speed command produced no meaningful movement.";

    g_payload->setGimbalSpeed(0, 0, -20, INPUT_SPEED);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    g_payload->setGimbalSpeed(0, 0, 0, INPUT_SPEED);
    gt::Attitude afterNegative;
    ASSERT_TRUE(gt::getAttitude(afterNegative));
    const double negativeDelta =
        gt::signedAngleDifference(afterNegative.yaw, afterPositive.yaw);
    EXPECT_GT(std::fabs(negativeDelta), 5.0)
        << "Negative yaw-speed command produced no meaningful movement.";
    EXPECT_LT(positiveDelta * negativeDelta, 0.0)
        << "Positive and negative speed commands did not move in opposite directions.";
}

TEST_F(GimbalMoveSpeedExampleTest, ZeroSpeed_HoldsYawStable) {
    g_payload->setGimbalSpeed(0, 0, 12, INPUT_SPEED);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    g_payload->setGimbalSpeed(0, 0, 0, INPUT_SPEED);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    gt::Attitude stopped;
    ASSERT_TRUE(gt::getAttitude(stopped));
    std::this_thread::sleep_for(std::chrono::seconds(1));
    gt::Attitude later;
    ASSERT_TRUE(gt::getAttitude(later));
    EXPECT_LT(gt::angleDifference(later.yaw, stopped.yaw), 3.0)
        << "Yaw continued moving after the zero-speed command.";
}

TEST_F(GimbalMoveSpeedExampleTest, PitchPositiveThenNegative_MoveOpposite) {
    gt::Attitude start;
    ASSERT_TRUE(gt::getAttitude(start));
    g_payload->setGimbalSpeed(10, 0, 0, INPUT_SPEED);
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    g_payload->setGimbalSpeed(0, 0, 0, INPUT_SPEED);
    gt::Attitude afterPositive;
    ASSERT_TRUE(gt::getAttitude(afterPositive));
    const double positiveDelta = afterPositive.pitch - start.pitch;

    g_payload->setGimbalSpeed(-10, 0, 0, INPUT_SPEED);
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    g_payload->setGimbalSpeed(0, 0, 0, INPUT_SPEED);
    gt::Attitude afterNegative;
    ASSERT_TRUE(gt::getAttitude(afterNegative));
    const double negativeDelta = afterNegative.pitch - afterPositive.pitch;

    EXPECT_GT(std::fabs(positiveDelta), 2.0);
    EXPECT_GT(std::fabs(negativeDelta), 2.0);
    EXPECT_LT(positiveDelta * negativeDelta, 0.0);
}
