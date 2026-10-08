#include "api_camera_eo_test_helper.h"

namespace cet = camera_eo_test;

class CameraEoFocusTest : public cet::CameraEoTest {};

TEST_F(CameraEoFocusTest, SetCameraFocus_StopIsAcceptedOrHarmless) {
#if defined(MB1)
    GTEST_SKIP() << "MB1 focus support is not confirmed in the feature table.";
#else
    ASSERT_TRUE(cet::setCameraParam(PAYLOAD_CAMERA_VIEW_SRC,
                                    PAYLOAD_CAMERA_VIEW_EO));

    EXPECT_TRUE(cet::sendAndAcceptIfAcked(
        MAV_CMD_SET_CAMERA_FOCUS,
        [] {
            g_payload->setCameraFocus(FOCUS_TYPE_CONTINUOUS, FOCUS_STOP);
        }))
        << "setCameraFocus(CONTINUOUS, STOP) was rejected.";
#endif
}

TEST_F(CameraEoFocusTest, SetCameraFocus_AutoAcceptedOrHarmless) {
#if defined(MB1)
    GTEST_SKIP() << "MB1 focus support is not confirmed in the feature table.";
#else
    ASSERT_TRUE(cet::setCameraParam(PAYLOAD_CAMERA_VIEW_SRC,
                                    PAYLOAD_CAMERA_VIEW_EO));

    EXPECT_TRUE(cet::sendAndAcceptIfAcked(
        MAV_CMD_SET_CAMERA_FOCUS,
        [] {
            g_payload->setCameraFocus(FOCUS_TYPE_AUTO);
        }))
        << "setCameraFocus(AUTO) was rejected.";
#endif
}
