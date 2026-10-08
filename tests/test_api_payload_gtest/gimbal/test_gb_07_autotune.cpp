/**
 * Sheet row 7: Auto tune gimbal - sendPayloadGimbalAutoTune()
 * Support: VIO x | ORUSL x | MB1 ? | ZIO ?
 * Example: examples/gimbal_calib_auto_tune.cpp
 */

#include "gb_test_helpers.h"

using namespace gb;

// Auto tune moves the gimbal and reboots it when done (the example waits 20 s),
// so on top of the sheet columns it also needs PAYLOAD_TEST_GIMBAL_CALIB=1.
// Requires gimbal software >= 790.6. The command is MAV_CMD_USER_3.
class GB_AutoTune : public GimbalTest {
protected:
    void SetUp() override {
        GB_SKIP_UNLESS_SUPPORTED("xx??");

        if (!runGimbalCalib()) {
            GTEST_SKIP() << "auto tune moves and reboots the gimbal; set PAYLOAD_TEST_GIMBAL_CALIB=1 to run it";
        }

        GimbalTest::SetUp();
    }

    void TearDown() override {
        // Never leave auto tune running.
        g_payload->sendPayloadGimbalAutoTune(false);

        GimbalTest::TearDown();
    }
};
// Auto tune gimbal	sendPayloadGimbalAutoTune()	x	x	?	?
// Check that sendPayloadGimbalAutoTune(true) then (false) are both accepted.
TEST_F(GB_AutoTune, SendPayloadGimbalAutoTune) {
    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_USER_3,
            [] {
                g_payload->sendPayloadGimbalAutoTune(true);
            },
            "auto tune on",
            10000
        )
    ) << "sendPayloadGimbalAutoTune(true) was not accepted";

    std::cout << "[  INFO  ] sendPayloadGimbalAutoTune(true): ACK accepted\n";

    sleepMs(1000);

    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_USER_3,
            [] {
                g_payload->sendPayloadGimbalAutoTune(false);
            },
            "auto tune off",
            10000
        )
    ) << "sendPayloadGimbalAutoTune(false) was not accepted";

    std::cout << "[  INFO  ] sendPayloadGimbalAutoTune(false): ACK accepted\n";
}
