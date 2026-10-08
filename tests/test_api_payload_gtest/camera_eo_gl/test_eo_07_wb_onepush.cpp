/**
 * Sheet row 7: Can bang trang One Push - setPayloadCameraWBOnePushTrigg()
 * Support: VIO x | ORUSL x | MB1 ? | ZIO ?
 * Example: examples/camera_eo_trigger_wb_onepush.cpp
 */

#include "camera_eo_test_helpers.h"

using namespace eo;

// The trigger is a MAV_CMD_USER_4 command; the only feedback is its ACK.
class EO_WbOnePush : public EoCameraTest {
protected:
    void SetUp() override {
        EO_SKIP_UNLESS_SUPPORTED("xx??");

        EoCameraTest::SetUp();

        if (HasFatalFailure()) {
            return;
        }

        // Select WB one-push mode like the example does. TearDown() restores it.
#if defined(VIO) || defined(ORUSL)
        if (restoreLater(PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE)) {
            EXPECT_TRUE(setParam(PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE, PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE_ONE_PUSH))
                << "could not select WB one-push mode";

            std::cout << "[  INFO  ] SetUp: WB mode = ONE_PUSH\n";
        }
#endif
    }
};

// Check that setPayloadCameraWBOnePushTrigg() is accepted.
TEST_F(EO_WbOnePush, SetPayloadCameraWBOnePushTrigg) {
    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_USER_4,
            [] {
                g_payload->setPayloadCameraWBOnePushTrigg();
            },
            "WB one push trigger"
        )
    ) << "setPayloadCameraWBOnePushTrigg() was not accepted";

    std::cout << "[  INFO  ] setPayloadCameraWBOnePushTrigg(): ACK accepted\n";
}

// Check that setPayloadCameraWBOnePushTrigg() can be repeated.
TEST_F(EO_WbOnePush, SetPayloadCameraWBOnePushTriggRepeated) {
    for (int i = 1; i <= 3; i++) {
        ASSERT_TRUE(
            sendAndWaitAck(
                MAV_CMD_USER_4,
                [] {
                    g_payload->setPayloadCameraWBOnePushTrigg();
                },
                "WB one push trigger"
            )
        ) << "setPayloadCameraWBOnePushTrigg() #" << i << " was not accepted";

        std::cout << "[  INFO  ] setPayloadCameraWBOnePushTrigg() #" << i << ": ACK accepted\n";

        sleepMs(1000);
    }
}
