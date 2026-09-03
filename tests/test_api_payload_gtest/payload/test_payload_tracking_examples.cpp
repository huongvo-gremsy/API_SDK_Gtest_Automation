/** @file test_payload_tracking_examples.cpp */
#include "../tracking/tracking_test_helpers.h"

#include <chrono>
#include <thread>

namespace tt = tracking_test;

class PayloadObjectDetectionExampleTest : public PayloadTest {
protected:
    void SetUp() override {
        if (!getCameraSettingByID(PAYLOAD_CAMERA_TRACKING_MODE,
                                  original_, 3000)) {
            GTEST_SKIP() << "TRACK_MODE is not exposed by this payload.";
        }
        haveOriginal_ = true;
        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIEW_SRC,
                                          originalView_, 3000));
        haveView_ = true;
        ASSERT_TRUE(tt::setCameraTrackingAlgorithm(
            static_cast<uint32_t>(original_)));
        char viewId[] = PAYLOAD_CAMERA_VIEW_SRC;
        ASSERT_TRUE(setAndVerifyCameraParam(
            viewId, PAYLOAD_CAMERA_VIEW_EO, PARAM_TYPE_UINT32,
            PAYLOAD_CAMERA_VIEW_EO));
    }
    void TearDown() override {
        if (haveOriginal_) {
            EXPECT_TRUE(tt::setCameraTrackingAlgorithm(
                static_cast<uint32_t>(original_)));
        }
        if (haveView_) {
            char viewId[] = PAYLOAD_CAMERA_VIEW_SRC;
            EXPECT_TRUE(setAndVerifyCameraParam(
                viewId, static_cast<uint32_t>(originalView_),
                PARAM_TYPE_UINT32, originalView_));
        }
    }
    double original_ = 0;
    bool haveOriginal_ = false;
    double originalView_ = 0;
    bool haveView_ = false;
};

TEST_F(PayloadObjectDetectionExampleTest, EnableDetectionThenTracking_ReadBack) {
    ASSERT_TRUE(tt::setCameraTrackingAlgorithm(
        PAYLOAD_CAMERA_TRACKING_OBJ_DETECTION));
    double actual = -1;
    ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_TRACKING_MODE,
                                      actual, 3000));
    EXPECT_EQ(actual, PAYLOAD_CAMERA_TRACKING_OBJ_DETECTION);

    ASSERT_TRUE(tt::setCameraTrackingAlgorithm(
        PAYLOAD_CAMERA_TRACKING_OBJ_TRACKING));
    ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_TRACKING_MODE,
                                      actual, 3000));
    EXPECT_EQ(actual, PAYLOAD_CAMERA_TRACKING_OBJ_TRACKING);
}

class PayloadObjectTrackingExampleTest : public tt::TrackingTestBase {
protected:
    void SetUp() override {
        TrackingTestBase::SetUp();
        if (::testing::Test::IsSkipped() || ::testing::Test::HasFatalFailure()) return;
        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIEW_SRC,
                                          originalView_, 3000));
        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIDEO_OSD_MODE,
                                          originalOsd_, 3000));
        configured_ = true;
        char viewId[] = PAYLOAD_CAMERA_VIEW_SRC;
        ASSERT_TRUE(setAndVerifyCameraParam(
            viewId, PAYLOAD_CAMERA_VIEW_EO, PARAM_TYPE_UINT32,
            PAYLOAD_CAMERA_VIEW_EO));
        char osdId[] = PAYLOAD_CAMERA_VIDEO_OSD_MODE;
        ASSERT_TRUE(setAndVerifyCameraParam(
            osdId, PAYLOAD_CAMERA_VIDEO_OSD_MODE_STATUS, PARAM_TYPE_UINT32,
            PAYLOAD_CAMERA_VIDEO_OSD_MODE_STATUS));
    }
    void TearDown() override {
        if (configured_) {
            char osdId[] = PAYLOAD_CAMERA_VIDEO_OSD_MODE;
            EXPECT_TRUE(setAndVerifyCameraParam(
                osdId, static_cast<uint32_t>(originalOsd_),
                PARAM_TYPE_UINT32, originalOsd_));
            char viewId[] = PAYLOAD_CAMERA_VIEW_SRC;
            EXPECT_TRUE(setAndVerifyCameraParam(
                viewId, static_cast<uint32_t>(originalView_),
                PARAM_TYPE_UINT32, originalView_));
        }
        TrackingTestBase::TearDown();
    }
    double originalView_ = 0;
    double originalOsd_ = 0;
    bool configured_ = false;
};

TEST_F(PayloadObjectTrackingExampleTest, ActivePositionThenStop) {
    g_payload->setPayloadObjectTrackingMode(tt::kTrackActive);
    ASSERT_TRUE(tt::waitForKnownStatus());
    ASSERT_TRUE(tt::waitForRoiNear(960, 540, 128, 128, 96, 7000));
    ASSERT_TRUE(tt::waitForRoiNear(1760, 920, 128, 128, 96, 7000));
    g_payload->setPayloadObjectTrackingMode(tt::kTrackStop);
    EXPECT_TRUE(tt::waitForStatus(tt::kTrackIdle));
}
