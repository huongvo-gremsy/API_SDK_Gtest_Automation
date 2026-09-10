/**
 * @file test_camera_ir_set_zoom.cpp
 * @brief Tests IR step, continuous, stop, and range zoom commands.
 */

#include "camera_ir_test_helpers.h"

namespace cit = camera_ir_test;

class CameraIrZoomTest : public cit::CameraIrTest {
protected:
    void SetUp() override {
        CameraIrTest::SetUp();
        if (::testing::Test::IsSkipped()) return;
        ASSERT_TRUE(cit::getCameraSettingByID(PAYLOAD_CAMERA_IR_ZOOM_FACTOR,
                                          originalZoom_, 3000));
        haveZoom_ = true;
        ASSERT_TRUE(cit::setUint32Param(PAYLOAD_CAMERA_IR_ZOOM_FACTOR,
                                        ZOOM_IR_1X));
    }
    void TearDown() override {
        g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_STOP);
        if (haveZoom_) {
            EXPECT_TRUE(cit::setUint32Param(
                PAYLOAD_CAMERA_IR_ZOOM_FACTOR,
                static_cast<uint32_t>(originalZoom_)))
                << "Could not restore the original IR zoom factor.";
        }
        CameraIrTest::TearDown();
    }
    double originalZoom_ = 0;
    bool haveZoom_ = false;
};

TEST_F(CameraIrZoomTest, StepInFourThenOutTwo_FactorChanges) {
    double baseline = 0;
    ASSERT_TRUE(cit::getCameraSettingByID(PAYLOAD_CAMERA_IR_ZOOM_FACTOR, baseline));
    for (int i = 0; i < 4; ++i) {
        g_payload->setCameraZoom(ZOOM_TYPE_STEP, ZOOM_IN);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    double zoomedIn = 0;
    ASSERT_TRUE(cit::getCameraSettingByID(PAYLOAD_CAMERA_IR_ZOOM_FACTOR, zoomedIn));
    EXPECT_NE(zoomedIn, baseline);

    for (int i = 0; i < 2; ++i) {
        g_payload->setCameraZoom(ZOOM_TYPE_STEP, ZOOM_OUT);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    double zoomedOut = 0;
    ASSERT_TRUE(cit::getCameraSettingByID(PAYLOAD_CAMERA_IR_ZOOM_FACTOR, zoomedOut));
    EXPECT_NE(zoomedOut, zoomedIn);
}

TEST_F(CameraIrZoomTest, ContinuousInStopOutStop_RemainsResponsive) {
    g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_IN);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_STOP);
    ASSERT_TRUE(cit::irStreamIsResponsive());

    g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_OUT);
    std::this_thread::sleep_for(std::chrono::seconds(4));
    g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_STOP);
    EXPECT_TRUE(cit::irStreamIsResponsive());
}

TEST_F(CameraIrZoomTest, Range0_50_70_100_CommandsAcceptedOrStreamHealthy) {
    const float ranges[] = {50.0f, 70.0f, 100.0f, 0.0f};
    for (const float range : ranges) {
        const uint64_t seq = getCommandAckSeq(MAV_CMD_SET_CAMERA_ZOOM);
        g_payload->setCameraZoom(ZOOM_TYPE_RANGE, range);
        AckInfo ack;
        const bool accepted =
            waitForCommandAck(MAV_CMD_SET_CAMERA_ZOOM, seq, ack, 2000) &&
            (ack.result == MAV_RESULT_ACCEPTED ||
             ack.result == MAV_RESULT_IN_PROGRESS);
        EXPECT_TRUE(accepted || cit::irStreamIsResponsive())
            << "IR range zoom " << range
            << "% had no accepted ACK and stream became unavailable.";
    }
}
