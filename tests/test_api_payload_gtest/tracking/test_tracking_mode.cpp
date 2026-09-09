/**
 * @file test_tracking_mode.cpp
 * @brief Tests setPayloadObjectTrackingMode().
 */

#include "tracking_test_helpers.h"

namespace tt = tracking_test;

class TrackingModeTest : public tt::TrackingTestBase {};

TEST_F(TrackingModeTest, Stop_ReportsIdle) {
    AckInfo ack;
    const bool gotAck = tt::sendUser4([] {
        g_payload->setPayloadObjectTrackingMode(tt::kTrackStop);
    }, ack);
    EXPECT_TRUE(tt::waitForStatus(tt::kTrackIdle))
        << "Tracking did not return to IDLE after STOP.";
    if (gotAck) EXPECT_TRUE(tt::acceptedOrInProgress(ack));
}

TEST_F(TrackingModeTest, Active_ProducesKnownTrackingStatus) {
    AckInfo ack;
    const bool gotAck = tt::sendUser4([] {
        g_payload->setPayloadObjectTrackingMode(tt::kTrackActive);
    }, ack);
    const bool gotStatus = tt::waitForKnownStatus();
    EXPECT_TRUE(gotAck || gotStatus)
        << "ACTIVE produced neither an ACK nor tracking telemetry.";
    if (gotAck) EXPECT_TRUE(tt::acceptedOrInProgress(ack));
}

TEST_F(TrackingModeTest, EagleEyes_CommandAcceptedOrTelemetryResponsive) {
    AckInfo ack;
    const bool gotAck = tt::sendUser4([] {
        g_payload->setPayloadObjectTrackingMode(tt::kTrackEagleEyes);
    }, ack);
    const bool gotStatus = tt::waitForKnownStatus();
    EXPECT_TRUE(gotAck || gotStatus)
        << "EagleEyes produced neither an ACK nor tracking telemetry.";
    if (gotAck) EXPECT_TRUE(tt::acceptedOrInProgress(ack));
}

TEST_F(TrackingModeTest, ActiveThenStop_ReturnsIdle) {
    g_payload->setPayloadObjectTrackingMode(tt::kTrackActive);
    ASSERT_TRUE(tt::waitForKnownStatus());
    g_payload->setPayloadObjectTrackingMode(tt::kTrackStop);
    EXPECT_TRUE(tt::waitForStatus(tt::kTrackIdle));
}
