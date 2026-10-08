#include "api_camera_eo_test_helper.h"

#include <cmath>
#include <string>

namespace cet = camera_eo_test;

#if defined(VIO) || defined(ORUSL)

namespace {

bool selectCombineZoomMode() {
#if defined(PAYLOAD_CAMERA_VIDEO_ZOOM_MODE)
    return cet::setCameraParam(PAYLOAD_CAMERA_VIDEO_ZOOM_MODE,
                               PAYLOAD_CAMERA_VIDEO_ZOOM_MODE_COMBINE);
#else
    return true;
#endif
}

bool selectSuperResolutionZoomMode() {
#if defined(PAYLOAD_CAMERA_VIDEO_ZOOM_MODE)
    return cet::setCameraParam(PAYLOAD_CAMERA_VIDEO_ZOOM_MODE,
                               PAYLOAD_CAMERA_VIDEO_ZOOM_MODE_SUPER_RESOLUTION);
#else
    return true;
#endif
}

bool readEoZoomLevel(double& value, int timeoutMs = 3000) {
    g_payload->requestParamValue(PARAM_EO_ZOOM_LEVEL);

    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        std::lock_guard<std::mutex> lock(g_cb.m);
        const auto it = g_cb.payloadParamValueByIndex.find(PARAM_EO_ZOOM_LEVEL);
        if (it != g_cb.payloadParamValueByIndex.end()) {
            value = it->second;
            std::cout << "--> Param_id: EO_ZOOM_LEVEL"
                      << ", value: " << value << std::endl;
            return true;
        }
    }
    return false;
}

struct ZoomReadResult {
    cet::FovStatus fov;
    double zoomLevel = -1.0;
};

bool requestTargetAndRead(float target, ZoomReadResult& result) {
    if (!cet::sendAndAcceptIfAcked(
            MAV_CMD_USER_4,
            [target] {
                g_payload->setCameraZoomTarget(target);
            })) {
        return false;
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));

    if (!cet::readFovStatus(CAMERA_EO, result.fov, 4000) ||
        result.fov.horizontal <= 0.0 ||
        result.fov.vertical <= 0.0 ||
        !std::isfinite(result.fov.horizontal) ||
        !std::isfinite(result.fov.vertical)) {
        return false;
    }

    return readEoZoomLevel(result.zoomLevel, 3000);
}
bool requestTargetAndReadFov(float target, cet::FovStatus& fov) {
    if (!cet::sendAndAcceptIfAcked(
            MAV_CMD_USER_4,
            [target] {
                g_payload->setCameraZoomTarget(target);
            })) {
        return false;
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));
    return cet::readFovStatus(CAMERA_EO, fov, 4000) &&
           fov.horizontal > 0.0 &&
           fov.vertical > 0.0 &&
           std::isfinite(fov.horizontal) &&
           std::isfinite(fov.vertical);
}

}  // namespace

class CameraEoZoomTargetTest : public cet::CameraEoTest {};

// TEST_F(CameraEoZoomTargetTest, SetCameraZoomTarget_OneXAcceptedOrHarmless) {
//     ASSERT_TRUE(selectCombineZoomMode());

//     cet::FovStatus baseline;
//     ASSERT_TRUE(requestTargetAndReadFov(1.0F, baseline))
//         << "Could not set 1.0x target or read EO baseline FOV.";
// }

// class CameraEoZoomTargetValueTest
//     : public cet::CameraEoTest,
    //   public ::testing::WithParamInterface<float> {
    //   };
bool zoomLevelMatchesTarget(double zoomLevel, float target) {
    const double tolerance = std::max(0.5, static_cast<double>(target) * 0.05);
    return std::fabs(zoomLevel - static_cast<double>(target)) <= tolerance;
}
static const std::vector<float> kZoomTargets_CB = {
    1.0F,
    13.5F,
    25.0F,
    33.5F,
    233.5F,
    240.0F,
    30.0F,
    5.0F,
};

TEST_F(CameraEoZoomTargetTest, ZoomTargetSequence_CB) {
    ASSERT_TRUE(selectCombineZoomMode());
    // ============================================================
    // Establish 1.0x baseline ONCE
    // ============================================================
    ZoomReadResult baseline;

    ASSERT_TRUE(requestTargetAndRead(1.0F, baseline))
        << "Could not establish the 1.0x EO FOV/zoom-level baseline.";

    ASSERT_GT(baseline.fov.horizontal, 0.0F)
        << "Invalid baseline horizontal FOV: "
        << baseline.fov.horizontal;

    ASSERT_GT(baseline.fov.vertical, 0.0F)
        << "Invalid baseline vertical FOV: "
        << baseline.fov.vertical;

    ASSERT_TRUE(zoomLevelMatchesTarget(baseline.zoomLevel, 1.0F))
        << "PARAM_EO_ZOOM_LEVEL at baseline does not match commanded 1.0x: "
        << "reported=" << baseline.zoomLevel;

    std::cout << "[  INFO  ] Baseline 1.0x: "
              << "HFOV=" << baseline.fov.horizontal
              << ", VFOV=" << baseline.fov.vertical
              << ", ZOOM_LEVEL=" << baseline.zoomLevel
              << std::endl;

    std::this_thread::sleep_for(
        std::chrono::seconds(4));

    // ============================================================
    // Sequential zoom test
    //
    // 13.5 → 25 → 33.5 → 233.5 → 240 → 30 → 5
    //
    // No reset to 1.0 between targets.
    // ============================================================
    for (const float target : kZoomTargets_CB) {

        // 1.0x was already used to establish the baseline.
        if (target == 1.0F) {
            continue;
        }

        SCOPED_TRACE(
            "targetZoom=" + std::to_string(target));

        ZoomReadResult result;

        ASSERT_TRUE(requestTargetAndRead(target, result))
            << "Could not set target zoom "
            << target << "x or read valid EO FOV/zoom-level.";

        ASSERT_GT(result.fov.horizontal, 0.0F)
            << "Invalid horizontal FOV at "
            << target << "x: "
            << result.fov.horizontal;

        ASSERT_GT(result.fov.vertical, 0.0F)
            << "Invalid vertical FOV at "
            << target << "x: "
            << result.fov.vertical;

        ASSERT_LT(result.fov.horizontal, baseline.fov.horizontal)
            << "Target " << target
            << "x did not reduce EO horizontal FOV. "
            << "baseline=" << baseline.fov.horizontal
            << ", actual=" << result.fov.horizontal;

        ASSERT_LT(result.fov.vertical, baseline.fov.vertical)
            << "Target " << target
            << "x did not reduce EO vertical FOV. "
            << "baseline=" << baseline.fov.vertical
            << ", actual=" << result.fov.vertical;

        // NEW: cross-check FOV against the reported zoom level parameter,
        // not just against the commanded target.
        ASSERT_TRUE(zoomLevelMatchesTarget(result.zoomLevel, target))
            << "PARAM_EO_ZOOM_LEVEL does not match commanded target "
            << target << "x: reported=" << result.zoomLevel;

        std::cout << "[  INFO  ] targetZoom="
                  << target
                  << ", HFOV=" << result.fov.horizontal
                  << ", VFOV=" << result.fov.vertical
                  << ", ZOOM_LEVEL=" << result.zoomLevel
                  << std::endl;

        std::this_thread::sleep_for(
            std::chrono::seconds(2));
    }

    g_payload->setCameraZoomTarget(1.0F);
    std::this_thread::sleep_for(std::chrono::seconds(3));
}

static const std::vector<float> kZoomTargets_SR = {
    1.0F,
    13.5F,
    12.5F,
    20.0F,
    25.0F,
    25.5F,
    30.0F,
};

TEST_F(CameraEoZoomTargetTest, ZoomTargetSequence_SR) {
    ASSERT_TRUE(selectSuperResolutionZoomMode());
    // ============================================================
    // Establish 1.0x baseline ONCE
    // ============================================================
    ZoomReadResult baseline;

    ASSERT_TRUE(requestTargetAndRead(1.0F, baseline))
        << "Could not establish the 1.0x EO FOV/zoom-level baseline.";

    ASSERT_GT(baseline.fov.horizontal, 0.0F)
        << "Invalid baseline horizontal FOV: "
        << baseline.fov.horizontal;

    ASSERT_GT(baseline.fov.vertical, 0.0F)
        << "Invalid baseline vertical FOV: "
        << baseline.fov.vertical;

    ASSERT_TRUE(zoomLevelMatchesTarget(baseline.zoomLevel, 1.0F))
        << "PARAM_EO_ZOOM_LEVEL at baseline does not match commanded 1.0x: "
        << "reported=" << baseline.zoomLevel;

    std::cout << "[  INFO  ] Baseline 1.0x: "
              << "HFOV=" << baseline.fov.horizontal
              << ", VFOV=" << baseline.fov.vertical
              << ", ZOOM_LEVEL=" << baseline.zoomLevel
              << std::endl;

    std::this_thread::sleep_for(
        std::chrono::seconds(4));

    // ============================================================
    // Sequential zoom test
    //
    // 12.5 → 20 → 25 → 25.5 → 30
    //
    // No reset to 1.0 between targets.
    // ============================================================
    for (const float target : kZoomTargets_SR) {

        // 1.0x was already used to establish the baseline.
        if (target == 1.0F) {
            continue;
        }

        SCOPED_TRACE(
            "targetZoom=" + std::to_string(target));

        ZoomReadResult result;

        ASSERT_TRUE(requestTargetAndRead(target, result))
            << "Could not set target zoom "
            << target << "x or read valid EO FOV/zoom-level.";

        ASSERT_GT(result.fov.horizontal, 0.0F)
            << "Invalid horizontal FOV at "
            << target << "x: "
            << result.fov.horizontal;

        ASSERT_GT(result.fov.vertical, 0.0F)
            << "Invalid vertical FOV at "
            << target << "x: "
            << result.fov.vertical;

        ASSERT_LT(result.fov.horizontal, baseline.fov.horizontal)
            << "Target " << target
            << "x did not reduce EO horizontal FOV. "
            << "baseline=" << baseline.fov.horizontal
            << ", actual=" << result.fov.horizontal;

        ASSERT_LT(result.fov.vertical, baseline.fov.vertical)
            << "Target " << target
            << "x did not reduce EO vertical FOV. "
            << "baseline=" << baseline.fov.vertical
            << ", actual=" << result.fov.vertical;

        // NEW: cross-check FOV against the reported zoom level parameter,
        // not just against the commanded target.
        ASSERT_TRUE(zoomLevelMatchesTarget(result.zoomLevel, target))
            << "PARAM_EO_ZOOM_LEVEL does not match commanded target "
            << target << "x: reported=" << result.zoomLevel;

        std::cout << "[  INFO  ] targetZoom="
                  << target
                  << ", HFOV=" << result.fov.horizontal
                  << ", VFOV=" << result.fov.vertical
                  << ", ZOOM_LEVEL=" << result.zoomLevel
                  << std::endl;

        std::this_thread::sleep_for(
            std::chrono::seconds(2));
    }

    g_payload->setCameraZoomTarget(1.0F);
    std::this_thread::sleep_for(std::chrono::seconds(3));
}

/*
TEST_F(CameraEoZoomTargetTest,
       SetCameraZoomTarget_ExampleSequenceNarrowsOrMaintainsFov) {
    ASSERT_TRUE(selectCombineZoomMode());

    constexpr float targets[] = {
        1.0F, 13.5F, 25.0F, 33.5F, 233.5F, 300.0F};
    cet::FovStatus previous;
    ASSERT_TRUE(requestTargetAndReadFov(targets[0], previous));

    bool observedZoomChange = false;
    for (size_t i = 1; i < sizeof(targets) / sizeof(targets[0]); ++i) {
        cet::FovStatus current;
        ASSERT_TRUE(requestTargetAndReadFov(targets[i], current))
            << "Could not set target zoom " << targets[i]
            << "x or read EO FOV.";

        EXPECT_LE(current.horizontal, previous.horizontal + 0.5)
            << "EO horizontal FOV increased unexpectedly at target "
            << targets[i] << "x.";
        EXPECT_LE(current.vertical, previous.vertical + 0.5)
            << "EO vertical FOV increased unexpectedly at target "
            << targets[i] << "x.";

        observedZoomChange = observedZoomChange ||
                             current.horizontal < previous.horizontal ||
                             current.vertical < previous.vertical;
        previous = current;
    }

    EXPECT_TRUE(observedZoomChange)
        << "Target zoom sequence did not change EO FOV.";
}*/

#else

TEST(CameraEoZoomTargetTest, UnsupportedProduct) {
    GTEST_SKIP() << "setCameraZoomTarget() tests are enabled only for VIO and ORUSL.";
}

#endif
