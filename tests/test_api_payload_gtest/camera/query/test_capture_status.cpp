/**
 * @file test_capture_status.cpp
 * @brief Read-only getPayloadCaptureStatus() tests.
 */

#include "camera_query_test_helpers.h"

#include <cmath>

namespace {

struct CaptureInfo {
    double imageStatus = -1;
    double videoStatus = -1;
    double imageCount = -1;
    double recordingTimeMs = -1;
};

bool readStatus(CaptureInfo& info) {
    return getCaptureStatus(info.imageStatus, info.videoStatus,
                            info.imageCount, info.recordingTimeMs, 3000);
}

bool isBinaryStatus(double status) {
    return status == 0.0 || status == 1.0;
}

}  // namespace

class CaptureStatusTest : public PayloadTest {};

TEST_F(CaptureStatusTest, EventArrives) {
    CaptureInfo info;
    EXPECT_TRUE(readStatus(info))
        << "No new PAYLOAD_CAM_CAPTURE_STATUS callback within 3000 ms.";
}

TEST_F(CaptureStatusTest, ImageAndVideoStatus_AreKnown) {
    CaptureInfo info;
    ASSERT_TRUE(readStatus(info));
    EXPECT_TRUE(isBinaryStatus(info.imageStatus))
        << "Unexpected image_status=" << info.imageStatus << ".";
    EXPECT_TRUE(isBinaryStatus(info.videoStatus))
        << "Unexpected video_status=" << info.videoStatus << ".";
}

TEST_F(CaptureStatusTest, ImageCount_IsFiniteNonnegativeInteger) {
    CaptureInfo info;
    ASSERT_TRUE(readStatus(info));
    EXPECT_TRUE(std::isfinite(info.imageCount));
    EXPECT_GE(info.imageCount, 0.0);
    EXPECT_EQ(info.imageCount, std::floor(info.imageCount));
}

TEST_F(CaptureStatusTest, RecordingTime_IsFiniteAndNonnegative) {
    CaptureInfo info;
    ASSERT_TRUE(readStatus(info));
    EXPECT_TRUE(std::isfinite(info.recordingTimeMs));
    EXPECT_GE(info.recordingTimeMs, 0.0);
}

TEST_F(CaptureStatusTest, RepeatedRead_ImageCountIsMonotonic) {
    CaptureInfo first;
    CaptureInfo second;
    ASSERT_TRUE(readStatus(first));
    ASSERT_TRUE(readStatus(second));
    EXPECT_GE(second.imageCount, first.imageCount)
        << "A cumulative image_count must not decrease.";
}
