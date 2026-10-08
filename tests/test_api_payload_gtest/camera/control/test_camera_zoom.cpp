/**
 * @file test_camera_zoom.cpp
 * @brief Tests setCameraZoom() and setCameraZoomTarget() using FOV telemetry.
 */

#include "../parameters/camera_param_test_helpers.h"
#include "../query/camera_query_test_helpers.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>

namespace {

struct Fov {
    double h = 0;
    double v = 0;
};

bool readEoFov(Fov& fov, int timeoutMs = 3000) {
    double cameraId = 0;
    return getCameraFov(CAMERA_EO, cameraId, fov.h, fov.v, timeoutMs) &&
           std::isfinite(fov.h) && std::isfinite(fov.v) &&
           fov.h > 0 && fov.v > 0;
}

double magnification(double baselineDegrees, double zoomedDegrees) {
    constexpr double kPi = 3.14159265358979323846;
    return std::tan(baselineDegrees * kPi / 360.0) /
           std::tan(zoomedDegrees * kPi / 360.0);
}

bool setZoomTargetAndRead(float target, Fov& result, int settleMs = 5000) {
    g_payload->setCameraZoomTarget(target);
    std::this_thread::sleep_for(std::chrono::milliseconds(settleMs));
    return readEoFov(result);
}

}  // namespace

class CameraZoomTest : public PayloadTest {
protected:
    void SetUp() override {
        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIEW_SRC,
                                          originalViewSource_, 3000));
        haveOriginalView_ = true;
        ASSERT_TRUE(readEoFov(originalFov_))
            << "No original EO FOV; zoom cannot be restored safely.";

        char viewId[] = PAYLOAD_CAMERA_VIEW_SRC;
        ASSERT_TRUE(setAndVerifyCameraParam(viewId, PAYLOAD_CAMERA_VIEW_EO,
                                             PARAM_TYPE_UINT32,
                                             PAYLOAD_CAMERA_VIEW_EO));
    }

    void TearDown() override {
        g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_STOP);

        // Derive the original optical magnification from a fresh 1x reference.
        Fov oneX;
        if (setZoomTargetAndRead(1.0f, oneX, 4000)) {
            const double originalZoom = magnification(oneX.h, originalFov_.h);
            if (std::isfinite(originalZoom) && originalZoom >= 1.0 &&
                originalZoom <= 100.0) {
                g_payload->setCameraZoomTarget(static_cast<float>(originalZoom));
                std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            } else {
                ADD_FAILURE() << "Could not derive original zoom from FOV.";
            }
        } else {
            ADD_FAILURE() << "Could not obtain 1x FOV during zoom restoration.";
        }

        if (haveOriginalView_) {
            char viewId[] = PAYLOAD_CAMERA_VIEW_SRC;
            if (!setAndVerifyCameraParam(
                    viewId, static_cast<uint32_t>(originalViewSource_),
                    PARAM_TYPE_UINT32, originalViewSource_)) {
                ADD_FAILURE() << "Could not restore original camera view source.";
            }
        }
    }

    void verifyTarget(float target, double tolerance) {
        Fov oneX;
        Fov zoomed;
        ASSERT_TRUE(setZoomTargetAndRead(1.0f, oneX));
        ASSERT_TRUE(setZoomTargetAndRead(target, zoomed));
        const double horizontal = magnification(oneX.h, zoomed.h);
        const double vertical = magnification(oneX.v, zoomed.v);
        std::cout << "[INFO] target=" << target
                  << " measured H=" << horizontal
                  << " V=" << vertical << std::endl;
        EXPECT_NEAR(horizontal, target, tolerance);
        EXPECT_NEAR(vertical, target, tolerance);
    }

    double originalViewSource_ = 0;
    Fov originalFov_;
    bool haveOriginalView_ = false;
};

TEST_F(CameraZoomTest, ContinuousInThenStop_NarrowsFov) {
    Fov baseline;
    ASSERT_TRUE(setZoomTargetAndRead(1.0f, baseline));
    g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_IN);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_STOP);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    Fov zoomed;
    ASSERT_TRUE(readEoFov(zoomed));
    EXPECT_LT(zoomed.h, baseline.h);
    EXPECT_LT(zoomed.v, baseline.v);
}

TEST_F(CameraZoomTest, ContinuousOutThenStop_WidensFov) {
    Fov zoomed;
    ASSERT_TRUE(setZoomTargetAndRead(3.0f, zoomed));
    g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_OUT);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_STOP);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    Fov wider;
    ASSERT_TRUE(readEoFov(wider));
    EXPECT_GT(wider.h, zoomed.h);
    EXPECT_GT(wider.v, zoomed.v);
}

TEST_F(CameraZoomTest, Stop_BestEffortCommandAck) {
    const uint64_t seq = getCommandAckSeq(MAV_CMD_SET_CAMERA_ZOOM);
    g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_STOP);
    AckInfo ack;
    if (!waitForCommandAck(MAV_CMD_SET_CAMERA_ZOOM, seq, ack, 3000)) {
        GTEST_SKIP() << "Firmware provided no zoom-stop ACK.";
    }
    EXPECT_EQ(ack.result, MAV_RESULT_ACCEPTED);
}

TEST_F(CameraZoomTest, StepInThenOut_CommandsAccepted) {
    expectAckedCommand([] {
        g_payload->setCameraZoom(ZOOM_TYPE_STEP, ZOOM_IN);
    }, MAV_CMD_SET_CAMERA_ZOOM);
    expectAckedCommand([] {
        g_payload->setCameraZoom(ZOOM_TYPE_STEP, ZOOM_OUT);
    }, MAV_CMD_SET_CAMERA_ZOOM);
}

TEST_F(CameraZoomTest, Target2x_IsMeasuredByFov) {
    verifyTarget(2.0f, 0.4);
}

TEST_F(CameraZoomTest, Target3x_IsMeasuredByFov) {
    verifyTarget(3.0f, 0.6);
}

TEST_F(CameraZoomTest, Target5x_IsMeasuredByFov) {
    verifyTarget(5.0f, 1.0);
}
