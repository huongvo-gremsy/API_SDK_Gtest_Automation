/**
 * Sheet row 6: Hieu chuan gyro / accel / motor; tim home
 *   sendPayloadGimbalCalibGyro() / CalibAccel() / CalibMotor() / SearchHome()
 * Support: VIO x | ORUSL x | MB1 ? | ZIO ?
 * Example: examples/gimbal_do_calib.cpp
 */

#include "gb_test_helpers.h"

using namespace gb;

// Calibration moves the gimbal and must run on a cleared bench, so on top of
// the sheet columns these tests also need PAYLOAD_TEST_GIMBAL_CALIB=1.
// The pass/fail decision is the COMMAND_ACK; the example only waits for it too.
class GB_Calibration : public GimbalTest {
protected:
    void SetUp() override {
        GB_SKIP_UNLESS_SUPPORTED("xx??");

        if (!runGimbalCalib()) {
            GTEST_SKIP() << "calibration moves the gimbal; set PAYLOAD_TEST_GIMBAL_CALIB=1 to run it";
        }

        GimbalTest::SetUp();
    }
};
// Hiệu chuẩn gyro / accel / motor; tìm home	"sendPayloadGimbalCalibGyro()
// sendPayloadGimbalCalibAccel()
// sendPayloadGimbalCalibMotor()
// sendPayloadGimbalSearchHome()"	x	x	?	?

// Check that sendPayloadGimbalCalibGyro() is accepted.
TEST_F(GB_Calibration, SendPayloadGimbalCalibGyro) {
    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_GIMBAL_REQUEST_AXIS_CALIBRATION,
            [] {
                g_payload->sendPayloadGimbalCalibGyro();
            },
            "calib gyro",
            10000
        )
    ) << "sendPayloadGimbalCalibGyro() was not accepted";

    std::cout << "[  INFO  ] sendPayloadGimbalCalibGyro(): ACK accepted\n";
}

// Check that sendPayloadGimbalCalibAccel() is accepted.
TEST_F(GB_Calibration, SendPayloadGimbalCalibAccel) {
    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_GIMBAL_REQUEST_AXIS_CALIBRATION,
            [] {
                g_payload->sendPayloadGimbalCalibAccel();
            },
            "calib accel",
            10000
        )
    ) << "sendPayloadGimbalCalibAccel() was not accepted";

    std::cout << "[  INFO  ] sendPayloadGimbalCalibAccel(): ACK accepted\n";
}

// Check that sendPayloadGimbalCalibMotor() is accepted.
TEST_F(GB_Calibration, SendPayloadGimbalCalibMotor) {
    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_DO_SET_HOME,
            [] {
                g_payload->sendPayloadGimbalCalibMotor();
            },
            "calib motor",
            10000
        )
    ) << "sendPayloadGimbalCalibMotor() was not accepted";

    std::cout << "[  INFO  ] sendPayloadGimbalCalibMotor(): ACK accepted\n";
}

// Check that sendPayloadGimbalSearchHome() is accepted and telemetry keeps coming.
TEST_F(GB_Calibration, SendPayloadGimbalSearchHome) {
    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_DO_SET_HOME,
            [] {
                g_payload->sendPayloadGimbalSearchHome();
            },
            "search home",
            10000
        )
    ) << "sendPayloadGimbalSearchHome() was not accepted";

    std::cout << "[  INFO  ] sendPayloadGimbalSearchHome(): ACK accepted\n";

    Attitude attitude;

    EXPECT_TRUE(readAttitude(attitude, 15000))
        << "no attitude telemetry during the home search";

    std::cout << "[  INFO  ] attitude during home search: pitch=" << attitude.pitch
              << " yaw=" << attitude.yaw << "\n";
}
