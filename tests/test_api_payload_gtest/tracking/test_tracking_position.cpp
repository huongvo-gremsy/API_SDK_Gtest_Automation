/**
 * @file test_tracking_position.cpp
 * @brief Tests setPayloadObjectTrackingPosition().
 */

#include "tracking_test_helpers.h"

#include <array>
#include <chrono>
#include <iostream>
#include <thread>

namespace tt = tracking_test;

class TrackingPositionTest : public tt::TrackingTestBase {
protected:
    void SetUp() override {
        TrackingTestBase::SetUp();
        if (::testing::Test::IsSkipped()) return;
        g_payload->setPayloadObjectTrackingMode(tt::kTrackActive);
        ASSERT_TRUE(tt::waitForKnownStatus());
    }
};

TEST_F(TrackingPositionTest, CenteredBox_AppearsInTelemetry) {
    EXPECT_TRUE(tt::waitForRoiNear(960, 540, 128, 128))
        << "Centered tracking ROI was not reflected in telemetry.";
}

TEST_F(TrackingPositionTest, TopLeftValidBox_AppearsInTelemetry) {
    EXPECT_TRUE(tt::waitForRoiNear(64, 64, 128, 128))
        << "Top-left valid tracking ROI was not reflected in telemetry.";
}

TEST_F(TrackingPositionTest, BottomRightValidBox_RemainsInsideFrame) {
    ASSERT_TRUE(tt::waitForRoiNear(1856, 1016, 128, 128));
    tt::Snapshot current;
    ASSERT_TRUE(tt::getSnapshot(current));
    EXPECT_GE(current.x, 0);
    EXPECT_LE(current.x, tt::kFrameWidth);
    EXPECT_GE(current.y, 0);
    EXPECT_LE(current.y, tt::kFrameHeight);
    EXPECT_GT(current.width, 0);
    EXPECT_GT(current.height, 0);
}

TEST_F(TrackingPositionTest, FarApartPoints_AreAppliedInSequence) {
    struct Point {
        double x;
        double y;
        const char* name;
    };

    // Keep the 128x128 ROI fully inside the documented 1920x1080 frame while
    // making every transition large and easy to observe on the live screen.
    const std::array<Point, 5> points = {{
        {160, 160, "top-left"},
        {1760, 920, "bottom-right"},
        {1760, 160, "top-right"},
        {160, 920, "bottom-left"},
        {960, 540, "center"},
    }};

    for (const auto& point : points) {
        std::cout << "[WATCH] Moving tracking point to " << point.name
                  << " (x=" << point.x << ", y=" << point.y << ")"
                  << std::endl;
        ASSERT_TRUE(tt::waitForRoiNear(point.x, point.y, 128, 128, 96, 7000))
            << "Tracking telemetry did not reach the " << point.name
            << " point.";
        std::cout << "[WATCH] Hold at " << point.name
                  << " for 2 seconds" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

TEST_F(TrackingPositionTest, OppositeFarPoints_TravelAcrossFrame) {
    ASSERT_TRUE(tt::waitForRoiNear(192, 540, 128, 128, 96, 7000))
        << "Could not acquire the far-left point.";
    std::this_thread::sleep_for(std::chrono::seconds(2));

    ASSERT_TRUE(tt::waitForRoiNear(1728, 540, 128, 128, 96, 7000))
        << "Could not acquire the far-right point.";
    std::this_thread::sleep_for(std::chrono::seconds(2));

    tt::Snapshot current;
    ASSERT_TRUE(tt::getSnapshot(current));
    EXPECT_NEAR(current.x, 1728, 96);
    EXPECT_NEAR(current.y, 540, 96);
}

TEST_F(TrackingPositionTest, ZeroSize_IsRejectedOrIgnored) {
    ASSERT_TRUE(tt::waitForRoiNear(960, 540, 128, 128));
    AckInfo ack;
    const bool gotAck = tt::sendUser4([] {
        g_payload->setPayloadObjectTrackingPosition(960, 540, 0, 0);
    }, ack);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    tt::Snapshot current;
    ASSERT_TRUE(tt::getSnapshot(current));
    const bool rejected = gotAck && !tt::acceptedOrInProgress(ack);
    const bool retainedValidBox = current.width > 0 && current.height > 0;
    EXPECT_TRUE(rejected || retainedValidBox)
        << "Payload accepted and reported a zero-sized tracking box.";
}

TEST_F(TrackingPositionTest, OutOfFrame_IsRejectedOrClamped) {
    AckInfo ack;
    const bool gotAck = tt::sendUser4([] {
        g_payload->setPayloadObjectTrackingPosition(2500, 1500, 128, 128);
    }, ack);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    tt::Snapshot current;
    ASSERT_TRUE(tt::getSnapshot(current));
    const bool rejected = gotAck && !tt::acceptedOrInProgress(ack);
    const bool inside = current.x >= 0 && current.x <= tt::kFrameWidth &&
                        current.y >= 0 && current.y <= tt::kFrameHeight;
    EXPECT_TRUE(rejected || inside)
        << "Out-of-frame ROI was neither rejected nor clamped.";
}

TEST_F(TrackingPositionTest, StopAfterTrigger_ReturnsIdle) {
    ASSERT_TRUE(tt::waitForRoiNear(960, 540, 128, 128));
    g_payload->setPayloadObjectTrackingMode(tt::kTrackStop);
    EXPECT_TRUE(tt::waitForStatus(tt::kTrackIdle));
}
