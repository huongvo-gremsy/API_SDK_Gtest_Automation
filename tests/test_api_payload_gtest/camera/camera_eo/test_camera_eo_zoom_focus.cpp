/**
 * @file test_camera_eo_zoom_focus.cpp
 * @brief EO zoom/focus tests based on camera_eo_set_zoom_focus.cpp.
 */

#include "camera_eo_test_helpers.h"

#include <cmath>

namespace cet = camera_eo_test;

namespace {

struct Fov {
    double horizontal = 0;
    double vertical = 0;
};

bool readEoFov(Fov& value, int timeoutMs = 3000) {
    double cameraId = 0;
    return cet::getCameraFov(CAMERA_EO, cameraId, value.horizontal,
                        value.vertical, timeoutMs) &&
           std::isfinite(value.horizontal) &&
           std::isfinite(value.vertical) &&
           value.horizontal > 0 && value.vertical > 0;
}

double magnification(double oneXDegrees, double zoomedDegrees) {
    constexpr double kPi = 3.14159265358979323846;
    return std::tan(oneXDegrees * kPi / 360.0) /
           std::tan(zoomedDegrees * kPi / 360.0);
}

bool sendZoom(float type, float value) {
    return cet::commandAcceptedOrEoResponsive(
        MAV_CMD_SET_CAMERA_ZOOM,
        [=] { g_payload->setCameraZoom(type, value); });
}

bool sendFocus(float type, float value = 0) {
    return cet::commandAcceptedOrEoResponsive(
        MAV_CMD_SET_CAMERA_FOCUS,
        [=] { g_payload->setCameraFocus(type, value); });
}

void watchDelay(const char* action, int seconds) {
    for (int remaining = seconds; remaining > 0; --remaining) {
        std::cout << "[WATCH] " << action << " (" << remaining << "s)"
                  << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

}  // namespace

class CameraEoZoomFocusTest : public cet::CameraEoTest {
protected:
    void SetUp() override {
        CameraEoTest::SetUp();
        if (::testing::Test::IsSkipped() || ::testing::Test::HasFatalFailure()) return;

#if defined(VIO) || defined(ZIO)
        ASSERT_TRUE(cet::getCameraSettingByID(
            PAYLOAD_CAMERA_VIDEO_ZOOM_SUPER_RESOLUTION_FACTOR,
            originalZoomFactor_, 3000));
        haveZoomFactor_ = true;
        ASSERT_TRUE(cet::setUint32Param(
            PAYLOAD_CAMERA_VIDEO_ZOOM_SUPER_RESOLUTION_FACTOR,
            ZOOM_SUPER_RESOLUTION_1X));

        ASSERT_TRUE(cet::getCameraSettingByID(PAYLOAD_CAMERA_VIDEO_FOCUS_MODE,
                                          originalFocusMode_, 3000));
        ASSERT_TRUE(cet::getCameraSettingByID(PAYLOAD_CAMERA_VIDEO_FOCUS_VALUE,
                                          originalFocusValue_, 3000));
        ASSERT_TRUE(cet::getCameraSettingByID(PAYLOAD_CAMERA_EO_FOCUS_SPEED,
                                          originalFocusSpeed_, 3000));
        haveFocus_ = true;
#elif defined(MB1)
        ASSERT_TRUE(cet::getCameraSettingByID(PAYLOAD_CAMERA_VIDEO_ZOOM_FACTOR,
                                          originalZoomFactor_, 3000));
        haveZoomFactor_ = true;
        ASSERT_TRUE(cet::setUint32Param(PAYLOAD_CAMERA_VIDEO_ZOOM_FACTOR,
                                        ZOOM_EO_1X));
#endif
        std::this_thread::sleep_for(std::chrono::seconds(2));
        ASSERT_TRUE(readEoFov(originalOpticalFov_))
            << "EO FOV is required to restore the original optical zoom.";
        haveOriginalOpticalFov_ = true;
    }

    void TearDown() override {
        if (!setupStarted_) {
            CameraEoTest::TearDown();
            return;
        }
        g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_STOP);
        g_payload->setCameraFocus(FOCUS_TYPE_CONTINUOUS, FOCUS_STOP);

        // Restore the optical lens position independently of the digital
        // super-resolution factor. Derive the starting magnification from a
        // fresh 1x reference, as CAMERA_SETTINGS.zoomLevel is not reliable on
        // this firmware.
        if (haveOriginalOpticalFov_) {
            g_payload->setCameraZoomTarget(1.0f);
            std::this_thread::sleep_for(std::chrono::seconds(4));
            Fov oneX;
            if (readEoFov(oneX)) {
                const double originalZoom = magnification(
                    oneX.horizontal, originalOpticalFov_.horizontal);
                if (std::isfinite(originalZoom) && originalZoom >= 1.0 &&
                    originalZoom <= 100.0) {
                    g_payload->setCameraZoomTarget(
                        static_cast<float>(originalZoom));
                    std::this_thread::sleep_for(std::chrono::seconds(3));
                } else {
                    ADD_FAILURE() << "Could not derive original EO optical zoom.";
                }
            } else {
                ADD_FAILURE() << "No 1x EO FOV during optical zoom restoration.";
            }
        }
#if defined(VIO) || defined(ZIO)
        if (haveFocus_) {
            EXPECT_TRUE(cet::setUint32Param(
                PAYLOAD_CAMERA_VIDEO_FOCUS_MODE,
                PAYLOAD_CAMERA_VIDEO_FOCUS_MODE_MANUAL));
            EXPECT_TRUE(cet::setUint32Param(
                PAYLOAD_CAMERA_VIDEO_FOCUS_VALUE,
                static_cast<uint32_t>(originalFocusValue_)));
            EXPECT_TRUE(cet::setUint32Param(
                PAYLOAD_CAMERA_EO_FOCUS_SPEED,
                static_cast<uint32_t>(originalFocusSpeed_)));
            EXPECT_TRUE(cet::setUint32Param(
                PAYLOAD_CAMERA_VIDEO_FOCUS_MODE,
                static_cast<uint32_t>(originalFocusMode_)));
        }
        if (haveZoomFactor_) {
            EXPECT_TRUE(cet::setUint32Param(
                PAYLOAD_CAMERA_VIDEO_ZOOM_SUPER_RESOLUTION_FACTOR,
                static_cast<uint32_t>(originalZoomFactor_)));
        }
#elif defined(MB1)
        if (haveZoomFactor_) {
            EXPECT_TRUE(cet::setUint32Param(
                PAYLOAD_CAMERA_VIDEO_ZOOM_FACTOR,
                static_cast<uint32_t>(originalZoomFactor_)));
        }
#endif
        CameraEoTest::TearDown();
    }

    Fov originalOpticalFov_;
    double originalZoomFactor_ = 0;
    double originalFocusMode_ = 0;
    double originalFocusValue_ = 0;
    double originalFocusSpeed_ = 0;
    bool haveOriginalOpticalFov_ = false;
    bool haveZoomFactor_ = false;
    bool haveFocus_ = false;
};

TEST_F(CameraEoZoomFocusTest, ExampleFlow_StepInFourThenOutTwo_ChangesFov) {
    Fov baseline;
    ASSERT_TRUE(readEoFov(baseline));
    for (int i = 0; i < 4; ++i) {
        ASSERT_TRUE(sendZoom(ZOOM_TYPE_STEP, ZOOM_IN));
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    Fov zoomedIn;
    ASSERT_TRUE(readEoFov(zoomedIn));
    EXPECT_LT(zoomedIn.horizontal, baseline.horizontal);
    EXPECT_LT(zoomedIn.vertical, baseline.vertical);

    for (int i = 0; i < 2; ++i) {
        ASSERT_TRUE(sendZoom(ZOOM_TYPE_STEP, ZOOM_OUT));
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    Fov zoomedOut;
    ASSERT_TRUE(readEoFov(zoomedOut));
    EXPECT_GT(zoomedOut.horizontal, zoomedIn.horizontal);
    EXPECT_GT(zoomedOut.vertical, zoomedIn.vertical);
}

TEST_F(CameraEoZoomFocusTest, ContinuousInStopOutStop_ChangesFov) {
    Fov baseline;
    ASSERT_TRUE(readEoFov(baseline));
    ASSERT_TRUE(sendZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_IN));
    std::this_thread::sleep_for(std::chrono::seconds(4));
    ASSERT_TRUE(sendZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_STOP));
    Fov zoomed;
    ASSERT_TRUE(readEoFov(zoomed));
    EXPECT_LT(zoomed.horizontal, baseline.horizontal);

    ASSERT_TRUE(sendZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_OUT));
    std::this_thread::sleep_for(std::chrono::seconds(5));
    ASSERT_TRUE(sendZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_STOP));
    Fov wider;
    ASSERT_TRUE(readEoFov(wider));
    EXPECT_GT(wider.horizontal, zoomed.horizontal);
}

TEST_F(CameraEoZoomFocusTest, Range50_70_100_0_AcceptedOrResponsive) {
    const float ranges[] = {50.0f, 70.0f, 100.0f, 0.0f};
    for (const float range : ranges) {
        EXPECT_TRUE(sendZoom(ZOOM_TYPE_RANGE, range))
            << "EO range zoom failed at " << range << "%";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

#if defined(VIO) || defined(ZIO)
TEST_F(CameraEoZoomFocusTest, FocusInStopOutStop_VisualVerification) {
    ASSERT_TRUE(cet::setUint32Param(PAYLOAD_CAMERA_VIDEO_FOCUS_MODE,
                                    PAYLOAD_CAMERA_VIDEO_FOCUS_MODE_MANUAL));
    ASSERT_TRUE(cet::setUint32Param(PAYLOAD_CAMERA_EO_FOCUS_SPEED, 4));
    watchDelay("Observe initial EO focus", 3);
    ASSERT_TRUE(sendFocus(FOCUS_TYPE_CONTINUOUS, FOCUS_IN));
    watchDelay("FOCUS_IN active; image sharpness should change", 5);
    ASSERT_TRUE(sendFocus(FOCUS_TYPE_CONTINUOUS, FOCUS_STOP));
    watchDelay("Inspect focus after stopping IN", 3);
    ASSERT_TRUE(sendFocus(FOCUS_TYPE_CONTINUOUS, FOCUS_OUT));
    watchDelay("FOCUS_OUT active; sharpness should reverse", 5);
    ASSERT_TRUE(sendFocus(FOCUS_TYPE_CONTINUOUS, FOCUS_STOP));
    watchDelay("Inspect final EO focus", 3);
}

TEST_F(CameraEoZoomFocusTest, AutoFocus_CommandAcceptedOrStreamResponsive) {
    EXPECT_TRUE(sendFocus(FOCUS_TYPE_AUTO));
}
#endif
