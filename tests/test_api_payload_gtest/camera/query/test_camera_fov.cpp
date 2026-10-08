/**
 * @file test_camera_fov.cpp
 * @brief Read-only getPayloadCameraFOVStatus() tests.
 */

#include "camera_query_test_helpers.h"

#include <cmath>
#include <iostream>

namespace {

struct FovInfo {
    double cameraId = -1;
    double horizontal = 0;
    double vertical = 0;
};

bool readFov(camera_type_t type, FovInfo& info) {
    return getCameraFov(type, info.cameraId, info.horizontal,
                        info.vertical, 3000);
}

void expectValidFov(const FovInfo& info) {
    EXPECT_TRUE(std::isfinite(info.cameraId));
    EXPECT_TRUE(std::isfinite(info.horizontal));
    EXPECT_TRUE(std::isfinite(info.vertical));
    EXPECT_GT(info.horizontal, 0.0);
    EXPECT_LE(info.horizontal, 180.0);
    EXPECT_GT(info.vertical, 0.0);
    EXPECT_LE(info.vertical, 180.0);
}

}  // namespace

class CameraFovTest : public PayloadTest {};

TEST_F(CameraFovTest, EO_EventArrivesAndAnglesAreValid) {
    FovInfo info;
    ASSERT_TRUE(readFov(CAMERA_EO, info))
        << "No EO CAMERA_FOV_STATUS response within 3000 ms.";
    expectValidFov(info);
    std::cout << "[INFO] EO FOV: cameraId=" << info.cameraId
              << " hfov=" << info.horizontal
              << " vfov=" << info.vertical << std::endl;
}

#if defined(VIO) || defined(MB1) || defined(ORUSL)
TEST_F(CameraFovTest, IR_EventArrivesAndAnglesAreValid) {
    FovInfo info;
    ASSERT_TRUE(readFov(CAMERA_IR, info))
        << "No IR CAMERA_FOV_STATUS response within 3000 ms.";
    expectValidFov(info);
    std::cout << "[INFO] IR FOV: cameraId=" << info.cameraId
              << " hfov=" << info.horizontal
              << " vfov=" << info.vertical << std::endl;
}
#endif

TEST_F(CameraFovTest, RepeatedEORead_IsStable) {
    FovInfo first;
    FovInfo second;
    ASSERT_TRUE(readFov(CAMERA_EO, first));
    ASSERT_TRUE(readFov(CAMERA_EO, second));
    EXPECT_EQ(second.cameraId, first.cameraId);
    EXPECT_NEAR(second.horizontal, first.horizontal, 0.5);
    EXPECT_NEAR(second.vertical, first.vertical, 0.5);
}

TEST_F(CameraFovTest, EO_AspectRatioIsPlausible) {
    FovInfo info;
    ASSERT_TRUE(readFov(CAMERA_EO, info));
    ASSERT_GT(info.vertical, 0.0);
    const double fovRatio = info.horizontal / info.vertical;
    EXPECT_GT(fovRatio, 0.5);
    EXPECT_LT(fovRatio, 3.0);
}
