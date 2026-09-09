/** @file test_camera_param_catalog.cpp
 *  @brief Product camera-parameter catalog and representative enum tests. */

#include "camera_param_test_helpers.h"

#include <set>

class CameraParamCatalogTest : public PayloadTest {};

TEST_F(CameraParamCatalogTest, DeclaredIds_AreUniqueAndFitMavlinkField) {
    std::set<std::string> ids;
    for (const auto& config : config_param::cameraParamConfigs()) {
        ASSERT_NE(config.id, nullptr);
        const std::string id(config.id);
        EXPECT_FALSE(id.empty()) << config.name;
        EXPECT_LE(id.size(), static_cast<std::size_t>(CAM_PARAM_ID_LEN))
            << config.name;
        EXPECT_TRUE(ids.insert(id).second) << "Duplicate catalog ID " << id;
    }
}

TEST_F(CameraParamCatalogTest, EveryDeclaredParameter_IsReadable) {
    int failures = 0;
    config_param::printCameraParamConfigHeader("camera parameter catalog");
    for (const auto& config : config_param::cameraParamConfigs()) {
        double value = 0;
        if (getCameraSettingByID(config.id, value, 3000)) {
            config_param::printCameraParamConfigRow(config, value);
        } else {
            ++failures;
            config_param::printCameraParamConfigRow(config, "READ_FAIL");
        }
    }
    EXPECT_EQ(failures, 0)
        << failures << " declared parameters were not exposed by this payload.";
}

TEST_F(CameraParamCatalogTest, SourceRecordAndOsd_RepresentativeValues) {
    config_param::expectConfigAccepted(PAYLOAD_CAMERA_VIEW_SRC,
                                       PAYLOAD_CAMERA_VIEW_EO);
    config_param::expectConfigAccepted(PAYLOAD_CAMERA_VIEW_SRC,
                                       PAYLOAD_CAMERA_VIEW_IR);
    config_param::expectConfigAccepted(PAYLOAD_CAMERA_RECORD_SRC,
                                       PAYLOAD_CAMERA_RECORD_BOTH);
    config_param::expectConfigAccepted(PAYLOAD_CAMERA_VIDEO_OSD_MODE,
                                       PAYLOAD_CAMERA_VIDEO_OSD_MODE_STATUS);
}

TEST_F(CameraParamCatalogTest, ExposureAndImage_RepresentativeValues) {
    config_param::expectConfigAccepted(PAYLOAD_CAMERA_VIDEO_FLIP,
                                       PAYLOAD_CAMERA_VIDEO_FLIP_ON);
    config_param::expectConfigAccepted(PAYLOAD_CAMERA_EO_FREEZE,
                                       PAYLOAD_CAMERA_EO_FREEZE_OFF);
    config_param::expectConfigAccepted(PAYLOAD_CAMERA_VIDEO_AUTO_EXPOSURE,
                                       PAYLOAD_CAMERA_VIDEO_EXPOSURE_AUTO);
    config_param::expectConfigAccepted(PAYLOAD_CAMERA_VIDEO_DEFOG,
                                       PAYLOAD_CAMERA_VIDEO_DEFOG_OFF);
}

TEST_F(CameraParamCatalogTest, WhiteBalanceAndFocus_RepresentativeValues) {
    config_param::expectConfigAccepted(PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE,
                                       PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE_AUTO);
    config_param::expectConfigAccepted(PAYLOAD_CAMERA_VIDEO_FOCUS_MODE,
                                       PAYLOAD_CAMERA_VIDEO_FOCUS_MODE_MANUAL);
    config_param::expectConfigAccepted(PAYLOAD_CAMERA_EO_FOCUS_SPEED, 3);
}

TEST_F(CameraParamCatalogTest, ZoomAndIr_RepresentativeValues) {
    config_param::expectConfigAccepted(PAYLOAD_CAMERA_VIDEO_ZOOM_MODE,
                                       PAYLOAD_CAMERA_VIDEO_ZOOM_MODE_SUPER_RESOLUTION);
    config_param::expectConfigAccepted(
        PAYLOAD_CAMERA_VIDEO_ZOOM_SUPER_RESOLUTION_FACTOR,
        ZOOM_SUPER_RESOLUTION_1X);
    config_param::expectConfigAccepted(PAYLOAD_CAMERA_IR_ZOOM_FACTOR,
                                       ZOOM_IR_1X);
    config_param::expectConfigAccepted(PAYLOAD_CAMERA_IR_PALETTE,
                                       PAYLOAD_CAMERA_IR_PALETTE_1);
}

TEST_F(CameraParamCatalogTest, ControlAndIcr_RepresentativeValues) {
    config_param::expectConfigAccepted(PAYLOAD_CAMERA_RC_MODE,
                                       PAYLOAD_CAMERA_RC_MODE_GREMSY);
    config_param::expectConfigAccepted(PAYLOAD_CAMERA_EO_ICR_MODE,
                                       PAYLOAD_CAMERA_EO_ICR_MODE_AUTO);
    config_param::expectConfigAccepted(
        PAYLOAD_CAMERA_EO_ICR_MODE_AUTO_THRESHOLD, 128);
}
