/** @file test_gimbal_motion.cpp */
#include "gimbal_test_helpers.h"

#include <algorithm>
#include <chrono>
#include <thread>

namespace gt = gimbal_test;

class GimbalMotionTest : public PayloadTest {
protected:
    void SetUp() override {
        ASSERT_TRUE(gt::getAttitude(original_, 4000))
            << "No gimbal attitude telemetry.";
        haveOriginal_ = true;
    }
    void TearDown() override {
        g_payload->setGimbalSpeed(0, 0, 0, INPUT_SPEED);
        if (haveOriginal_) {
            g_payload->setGimbalSpeed(static_cast<float>(original_.pitch), 0,
                                      static_cast<float>(original_.yaw), INPUT_ANGLE);
            gt::waitForAttitudeNear(original_.pitch, original_.yaw, 3.0, 6000);
            g_payload->setGimbalSpeed(0, 0, 0, INPUT_SPEED);
        }
    }
    gt::Attitude original_;
    bool haveOriginal_ = false;
};

TEST_F(GimbalMotionTest, V2Angle_ReachesSmallPitchOffset) {
    const double targetPitch = std::max(-80.0, std::min(80.0,
        original_.pitch + (original_.pitch < 70.0 ? 5.0 : -5.0)));
    g_payload->setGimbalSpeed(static_cast<float>(targetPitch), 0,
                              static_cast<float>(original_.yaw), INPUT_ANGLE);
    EXPECT_TRUE(gt::waitForAttitudeNear(targetPitch, original_.yaw, 3.0));
}

TEST_F(GimbalMotionTest, V2Speed_ThenZeroChangesAndStopsYaw) {
    g_payload->setGimbalSpeed(0, 0, 10, INPUT_SPEED);
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    g_payload->setGimbalSpeed(0, 0, 0, INPUT_SPEED);
    gt::Attitude moved;
    ASSERT_TRUE(gt::getAttitude(moved));
    EXPECT_GT(gt::angleDifference(moved.yaw, original_.yaw), 2.0);

    std::this_thread::sleep_for(std::chrono::milliseconds(800));
    gt::Attitude stopped;
    ASSERT_TRUE(gt::getAttitude(stopped));
    EXPECT_LT(gt::angleDifference(stopped.yaw, moved.yaw), 3.0);
}

TEST_F(GimbalMotionTest, MAVLinkV1Angle_CommandAcceptedAndMoves) {
    const double targetPitch = std::max(-75.0, std::min(75.0,
        original_.pitch + (original_.pitch < 65.0 ? 5.0 : -5.0)));
    ASSERT_TRUE(gt::commandAccepted(MAV_CMD_DO_MOUNT_CONTROL, [=] {
        g_payload->setGimbalMove_MAVLinkV1(static_cast<float>(targetPitch), 0,
                                           static_cast<float>(original_.yaw),
                                           INPUT_ANGLE);
    }));
    EXPECT_TRUE(gt::waitForAttitudeNear(targetPitch, original_.yaw, 4.0));
}
