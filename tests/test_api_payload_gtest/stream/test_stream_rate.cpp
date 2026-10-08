/**
 * @file test_stream_rate.cpp
 * @brief Tests sendPayloadRequestStreamRate().
 */

#include "stream_test_helpers.h"

namespace st = stream_test;

class StreamRateTest : public PayloadTest {};

TEST_F(StreamRateTest, EOZoomIndex_OneSecondInterval_IsAccepted) {
    ASSERT_TRUE(st::sendRateAndWaitForAcceptedAck([] {
        g_payload->sendPayloadRequestStreamRate(PARAM_EO_ZOOM_LEVEL, 1000);
    })) << "No accepted SET_MESSAGE_INTERVAL ACK for EO zoom index.";

    // Return the index to its firmware-defined default interval.
    EXPECT_TRUE(st::sendRateAndWaitForAcceptedAck([] {
        g_payload->sendPayloadRequestStreamRate(PARAM_EO_ZOOM_LEVEL, 0);
    })) << "Could not restore the default EO zoom message interval.";
}

TEST_F(StreamRateTest, IRZoomIndex_DefaultInterval_IsAccepted) {
    EXPECT_TRUE(st::sendRateAndWaitForAcceptedAck([] {
        g_payload->sendPayloadRequestStreamRate(PARAM_IR_ZOOM_LEVEL, 0);
    })) << "No accepted SET_MESSAGE_INTERVAL ACK for IR zoom index.";
}

TEST_F(StreamRateTest, RepeatedRateRequest_RemainsConnected) {
    ASSERT_TRUE(st::sendRateAndWaitForAcceptedAck([] {
        g_payload->sendPayloadRequestStreamRate(PARAM_EO_ZOOM_LEVEL, 500);
    }));
    ASSERT_TRUE(st::sendRateAndWaitForAcceptedAck([] {
        g_payload->sendPayloadRequestStreamRate(PARAM_EO_ZOOM_LEVEL, 1000);
    }));
    ASSERT_TRUE(st::sendRateAndWaitForAcceptedAck([] {
        g_payload->sendPayloadRequestStreamRate(PARAM_EO_ZOOM_LEVEL, 0);
    }));

    st::Snapshot info;
    EXPECT_TRUE(st::getSnapshot(st::kEoStreamId, info))
        << "Payload stopped answering stream queries after rate changes.";
}
