/**
 * @file test_camera_eo_ext_settings.cpp
 * @brief EO SpotAE extension-command API tests.
 */

#include "camera_eo_test_helpers.h"

namespace cet = camera_eo_test;

class CameraEoExtSettingsTest : public cet::CameraEoTest {};

// The SpotAE commands use MAV_CMD_USER_4 and do not expose an independent
// readback event in the SDK.  Verify an accepted/in-progress ACK when present,
// or that the EO stream remains responsive after the command is sent.
TEST_F(CameraEoExtSettingsTest, SpotAeDisplay_OnAndOff) {
    EXPECT_TRUE(cet::commandAcceptedOrEoResponsive(
        MAV_CMD_USER_4,
        [] { g_payload->setCameraExtSettings_SpotAE_Display(1); }));
    EXPECT_TRUE(cet::commandAcceptedOrEoResponsive(
        MAV_CMD_USER_4,
        [] { g_payload->setCameraExtSettings_SpotAE_Display(0); }));
}

TEST_F(CameraEoExtSettingsTest, SpotAeMode_OnAndOff) {
    EXPECT_TRUE(cet::commandAcceptedOrEoResponsive(
        MAV_CMD_USER_4,
        [] { g_payload->setCameraExtSettings_SpotAE_Mode(2); }));
    EXPECT_TRUE(cet::commandAcceptedOrEoResponsive(
        MAV_CMD_USER_4,
        [] { g_payload->setCameraExtSettings_SpotAE_Mode(3); }));
}

TEST_F(CameraEoExtSettingsTest, SpotAePosition_ValidWindow) {
    EXPECT_TRUE(cet::commandAcceptedOrEoResponsive(
        MAV_CMD_USER_4,
        [] { g_payload->setCameraExtSettings_SpotAE_Position(6, 6, 2, 2); }));
}
