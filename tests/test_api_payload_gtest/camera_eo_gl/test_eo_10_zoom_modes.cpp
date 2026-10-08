/**
 * Sheet row 65: Chon kieu zoom Step / Continuous / Range - setCameraZoom() + setPayloadCameraParam()
 * Support: VIO x | ORUSL x | MB1 ? | ZIO ?
 * Example: examples/camera_set_all_zoom_modes.cpp
 */

#include "camera_eo_test_helpers.h"

using namespace eo;

// MB1 has no C_V_ZM_MODE (single C_V_ZOOM level), so the suite is skipped there.
class EO_ZoomModes : public EoCameraTest {
protected:
    void SetUp() override {
        EO_SKIP_UNLESS_SUPPORTED("xx??");

        EoCameraTest::SetUp();

        if (HasFatalFailure()) {
            return;
        }

        // Remember the current zoom mode so TearDown() puts it back.
        ASSERT_TRUE(restoreLater("C_V_ZM_MODE"))
            << "C_V_ZM_MODE is not readable on this payload";
    }

    // Select a digital zoom mode, then try the three zoom types in it.
    void runZoomTypesInMode(uint32_t mode, const char* modeName) {
        ASSERT_TRUE(setParam("C_V_ZM_MODE", mode))
            << "could not select zoom mode " << modeName;

        std::cout << "[  INFO  ] C_V_ZM_MODE = " << mode << " (" << modeName << ")\n";

        sleepMs(500);

        // Start from wide.
        g_payload->setCameraZoom(ZOOM_TYPE_RANGE, 0);

        sleepMs(1500);

        double zoomBefore = 0;

        ASSERT_TRUE(readZoomLevel(CAMERA_EO, zoomBefore))
            << "EO zoom level is not readable";

        std::cout << "[  INFO  ] " << modeName << ": zoom level before = " << zoomBefore << "\n";

        // Step zoom in, the level must increase.
        ASSERT_TRUE(
            sendAndWaitAck(
                MAV_CMD_SET_CAMERA_ZOOM,
                [] {
                    g_payload->setCameraZoom(ZOOM_TYPE_STEP, ZOOM_IN);
                },
                std::string(modeName) + " step in"
            )
        ) << modeName << ": setCameraZoom(ZOOM_TYPE_STEP, ZOOM_IN) was not accepted";

        std::cout << "[  INFO  ] " << modeName << ": setCameraZoom(STEP, ZOOM_IN): ACK accepted\n";

        double zoomAfterStep = 0;

        bool stepIncreased = waitZoomIncreased(CAMERA_EO, zoomBefore, 5000, &zoomAfterStep);

        EXPECT_TRUE(stepIncreased)
            << modeName << ": step zoom did not increase the level";

        std::cout << "[  INFO  ] " << modeName << ": zoom level after step = " << zoomAfterStep << "\n";

        // Continuous zoom in for 2 seconds then stop, the level must increase again.
        ASSERT_TRUE(
            sendAndWaitAck(
                MAV_CMD_SET_CAMERA_ZOOM,
                [] {
                    g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_IN);
                },
                std::string(modeName) + " continuous in"
            )
        ) << modeName << ": setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_IN) was not accepted";

        std::cout << "[  INFO  ] " << modeName << ": setCameraZoom(CONTINUOUS, ZOOM_IN): ACK accepted, zooming for 2 s\n";

        sleepMs(2000);

        ASSERT_TRUE(
            sendAndWaitAck(
                MAV_CMD_SET_CAMERA_ZOOM,
                [] {
                    g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_STOP);
                },
                std::string(modeName) + " stop"
            )
        ) << modeName << ": setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_STOP) was not accepted";

        std::cout << "[  INFO  ] " << modeName << ": setCameraZoom(CONTINUOUS, ZOOM_STOP): ACK accepted\n";

        double zoomAfterContinuous = 0;

        bool continuousIncreased = waitZoomIncreased(CAMERA_EO, zoomAfterStep, 5000, &zoomAfterContinuous);

        EXPECT_TRUE(continuousIncreased)
            << modeName << ": continuous zoom did not increase the level";

        std::cout << "[  INFO  ] " << modeName << ": zoom level after continuous = " << zoomAfterContinuous << "\n";

        // Range zoom 0%, the level must return to where it started.
        ASSERT_TRUE(
            sendAndWaitAck(
                MAV_CMD_SET_CAMERA_ZOOM,
                [] {
                    g_payload->setCameraZoom(ZOOM_TYPE_RANGE, 0);
                },
                std::string(modeName) + " range 0%"
            )
        ) << modeName << ": setCameraZoom(ZOOM_TYPE_RANGE, 0) was not accepted";

        std::cout << "[  INFO  ] " << modeName << ": setCameraZoom(RANGE, 0): ACK accepted\n";

        double zoomAfterRange = 0;

        bool rangeReturned = waitZoomNear(CAMERA_EO, zoomBefore, 0.3, 10000, &zoomAfterRange);

        EXPECT_TRUE(rangeReturned)
            << modeName << ": range 0% did not return to " << zoomBefore;

        std::cout << "[  INFO  ] " << modeName << ": zoom level after range 0% = " << zoomAfterRange << "\n";

        std::cout << "[  INFO  ] " << modeName << " zoom levels: "
                  << zoomBefore << " -> " << zoomAfterStep << " -> "
                  << zoomAfterContinuous << " -> " << zoomAfterRange << "\n";
    }
};

// Check the three zoom types in Combine mode (C_V_ZM_MODE = 0).
TEST_F(EO_ZoomModes, SetCameraZoomInCombineMode) {
    runZoomTypesInMode(0, "combine");
}

// Check the three zoom types in Super Resolution mode (C_V_ZM_MODE = 2).
TEST_F(EO_ZoomModes, SetCameraZoomInSuperResolutionMode) {
    runZoomTypesInMode(2, "super_resolution");
}
