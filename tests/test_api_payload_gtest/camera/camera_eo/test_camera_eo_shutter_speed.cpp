/**
 * @file test_camera_eo_shutter_speed.cpp
 * @brief EO shutter tests based on camera_eo_set_shutter_speed.cpp.
 */

#include "camera_eo_test_helpers.h"

namespace cet = camera_eo_test;

class CameraEoShutterSpeedTest : public cet::CameraEoTest {
protected:
    void SetUp() override {
        CameraEoTest::SetUp();
        if (::testing::Test::IsSkipped() || ::testing::Test::HasFatalFailure()) return;
#if defined(VIO) || defined(ZIO)
        // Shutter speed is an EO-only setting, so verify the active source
        // before reading or changing its exposure parameters.
        ASSERT_TRUE(cet::setUint32Param(PAYLOAD_CAMERA_VIEW_SRC,
                                        PAYLOAD_CAMERA_VIEW_EO));
        ASSERT_TRUE(cet::getCameraSettingByID(PAYLOAD_CAMERA_VIDEO_AUTO_EXPOSURE,
                                          originalExposure_, 3000));
        ASSERT_TRUE(cet::getCameraSettingByID(PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED,
                                          originalShutter_, 3000));
        haveSettings_ = true;

        // Shutter-priority exposure mode permits manual shutter control.
        ASSERT_TRUE(cet::setUint32Param(
            PAYLOAD_CAMERA_VIDEO_AUTO_EXPOSURE,
            PAYLOAD_CAMERA_VIDEO_EXPOSURE_SHUTTER));
#else
        GTEST_SKIP() << "EO shutter parameters are supported on VIO/ZIO.";
#endif
    }

    void TearDown() override {
#if defined(VIO) || defined(ZIO)
        if (haveSettings_) {
            EXPECT_TRUE(cet::setUint32Param(
                PAYLOAD_CAMERA_VIDEO_AUTO_EXPOSURE,
                PAYLOAD_CAMERA_VIDEO_EXPOSURE_SHUTTER));
            EXPECT_TRUE(cet::setUint32Param(
                PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED,
                static_cast<uint32_t>(originalShutter_)));
            EXPECT_TRUE(cet::setUint32Param(
                PAYLOAD_CAMERA_VIDEO_AUTO_EXPOSURE,
                static_cast<uint32_t>(originalExposure_)));
        }
#endif
        CameraEoTest::TearDown();
    }

    double originalExposure_ = 0;
    double originalShutter_ = 0;
    bool haveSettings_ = false;
};

#if defined(VIO) || defined(ZIO)
TEST_F(CameraEoShutterSpeedTest, ExampleFlow_OneTenthThenOneThousandth) {
    ASSERT_TRUE(cet::setUint32Param(PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED,
                                    PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_10));
    double actual = -1;
    ASSERT_TRUE(cet::getCameraSettingByID(PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED,
                                      actual, 3000));
    EXPECT_EQ(actual, PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_10);

    ASSERT_TRUE(cet::setUint32Param(PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED,
                                    PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_1000));
    ASSERT_TRUE(cet::getCameraSettingByID(PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED,
                                      actual, 3000));
    EXPECT_EQ(actual, PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_1000);
}

TEST_F(CameraEoShutterSpeedTest, AllSupportedValues_SetAndReadBack) {
    struct ShutterSpeedCase {
        const char* name;
        uint32_t value;
    };

    const ShutterSpeedCase speeds[] = {
        {"1/1", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_1},
        {"2/3", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_2_3},
        {"1/2", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_2},
        {"1/3", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_3},
        {"1/4", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_4},
        {"1/6", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_6},
        {"1/8", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_8},
        {"1/10", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_10},
        {"1/15", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_15},
        {"1/20", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_20},
        {"1/30", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_30},
        {"1/50", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_50},
        {"1/60", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_60},
        {"1/90", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_90},
        {"1/100", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_100},
        {"1/125", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_125},
        {"1/180", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_180},
        {"1/250", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_250},
        {"1/350", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_350},
        {"1/500", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_500},
        {"1/725", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_725},
        {"1/1000", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_1000},
        {"1/1500", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_1500},
        {"1/2000", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_2000},
        {"1/3000", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_3000},
        {"1/4000", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_4000},
        {"1/6000", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_6000},
        {"1/10000", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_10000},
    };

    for (const ShutterSpeedCase& speed : speeds) {
        SCOPED_TRACE(::testing::Message()
                     << "shutter speed=" << speed.name
                     << ", macro value=" << speed.value);

        const bool setSucceeded = cet::setUint32Param(
            PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED, speed.value);
        EXPECT_TRUE(setSucceeded)
            << "Could not set shutter speed " << speed.name << ".";
        if (!setSucceeded) continue;

        double actual = -1;
        const bool readSucceeded = cet::getCameraSettingByID(
            PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED, actual, 3000);
        EXPECT_TRUE(readSucceeded)
            << "Could not read back shutter speed " << speed.name << ".";
        if (readSucceeded) {
            EXPECT_EQ(actual, speed.value)
                << "Unexpected readback for shutter speed " << speed.name
                << ".";
        }
    }
}

#endif

TEST_F(CameraEoShutterSpeedTest, Manual_SetCameraEoShutterSpeed) {
    g_payload->setPayloadCameraParam(
        PAYLOAD_CAMERA_VIEW_SRC, PAYLOAD_CAMERA_VIEW_EO, PARAM_TYPE_UINT32);
}
