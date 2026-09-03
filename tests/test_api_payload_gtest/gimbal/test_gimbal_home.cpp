/** @file test_gimbal_home.cpp */
#include "gimbal_test_helpers.h"

namespace gt = gimbal_test;
class GimbalHomeTest : public PayloadTest {};

TEST_F(GimbalHomeTest, DISABLED_SearchHome_AcceptedAndTelemetryContinues) {
    const uint64_t attitudeSeq = g_cb.gimbalAttitudeSeq.load();
    ASSERT_TRUE(gt::commandAccepted(MAV_CMD_DO_SET_HOME, [] {
        g_payload->sendPayloadGimbalSearchHome();
    }, 10000));
    EXPECT_TRUE(waitForSeq(g_cb.gimbalAttitudeSeq, attitudeSeq, 15000))
        << "No attitude telemetry during/after home search.";
}
