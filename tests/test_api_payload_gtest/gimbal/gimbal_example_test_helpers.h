#ifndef PAYLOADSDK_TEST_GIMBAL_EXAMPLE_TEST_HELPERS_H_
#define PAYLOADSDK_TEST_GIMBAL_EXAMPLE_TEST_HELPERS_H_

#include "gimbal_test_helpers.h"

#include <algorithm>
#include <chrono>
#include <thread>

namespace gimbal_example_test {

namespace gt = gimbal_test;

class GimbalMotionFixture : public PayloadTest {
protected:
    void SetUp() override {
        ASSERT_TRUE(gt::getAttitude(original_, 4000))
            << "No MOUNT_ORIENTATION telemetry.";
        haveAttitude_ = true;
        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_GIMBAL_MODE,
                                          originalMode_, 3000));
        haveMode_ = true;
        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_RC_MODE,
                                          originalRcMode_, 3000));
        haveRcMode_ = true;
        ASSERT_TRUE(gt::setControlParamAndVerify(
            PAYLOAD_CAMERA_GIMBAL_MODE,
            PAYLOAD_CAMERA_GIMBAL_MODE_FOLLOW));
        // Re-sample after changing mode because yaw telemetry is absolute in
        // LOCK and relative in FOLLOW. The physical pose is unchanged, but
        // the coordinate used to restore it is now deterministic.
        ASSERT_TRUE(gt::getAttitude(original_, 4000));
    }

    void TearDown() override {
        g_payload->setGimbalSpeed(0, 0, 0, INPUT_SPEED);
        if (haveAttitude_) {
            // FOLLOW gives a deterministic relative-yaw frame for returning
            // to the original bench position.
            gt::setControlParamAndVerify(PAYLOAD_CAMERA_GIMBAL_MODE,
                                         PAYLOAD_CAMERA_GIMBAL_MODE_FOLLOW);
            g_payload->setGimbalSpeed(static_cast<float>(original_.pitch), 0,
                                      static_cast<float>(original_.yaw),
                                      INPUT_ANGLE);
            if (!gt::waitForAttitudeNearPassive(
                    original_.pitch, original_.yaw, 4.0, 8000)) {
                ADD_FAILURE() << "Could not restore original gimbal attitude.";
            }
            g_payload->setGimbalSpeed(0, 0, 0, INPUT_SPEED);
        }
        // setPayloadCameraParam updates the SDK's cached gimbal mode even for
        // RC_MODE, so restore RC first and GB_MODE last.
        if (haveRcMode_ && !gt::setControlParamAndVerify(
                PAYLOAD_CAMERA_RC_MODE,
                static_cast<uint32_t>(originalRcMode_))) {
            ADD_FAILURE() << "Could not restore original RC mode.";
        }
        if (haveMode_ && !gt::setControlParamAndVerify(
                PAYLOAD_CAMERA_GIMBAL_MODE,
                static_cast<uint32_t>(originalMode_))) {
            ADD_FAILURE() << "Could not restore original gimbal mode.";
        }
    }

    double boundedPitchOffset(double degrees) const {
        return std::max(-75.0, std::min(75.0, original_.pitch + degrees));
    }

    double boundedYawOffset(double degrees) const {
        double value = original_.yaw + degrees;
        while (value > 175.0) value -= 360.0;
        while (value < -175.0) value += 360.0;
        return value;
    }

    gt::Attitude original_;
    double originalMode_ = 0;
    double originalRcMode_ = 0;
    bool haveAttitude_ = false;
    bool haveMode_ = false;
    bool haveRcMode_ = false;
};

}  // namespace gimbal_example_test

#endif
