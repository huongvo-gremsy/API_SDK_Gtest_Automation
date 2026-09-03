/** @file test_gimbal_autotune.cpp */
#include "gimbal_test_helpers.h"

namespace gt = gimbal_test;

class AutoTuneDisableGuard {
public:
    ~AutoTuneDisableGuard() { g_payload->sendPayloadGimbalAutoTune(false); }
};

class GimbalAutoTuneTest : public PayloadTest {};

TEST_F(GimbalAutoTuneTest, DISABLED_EnableThenDisable_Accepted) {
    AutoTuneDisableGuard guard;
    ASSERT_TRUE(gt::commandAccepted(MAV_CMD_USER_3, [] {
        g_payload->sendPayloadGimbalAutoTune(true);
    }, 10000));
    EXPECT_TRUE(gt::commandAccepted(MAV_CMD_USER_3, [] {
        g_payload->sendPayloadGimbalAutoTune(false);
    }, 10000));
}

TEST_F(GimbalAutoTuneTest, DISABLED_Disable_IsIdempotent) {
    EXPECT_TRUE(gt::commandAccepted(MAV_CMD_USER_3, [] {
        g_payload->sendPayloadGimbalAutoTune(false);
    }, 10000));
}
