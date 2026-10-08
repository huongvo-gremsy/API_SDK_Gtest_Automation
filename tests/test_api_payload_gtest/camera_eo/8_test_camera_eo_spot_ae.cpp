#include "api_camera_eo_test_helper.h"

namespace cet = camera_eo_test;

class CameraEoSpotAeTest : public cet::CameraEoTest {};

TEST_F(CameraEoSpotAeTest, SetCameraExtSettingsSpotAE_DisplayModePosition) {
#if !defined(ORUSL)
    GTEST_SKIP() << "Spot AE extended settings are ORUSL-only.";
#else
    ASSERT_TRUE(cet::setCameraParam(PAYLOAD_CAMERA_VIEW_SRC,
                                    PAYLOAD_CAMERA_VIEW_EO));

    EXPECT_TRUE(cet::sendAndAcceptIfAcked(
        MAV_CMD_USER_4,
        [] {
            g_payload->setCameraExtSettings_SpotAE_Display(1);
        }))
        << "Spot AE display command was rejected.";

    EXPECT_TRUE(cet::sendAndAcceptIfAcked(
        MAV_CMD_USER_4,
        [] {
            g_payload->setCameraExtSettings_SpotAE_Mode(2);
        }))
        << "Spot AE mode command was rejected.";

    EXPECT_TRUE(cet::sendAndAcceptIfAcked(
        MAV_CMD_USER_4,
        [] {
            g_payload->setCameraExtSettings_SpotAE_Position(6, 6, 2, 2);
        }))
        << "Spot AE position command was rejected.";
#endif
}
