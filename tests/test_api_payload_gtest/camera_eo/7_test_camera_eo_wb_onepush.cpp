#include "api_camera_eo_test_helper.h"

namespace cet = camera_eo_test;

class CameraEoWhiteBalanceOnePushTest : public cet::CameraEoTest {};

TEST_F(CameraEoWhiteBalanceOnePushTest,
       SetPayloadCameraWBOnePushTrigg_AcceptedOrHarmless) {
#if defined(MB1) || defined(ZIO)
    GTEST_SKIP() << "WB one-push support is not confirmed for this product.";
#else
    ASSERT_TRUE(cet::setCameraParam(PAYLOAD_CAMERA_VIEW_SRC,
                                    PAYLOAD_CAMERA_VIEW_EO));

    EXPECT_TRUE(cet::sendAndAcceptIfAcked(
        MAV_CMD_USER_4,
        [] {
            g_payload->setPayloadCameraWBOnePushTrigg();
        }))
        << "setPayloadCameraWBOnePushTrigg() was rejected.";
#endif
}
