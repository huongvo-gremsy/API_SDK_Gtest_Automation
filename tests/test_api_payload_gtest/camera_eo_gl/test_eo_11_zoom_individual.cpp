/**
 * Sheet row 66: Zoom EO va IR rieng theo nguon hien thi - setPayloadCameraParam() + setParamRate()
 * Support: VIO x | ORUSL x | MB1 ? | ZIO -
 * Example: examples/camera_do_setzoom_individual.cpp
 */

#include "camera_eo_test_helpers.h"

using namespace eo;

// The payload streams both zoom levels after setParamRate(), like the example.
class EO_ZoomIndividual : public EoCameraTest {
protected:
    void SetUp() override {
        EO_SKIP_UNLESS_SUPPORTED("xx?-");

        EoCameraTest::SetUp();

        if (HasFatalFailure()) {
            return;
        }

        // Stream both zoom levels once a second.
        g_payload->setParamRate(PARAM_EO_ZOOM_LEVEL, 1000);
        g_payload->setParamRate(PARAM_IR_ZOOM_LEVEL, 1000);

        std::cout << "[  INFO  ] SetUp: setParamRate(EO_ZOOM_LEVEL, 1000), setParamRate(IR_ZOOM_LEVEL, 1000)\n";
    }

    void TearDown() override {
        // Stop the streaming.
        g_payload->setParamRate(PARAM_EO_ZOOM_LEVEL, 0);
        g_payload->setParamRate(PARAM_IR_ZOOM_LEVEL, 0);

        EoCameraTest::TearDown();
    }

    // Wait until the zoom level of `camera` is near `target`.
    void expectZoomLevel(camera_type_t camera, double target, double tolerance, const char* what) {
        double last = -1;

        bool zoomReached = waitZoomNear(camera, target, tolerance, 10000, &last);

        EXPECT_TRUE(zoomReached)
            << what << ": zoom level did not reach " << target << " (last " << last << ")";

        std::cout << "[  INFO  ] " << what << ": expected " << target << ", got " << last << "\n";
    }
};

// Check that C_V_ZM_SR_LV changes the EO zoom in Super Resolution mode.
TEST_F(EO_ZoomIndividual, SetPayloadCameraParam_SuperResolutionLevel) {
    // Remember the current values so TearDown() puts them back.
    ASSERT_TRUE(restoreLater("C_V_ZM_MODE"));
    ASSERT_TRUE(restoreLater("C_V_ZM_SR_LV"));

    // Super Resolution mode, view EO/IR.
    ASSERT_TRUE(setParam("C_V_ZM_MODE", 2));
    ASSERT_TRUE(setParam(PAYLOAD_CAMERA_VIEW_SRC, 0));

    std::cout << "[  INFO  ] C_V_ZM_MODE = 2 (super resolution), view source = 0 (EO/IR)\n";

    // Index 0 is 1x on every product table.
    ASSERT_TRUE(setParam("C_V_ZM_SR_LV", 0));

    std::cout << "[  INFO  ] C_V_ZM_SR_LV = 0\n";

    expectZoomLevel(CAMERA_EO, 1.0, 0.3, "SR level index 0");

    // Index 2 is 4x on every product table.
    ASSERT_TRUE(setParam("C_V_ZM_SR_LV", 2));

    std::cout << "[  INFO  ] C_V_ZM_SR_LV = 2\n";

    expectZoomLevel(CAMERA_EO, 4.0, 0.6, "SR level index 2");
}

// Check that C_V_ZM_CB_LV changes the EO zoom in Combine mode.
TEST_F(EO_ZoomIndividual, SetPayloadCameraParam_CombineLevel) {
    // Remember the current values so TearDown() puts them back.
    ASSERT_TRUE(restoreLater("C_V_ZM_MODE"));
    ASSERT_TRUE(restoreLater("C_V_ZM_CB_LV"));

    // Combine mode.
    ASSERT_TRUE(setParam("C_V_ZM_MODE", 0));

    std::cout << "[  INFO  ] C_V_ZM_MODE = 0 (combine)\n";

    // Index 0 is 1x.
    ASSERT_TRUE(setParam("C_V_ZM_CB_LV", 0));

    std::cout << "[  INFO  ] C_V_ZM_CB_LV = 0\n";

    expectZoomLevel(CAMERA_EO, 1.0, 0.3, "combine level index 0");

    // Index 1 is the next table entry, above 1x.
    ASSERT_TRUE(setParam("C_V_ZM_CB_LV", 1));

    std::cout << "[  INFO  ] C_V_ZM_CB_LV = 1\n";

    double last = -1;

    bool zoomIncreased = waitZoomIncreased(CAMERA_EO, 1.0, 10000, &last);

    EXPECT_TRUE(zoomIncreased)
        << "combine level index 1 did not increase the EO zoom (last " << last << ")";

    std::cout << "[  INFO  ] combine level index 1: EO zoom = " << last << "\n";
}

// Check that C_T_ZOOM changes the IR zoom.
TEST_F(EO_ZoomIndividual, SetPayloadCameraParam_IrZoomLevel) {
    // Remember the current value so TearDown() puts it back.
    ASSERT_TRUE(restoreLater("C_T_ZOOM"));

    // View IR/EO.
    ASSERT_TRUE(setParam(PAYLOAD_CAMERA_VIEW_SRC, 3));

    std::cout << "[  INFO  ] view source = 3 (IR/EO)\n";

    // 0 is 1x.
    ASSERT_TRUE(setParam("C_T_ZOOM", 0));

    std::cout << "[  INFO  ] C_T_ZOOM = 0\n";

    expectZoomLevel(CAMERA_IR, 1.0, 0.3, "IR zoom 1x");

    // 3 is 4x.
    ASSERT_TRUE(setParam("C_T_ZOOM", 3));

    std::cout << "[  INFO  ] C_T_ZOOM = 3\n";

    expectZoomLevel(CAMERA_IR, 4.0, 0.6, "IR zoom 4x");
}
