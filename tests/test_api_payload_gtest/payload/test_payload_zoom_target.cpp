/** @file test_payload_zoom_target.cpp */
#include "payload_example_test_helpers.h"
#include "../camera/camera_eo/camera_eo_test_helpers.h"
#include "../telemetry/udp_telemetry_capture.h"

#include <cmath>

namespace cet = camera_eo_test;
namespace tt = telemetry_test;

class PayloadZoomTargetEncodingTest : public tt::TelemetryPacketTest {};

TEST_F(PayloadZoomTargetEncodingTest, ExampleTargets_EncodeExactTargetPosition) {
    const float targets[] = {1.0f, 13.5f, 25.0f, 33.5f, 233.5f, 300.0f};
    for (const float target : targets) {
        mavlink_message_t message{};
        ASSERT_TRUE(capture_->sendAndCapture(
            MAVLINK_MSG_ID_COMMAND_LONG,
            [=](PayloadSdkInterface& sdk) { sdk.setCameraZoomTarget(target); },
            message));
        mavlink_command_long_t command{};
        mavlink_msg_command_long_decode(&message, &command);
        EXPECT_EQ(command.command, MAV_CMD_USER_4);
        EXPECT_FLOAT_EQ(command.param1, 2.0f);
        EXPECT_FLOAT_EQ(command.param2, 0.0f);
        EXPECT_FLOAT_EQ(command.param3,
                        static_cast<float>(PAYLOADSDK_ZOOM_POS));
        EXPECT_FLOAT_EQ(command.param4, target);
    }
}

namespace {
struct Fov { double h = 0; double v = 0; };
bool readEoFov(Fov& value, int timeoutMs = 4000) {
    double id = 0;
    return getCameraFov(CAMERA_EO, id, value.h, value.v, timeoutMs) &&
           value.h > 0 && value.v > 0;
}
}  // namespace

class PayloadZoomTargetHardwareTest : public cet::CameraEoTest {
protected:
    void SetUp() override {
        CameraEoTest::SetUp();
        if (::testing::Test::IsSkipped() || ::testing::Test::HasFatalFailure()) return;
        ASSERT_TRUE(readEoFov(original_));
        haveOriginal_ = true;
#if defined(VIO) || defined(ZIO)
        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIDEO_ZOOM_MODE,
                                          originalMode_, 3000));
        haveMode_ = true;
        ASSERT_TRUE(cet::setUint32Param(
            PAYLOAD_CAMERA_VIDEO_ZOOM_MODE,
            PAYLOAD_CAMERA_VIDEO_ZOOM_MODE_COMBINE));
#endif
    }
    void TearDown() override {
        if (setupStarted_) {
            g_payload->setCameraZoomTarget(1.0f);
            std::this_thread::sleep_for(std::chrono::seconds(4));
            if (haveOriginal_) {
                Fov oneX;
                if (readEoFov(oneX)) {
                    constexpr double pi = 3.14159265358979323846;
                    const double zoom =
                        std::tan(oneX.h * pi / 360.0) /
                        std::tan(original_.h * pi / 360.0);
                    if (std::isfinite(zoom) && zoom >= 1.0 && zoom <= 300.0) {
                        g_payload->setCameraZoomTarget(static_cast<float>(zoom));
                        std::this_thread::sleep_for(std::chrono::seconds(3));
                    }
                }
            }
#if defined(VIO) || defined(ZIO)
            if (haveMode_) {
                EXPECT_TRUE(cet::setUint32Param(
                    PAYLOAD_CAMERA_VIDEO_ZOOM_MODE,
                    static_cast<uint32_t>(originalMode_)));
            }
#endif
        }
        CameraEoTest::TearDown();
    }
    Fov original_;
    double originalMode_ = 0;
    bool haveOriginal_ = false;
    bool haveMode_ = false;
};

TEST_F(PayloadZoomTargetHardwareTest, OneThenThirteenPointFive_NarrowsFov) {
    g_payload->setCameraZoomTarget(1.0f);
    std::this_thread::sleep_for(std::chrono::seconds(5));
    Fov oneX;
    ASSERT_TRUE(readEoFov(oneX));

    g_payload->setCameraZoomTarget(13.5f);
    std::this_thread::sleep_for(std::chrono::seconds(6));
    Fov zoomed;
    ASSERT_TRUE(readEoFov(zoomed));
    EXPECT_LT(zoomed.h, oneX.h);
    EXPECT_LT(zoomed.v, oneX.v);
}
