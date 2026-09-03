/**
 * @file test_camera_eo_zoom_modes.cpp
 * @brief EO zoom-mode tests based on camera_set_all_zoom_modes.cpp.
 */

#include "camera_eo_test_helpers.h"

#include <cstdint>
#include <iostream>

namespace cet = camera_eo_test;

#if defined(VIO) || defined(ZIO) || defined(ORUSL)

namespace {

#if defined(ORUSL)
constexpr uint32_t kLastCombineFactor = ZOOM_COMBINE_300X;
constexpr uint32_t kLastSuperResolutionFactor =
    ZOOM_SUPER_RESOLUTION_25X;
#else
constexpr uint32_t kLastCombineFactor = ZOOM_COMBINE_240X;
constexpr uint32_t kLastSuperResolutionFactor =
    ZOOM_SUPER_RESOLUTION_30X;
#endif

bool setZoomMode(uint32_t mode) {
    return cet::setUint32Param(PAYLOAD_CAMERA_VIDEO_ZOOM_MODE, mode);
}

bool setCombineFactor(uint32_t factor) {
    return cet::setUint32Param(
        PAYLOAD_CAMERA_VIDEO_ZOOM_COMBINE_FACTOR, factor);
}

bool setSuperResolutionFactor(uint32_t factor) {
    return cet::setUint32Param(
        PAYLOAD_CAMERA_VIDEO_ZOOM_SUPER_RESOLUTION_FACTOR, factor);
}

}  // namespace

class CameraEoZoomModesTest : public cet::CameraEoTest {
protected:
    void SetUp() override {
        cet::CameraEoTest::SetUp();
        if (::testing::Test::IsSkipped() ||
            ::testing::Test::HasFatalFailure()) {
            return;
        }

        ASSERT_TRUE(getCameraSettingByID(
            PAYLOAD_CAMERA_VIDEO_ZOOM_MODE, originalMode_, 3000))
            << "Could not read the original EO zoom mode.";
        haveMode_ = true;

        ASSERT_TRUE(getCameraSettingByID(
            PAYLOAD_CAMERA_VIDEO_ZOOM_COMBINE_FACTOR,
            originalCombineFactor_, 3000))
            << "Could not read the original Combine zoom factor.";
        haveCombineFactor_ = true;

        ASSERT_TRUE(getCameraSettingByID(
            PAYLOAD_CAMERA_VIDEO_ZOOM_SUPER_RESOLUTION_FACTOR,
            originalSuperResolutionFactor_, 3000))
            << "Could not read the original Super Resolution factor.";
        haveSuperResolutionFactor_ = true;
    }

    void TearDown() override {
        // Restore factor values before restoring the active mode. This keeps
        // the payload from being left at the largest factor if a test fails.
        if (haveCombineFactor_) {
            EXPECT_TRUE(setZoomMode(
                PAYLOAD_CAMERA_VIDEO_ZOOM_MODE_COMBINE))
                << "Could not select Combine mode during cleanup.";
            EXPECT_TRUE(setCombineFactor(
                static_cast<uint32_t>(originalCombineFactor_)))
                << "Could not restore the original Combine zoom factor.";
        }
        if (haveSuperResolutionFactor_) {
            EXPECT_TRUE(setZoomMode(
                PAYLOAD_CAMERA_VIDEO_ZOOM_MODE_SUPER_RESOLUTION))
                << "Could not select Super Resolution mode during cleanup.";
            EXPECT_TRUE(setSuperResolutionFactor(
                static_cast<uint32_t>(originalSuperResolutionFactor_)))
                << "Could not restore the original Super Resolution factor.";
        }
        if (haveMode_) {
            EXPECT_TRUE(setZoomMode(static_cast<uint32_t>(originalMode_)))
                << "Could not restore the original EO zoom mode.";
        }

        cet::CameraEoTest::TearDown();
    }

    double originalMode_ = 0;
    double originalCombineFactor_ = 0;
    double originalSuperResolutionFactor_ = 0;
    bool haveMode_ = false;
    bool haveCombineFactor_ = false;
    bool haveSuperResolutionFactor_ = false;
};

TEST_F(CameraEoZoomModesTest, CombineMode_SetAndReadBack) {
    // Exercise the SDK API directly, as camera_set_all_zoom_modes.cpp does.
    g_payload->setPayloadCameraParam(
        const_cast<char*>(PAYLOAD_CAMERA_VIDEO_ZOOM_MODE),
        PAYLOAD_CAMERA_VIDEO_ZOOM_MODE_COMBINE,
        PARAM_TYPE_UINT32);

    double actualMode = -1;
    ASSERT_TRUE(getCameraSettingByID(
        PAYLOAD_CAMERA_VIDEO_ZOOM_MODE, actualMode, 3000))
        << "No zoom-mode readback after selecting Combine mode.";
    EXPECT_EQ(static_cast<uint32_t>(actualMode),
              PAYLOAD_CAMERA_VIDEO_ZOOM_MODE_COMBINE);
}

TEST_F(CameraEoZoomModesTest, SuperResolutionMode_SetAndReadBack) {
    g_payload->setPayloadCameraParam(
        const_cast<char*>(PAYLOAD_CAMERA_VIDEO_ZOOM_MODE),
        PAYLOAD_CAMERA_VIDEO_ZOOM_MODE_SUPER_RESOLUTION,
        PARAM_TYPE_UINT32);

    double actualMode = -1;
    ASSERT_TRUE(getCameraSettingByID(
        PAYLOAD_CAMERA_VIDEO_ZOOM_MODE, actualMode, 3000))
        << "No zoom-mode readback after selecting Super Resolution mode.";
    EXPECT_EQ(static_cast<uint32_t>(actualMode),
              PAYLOAD_CAMERA_VIDEO_ZOOM_MODE_SUPER_RESOLUTION);
}

TEST_F(CameraEoZoomModesTest, CombineMode_AllDocumentedFactorsReadBack) {
    g_payload->setPayloadCameraParam(
        const_cast<char*>(PAYLOAD_CAMERA_VIDEO_ZOOM_MODE),
        PAYLOAD_CAMERA_VIDEO_ZOOM_MODE_COMBINE,
        PARAM_TYPE_UINT32);

    double actualMode = -1;
    ASSERT_TRUE(getCameraSettingByID(
        PAYLOAD_CAMERA_VIDEO_ZOOM_MODE, actualMode, 3000))
        << "No zoom-mode readback after selecting Combine mode.";
    ASSERT_EQ(static_cast<uint32_t>(actualMode),
              PAYLOAD_CAMERA_VIDEO_ZOOM_MODE_COMBINE);

    for (uint32_t factor = 0; factor <= kLastCombineFactor; ++factor) {
        SCOPED_TRACE(::testing::Message()
                     << "Combine factor enum " << factor);
        std::cout << "[TEST] Combine zoom factor enum: " << factor
                  << std::endl;

        g_payload->setPayloadCameraParam(
            const_cast<char*>(
                PAYLOAD_CAMERA_VIDEO_ZOOM_COMBINE_FACTOR),
            factor, PARAM_TYPE_UINT32);

        double actualFactor = -1;
        ASSERT_TRUE(getCameraSettingByID(
            PAYLOAD_CAMERA_VIDEO_ZOOM_COMBINE_FACTOR,
            actualFactor, 3000))
            << "No Combine factor readback.";
        EXPECT_EQ(static_cast<uint32_t>(actualFactor), factor);
    }
}

TEST_F(CameraEoZoomModesTest,
       SuperResolutionMode_AllDocumentedFactorsReadBack) {
    g_payload->setPayloadCameraParam(
        const_cast<char*>(PAYLOAD_CAMERA_VIDEO_ZOOM_MODE),
        PAYLOAD_CAMERA_VIDEO_ZOOM_MODE_SUPER_RESOLUTION,
        PARAM_TYPE_UINT32);

    double actualMode = -1;
    ASSERT_TRUE(getCameraSettingByID(
        PAYLOAD_CAMERA_VIDEO_ZOOM_MODE, actualMode, 3000))
        << "No zoom-mode readback after selecting Super Resolution mode.";
    ASSERT_EQ(static_cast<uint32_t>(actualMode),
              PAYLOAD_CAMERA_VIDEO_ZOOM_MODE_SUPER_RESOLUTION);

    for (uint32_t factor = 0;
         factor <= kLastSuperResolutionFactor; ++factor) {
        SCOPED_TRACE(::testing::Message()
                     << "Super Resolution factor enum " << factor);
        std::cout << "[TEST] Super Resolution zoom factor enum: " << factor
                  << std::endl;

        g_payload->setPayloadCameraParam(
            const_cast<char*>(
                PAYLOAD_CAMERA_VIDEO_ZOOM_SUPER_RESOLUTION_FACTOR),
            factor, PARAM_TYPE_UINT32);

        double actualFactor = -1;
        ASSERT_TRUE(getCameraSettingByID(
            PAYLOAD_CAMERA_VIDEO_ZOOM_SUPER_RESOLUTION_FACTOR,
            actualFactor, 3000))
            << "No Super Resolution factor readback.";
        EXPECT_EQ(static_cast<uint32_t>(actualFactor), factor);
    }
}

#else

TEST(CameraEoZoomModesTest, UnsupportedProduct) {
    GTEST_SKIP() << "EO Combine and Super Resolution zoom modes are not "
                    "defined for this product.";
}

#endif
