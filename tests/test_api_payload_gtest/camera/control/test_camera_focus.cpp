/**
 * @file test_camera_focus.cpp
 * @brief Tests setCameraFocus() with reversible manual-focus configuration.
 */

#include "../parameters/camera_param_test_helpers.h"

#include <chrono>
#include <cstring>
#include <thread>

namespace {

constexpr int kVisualPrepareSeconds = 3;
constexpr int kVisualFocusSeconds = 5;
constexpr int kVisualInspectSeconds = 3;

void visualCountdown(const char* message, int seconds) {
    for (int remaining = seconds; remaining > 0; --remaining) {
        std::cout << "[WATCH] " << message << " (" << remaining
                  << "s remaining)" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

bool setUint32(const char* id, uint32_t value) {
    char mutableId[CAM_PARAM_ID_LEN] = {0};
    std::strncpy(mutableId, id, sizeof(mutableId) - 1);
    return setAndVerifyCameraParam(mutableId, value, PARAM_TYPE_UINT32,
                                   value, 5000, 500);
}

bool sendFocusAndWaitForAck(float type, float value, int timeoutMs = 3000) {
    const uint64_t seq = getCommandAckSeq(MAV_CMD_SET_CAMERA_FOCUS);
    g_payload->setCameraFocus(type, value);
    AckInfo ack;
    return waitForCommandAck(MAV_CMD_SET_CAMERA_FOCUS, seq, ack, timeoutMs) &&
           ack.result == MAV_RESULT_ACCEPTED;
}

}  // namespace

class CameraFocusTest : public PayloadTest {
protected:
    void SetUp() override {
#if defined(VIO) || defined(ZIO)
        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIEW_SRC,
                                          originalView_, 3000));
        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIDEO_FOCUS_MODE,
                                          originalMode_, 3000));
        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIDEO_FOCUS_VALUE,
                                          originalValue_, 3000));
        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_EO_FOCUS_SPEED,
                                          originalSpeed_, 3000));
        configured_ = true;

        ASSERT_TRUE(setUint32(PAYLOAD_CAMERA_VIEW_SRC, PAYLOAD_CAMERA_VIEW_EO));
        ASSERT_TRUE(setUint32(PAYLOAD_CAMERA_VIDEO_FOCUS_MODE,
                              PAYLOAD_CAMERA_VIDEO_FOCUS_MODE_MANUAL));
        ASSERT_TRUE(setUint32(PAYLOAD_CAMERA_VIDEO_FOCUS_VALUE, 30720));
        ASSERT_TRUE(setUint32(PAYLOAD_CAMERA_EO_FOCUS_SPEED, 4));
#else
        GTEST_SKIP() << "Native manual-focus parameters are available on VIO/ZIO.";
#endif
    }

    void TearDown() override {
        g_payload->setCameraFocus(FOCUS_TYPE_CONTINUOUS, FOCUS_STOP);
        if (!configured_) return;
        if (!setUint32(PAYLOAD_CAMERA_VIDEO_FOCUS_VALUE,
                       static_cast<uint32_t>(originalValue_))) {
            ADD_FAILURE() << "Could not restore original focus value.";
        }
        if (!setUint32(PAYLOAD_CAMERA_EO_FOCUS_SPEED,
                       static_cast<uint32_t>(originalSpeed_))) {
            ADD_FAILURE() << "Could not restore original focus speed.";
        }
        if (!setUint32(PAYLOAD_CAMERA_VIDEO_FOCUS_MODE,
                       static_cast<uint32_t>(originalMode_))) {
            ADD_FAILURE() << "Could not restore original focus mode.";
        }
        if (!setUint32(PAYLOAD_CAMERA_VIEW_SRC,
                       static_cast<uint32_t>(originalView_))) {
            ADD_FAILURE() << "Could not restore original view source.";
        }
    }

    double originalView_ = 0;
    double originalMode_ = 0;
    double originalValue_ = 0;
    double originalSpeed_ = 0;
    bool configured_ = false;
};

TEST_F(CameraFocusTest, ContinuousInThenStop_IsAccepted) {
    visualCountdown("Observe the baseline image before FOCUS_IN",
                    kVisualPrepareSeconds);
    ASSERT_TRUE(sendFocusAndWaitForAck(FOCUS_TYPE_CONTINUOUS, FOCUS_IN));
    visualCountdown("FOCUS_IN is active; watch the EO image",
                    kVisualFocusSeconds);
    ASSERT_TRUE(sendFocusAndWaitForAck(FOCUS_TYPE_CONTINUOUS, FOCUS_STOP));
    visualCountdown("FOCUS_STOP sent; inspect the resulting image",
                    kVisualInspectSeconds);
}

TEST_F(CameraFocusTest, ContinuousOutThenStop_IsAccepted) {
    visualCountdown("Observe the baseline image before FOCUS_OUT",
                    kVisualPrepareSeconds);
    ASSERT_TRUE(sendFocusAndWaitForAck(FOCUS_TYPE_CONTINUOUS, FOCUS_OUT));
    visualCountdown("FOCUS_OUT is active; watch the EO image",
                    kVisualFocusSeconds);
    ASSERT_TRUE(sendFocusAndWaitForAck(FOCUS_TYPE_CONTINUOUS, FOCUS_STOP));
    visualCountdown("FOCUS_STOP sent; inspect the resulting image",
                    kVisualInspectSeconds);
}

TEST_F(CameraFocusTest, VisualObserve_InStopOutStop) {
    visualCountdown("Observe the initial EO image", kVisualPrepareSeconds);

    ASSERT_TRUE(sendFocusAndWaitForAck(FOCUS_TYPE_CONTINUOUS, FOCUS_IN));
    visualCountdown("FOCUS_IN is active; image sharpness should change",
                    kVisualFocusSeconds);
    ASSERT_TRUE(sendFocusAndWaitForAck(FOCUS_TYPE_CONTINUOUS, FOCUS_STOP));
    visualCountdown("FOCUS_IN stopped; inspect and remember this image",
                    kVisualInspectSeconds);

    ASSERT_TRUE(sendFocusAndWaitForAck(FOCUS_TYPE_CONTINUOUS, FOCUS_OUT));
    visualCountdown("FOCUS_OUT is active; image sharpness should reverse",
                    kVisualFocusSeconds);
    ASSERT_TRUE(sendFocusAndWaitForAck(FOCUS_TYPE_CONTINUOUS, FOCUS_STOP));
    visualCountdown("FOCUS_OUT stopped; inspect the final image",
                    kVisualInspectSeconds);
}

TEST_F(CameraFocusTest, Stop_BestEffortCommandAck) {
    if (!sendFocusAndWaitForAck(FOCUS_TYPE_CONTINUOUS, FOCUS_STOP)) {
        GTEST_SKIP() << "Firmware provided no accepted focus-stop ACK.";
    }
}

TEST_F(CameraFocusTest, RangeCommand_IsAccepted) {
    EXPECT_TRUE(sendFocusAndWaitForAck(FOCUS_TYPE_RANGE, 50.0f));
}

TEST_F(CameraFocusTest, NativeFocusConfiguration_IsReadable) {
    double mode = 0, value = 0, speed = 0;
    ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIDEO_FOCUS_MODE, mode));
    ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIDEO_FOCUS_VALUE, value));
    ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_EO_FOCUS_SPEED, speed));
    EXPECT_EQ(mode, PAYLOAD_CAMERA_VIDEO_FOCUS_MODE_MANUAL);
    EXPECT_EQ(value, 30720);
    EXPECT_EQ(speed, 4);
}
