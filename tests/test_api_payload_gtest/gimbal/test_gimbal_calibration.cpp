/** @file test_gimbal_calibration.cpp */
#include "gimbal_test_helpers.h"

namespace gt = gimbal_test;
class GimbalCalibrationTest : public PayloadTest {};

TEST_F(GimbalCalibrationTest, DISABLED_GyroCalibration_StartsOrCompletes) {
    EXPECT_TRUE(gt::commandAccepted(MAV_CMD_GIMBAL_REQUEST_AXIS_CALIBRATION, [] {
        g_payload->sendPayloadGimbalCalibGyro();
    }, 10000));
}

TEST_F(GimbalCalibrationTest, DISABLED_AccelerometerCalibration_StartsOrCompletes) {
    EXPECT_TRUE(gt::commandAccepted(MAV_CMD_GIMBAL_REQUEST_AXIS_CALIBRATION, [] {
        g_payload->sendPayloadGimbalCalibAccel();
    }, 10000));
}

TEST_F(GimbalCalibrationTest, DISABLED_MotorCalibration_StartsOrCompletes) {
    EXPECT_TRUE(gt::commandAccepted(MAV_CMD_DO_SET_HOME, [] {
        g_payload->sendPayloadGimbalCalibMotor();
    }, 10000));
}
