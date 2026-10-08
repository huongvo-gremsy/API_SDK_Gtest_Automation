#include "api_camera_eo_test_helper.h"

#include <cmath>

namespace cet = camera_eo_test;

namespace {

bool waitForHorizontalFovLessThan(double baseline, cet::FovStatus& output,
                                  int timeoutMs = 8000) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        if (cet::readFovStatus(CAMERA_EO, output, 1200) &&
            output.horizontal > 0.0 &&
            output.horizontal < baseline) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    return false;
}

bool waitForHorizontalFovGreaterThan(double baseline, cet::FovStatus& output,
                                     int timeoutMs = 8000) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        if (cet::readFovStatus(CAMERA_EO, output, 1200) &&
            output.horizontal > baseline) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    return false;
}

}  // namespace

class CameraEoZoomTest : public cet::CameraEoTest {};
class ManualCameraEoZoomTest : public testing::Test {};

#if defined(VIO) || defined(ZIO) || defined(ORUSL)
TEST_F(CameraEoZoomTest, SetCameraZoom_StopIsAcceptedOrHarmless) {
    ASSERT_TRUE(cet::setCameraParam(PAYLOAD_CAMERA_VIEW_SRC,
                                    PAYLOAD_CAMERA_VIEW_EO));

    EXPECT_TRUE(cet::sendAndAcceptIfAcked(
        MAV_CMD_SET_CAMERA_ZOOM,
        [] {
            g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_STOP);
        }))
        << "setCameraZoom(CONTINUOUS, STOP) was rejected.";
}

TEST_F(CameraEoZoomTest, SetCameraZoom_RangeCommandIsAcceptedOrHarmless) {
    ASSERT_TRUE(cet::setCameraParam(PAYLOAD_CAMERA_VIEW_SRC,
                                    PAYLOAD_CAMERA_VIEW_EO));

    EXPECT_TRUE(cet::sendAndAcceptIfAcked(
        MAV_CMD_SET_CAMERA_ZOOM,
        [] {
            g_payload->setCameraZoom(ZOOM_TYPE_RANGE, 0.0F);
        }))
        << "setCameraZoom(RANGE, 0) was rejected.";
}

TEST_F(CameraEoZoomTest, SetCameraZoom_ZoomInDecreasesFov) {
    ASSERT_TRUE(cet::setCameraParam(PAYLOAD_CAMERA_VIEW_SRC,
                                    PAYLOAD_CAMERA_VIEW_EO));
    ASSERT_TRUE(cet::sendAndAcceptIfAcked(
        MAV_CMD_SET_CAMERA_ZOOM,
        [] {
            g_payload->setCameraZoom(ZOOM_TYPE_RANGE, 0.0F);
        }))
        << "Could not reset EO zoom before measuring FOV.";
    std::this_thread::sleep_for(std::chrono::seconds(2));

    cet::FovStatus before;
    ASSERT_TRUE(cet::readFovStatus(CAMERA_EO, before, 4000))
        << "getPayloadCameraFOVStatus(CAMERA_EO) produced no FOV status.";
    ASSERT_GT(before.horizontal, 0.0);
    ASSERT_GT(before.vertical, 0.0);

    for (int i = 0; i < 3; ++i) {
        ASSERT_TRUE(cet::sendAndAcceptIfAcked(
            MAV_CMD_SET_CAMERA_ZOOM,
            [] {
                g_payload->setCameraZoom(ZOOM_TYPE_STEP, ZOOM_IN);
            }))
            << "setCameraZoom(STEP, ZOOM_IN) was rejected.";
        std::this_thread::sleep_for(std::chrono::milliseconds(600));
    }

    cet::FovStatus after;
    EXPECT_TRUE(waitForHorizontalFovLessThan(before.horizontal, after))
        << "ZOOM_IN did not reduce EO horizontal FOV. before="
        << before.horizontal << ", after=" << after.horizontal;

    g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_STOP);
}

TEST_F(CameraEoZoomTest, SetCameraZoom_ZoomOutIncreasesFovAfterZoomIn) {
    ASSERT_TRUE(cet::setCameraParam(PAYLOAD_CAMERA_VIEW_SRC,
                                    PAYLOAD_CAMERA_VIEW_EO));
    ASSERT_TRUE(cet::sendAndAcceptIfAcked(
        MAV_CMD_SET_CAMERA_ZOOM,
        [] {
            g_payload->setCameraZoom(ZOOM_TYPE_RANGE, 0.0F);
        }))
        << "Could not reset EO zoom before measuring FOV.";
    std::this_thread::sleep_for(std::chrono::seconds(2));

    cet::FovStatus wide;
    ASSERT_TRUE(cet::readFovStatus(CAMERA_EO, wide, 4000));
    ASSERT_GT(wide.horizontal, 0.0);

    for (int i = 0; i < 4; ++i) {
        ASSERT_TRUE(cet::sendAndAcceptIfAcked(
            MAV_CMD_SET_CAMERA_ZOOM,
            [] {
                g_payload->setCameraZoom(ZOOM_TYPE_STEP, ZOOM_IN);
            }))
            << "setCameraZoom(STEP, ZOOM_IN) was rejected.";
        std::this_thread::sleep_for(std::chrono::milliseconds(600));
    }

    cet::FovStatus zoomed;
    ASSERT_TRUE(waitForHorizontalFovLessThan(wide.horizontal, zoomed))
        << "Precondition failed: ZOOM_IN did not reduce EO FOV.";

    for (int i = 0; i < 2; ++i) {
        ASSERT_TRUE(cet::sendAndAcceptIfAcked(
            MAV_CMD_SET_CAMERA_ZOOM,
            [] {
                g_payload->setCameraZoom(ZOOM_TYPE_STEP, ZOOM_OUT);
            }))
            << "setCameraZoom(STEP, ZOOM_OUT) was rejected.";
        std::this_thread::sleep_for(std::chrono::milliseconds(600));
    }

    cet::FovStatus after;
    EXPECT_TRUE(waitForHorizontalFovGreaterThan(zoomed.horizontal, after))
        << "ZOOM_OUT did not increase EO horizontal FOV. zoomed="
        << zoomed.horizontal << ", after=" << after.horizontal;

    g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_STOP);
}
#else
TEST(CameraEoZoomTest, UnsupportedProduct) {
    GTEST_SKIP() << "EO Set Zoom is only defined for VIO, ZIO, and ORUSL.";
}
#endif

TEST_F(ManualCameraEoZoomTest, ManualGetFovCameraEo) {
    cet::FovStatus status;
    if (cet::readFovStatus(CAMERA_EO, status, 4000)) {
        // std::cout << "EO FOV: cameraId=" << status.cameraId
        //           << ", hfov=" << status.horizontal
        //           << ", vfov=" << status.vertical
        //           << std::endl;
    } else {
        std::cout << "Failed to read EO FOV." << std::endl;
    }
}
