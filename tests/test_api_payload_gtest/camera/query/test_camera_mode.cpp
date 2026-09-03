/**
 * @file test_camera_mode.cpp
 * @brief Reversible camera-mode tests.
 *
 * Some supported firmware accepts MAV_CMD_SET_CAMERA_MODE but continues to
 * report the previous CAMERA_SETTINGS.mode_id. Setter tests therefore prefer
 * readback and fall back to a command-specific accepted ACK. Getter tests do
 * not use that fallback: they require a real CAMERA_SETTINGS response.
 */

#include "camera_query_test_helpers.h"

#include <cstdint>
#include <iostream>

namespace {

bool isKnownCameraMode(double mode) {
    const int value = static_cast<int>(mode);
    return value == CAMERA_MODE_IMAGE ||
           value == CAMERA_MODE_VIDEO ||
           value == CAMERA_MODE_IMAGE_SURVEY;
}

const char* cameraModeName(int mode) {
    switch (mode) {
        case CAMERA_MODE_IMAGE: return "IMAGE";
        case CAMERA_MODE_VIDEO: return "VIDEO";
        case CAMERA_MODE_IMAGE_SURVEY: return "IMAGE_SURVEY";
        default: return "UNKNOWN";
    }
}

// Prefer observable CAMERA_SETTINGS state. This firmware is also known to
// apply/accept mode changes without updating mode_id, so a command-specific
// accepted ACK is the documented fallback.
bool setModeStateOrAcceptedAck(CAMERA_MODE target, int timeoutMs = 4000) {
    if (setAndVerifyCameraMode(target, timeoutMs, 500)) {
        std::cout << "[INFO] Mode readback reached "
                  << cameraModeName(static_cast<int>(target)) << std::endl;
        return true;
    }

    const uint64_t ackSeq = getCommandAckSeq(MAV_CMD_SET_CAMERA_MODE);
    g_payload->setPayloadCameraMode(target);

    AckInfo ack;
    if (!waitForCommandAck(MAV_CMD_SET_CAMERA_MODE, ackSeq, ack, 3000)) {
        std::cout << "[ERROR] No state readback or MAV_CMD_SET_CAMERA_MODE ACK."
                  << std::endl;
        return false;
    }

    std::cout << "[INFO] Mode " << cameraModeName(static_cast<int>(target))
              << " verified by command ACK; result="
              << static_cast<int>(ack.result) << std::endl;
    return ack.result == MAV_RESULT_ACCEPTED;
}

}  // namespace

class CameraModeTest : public PayloadTest {
protected:
    void SetUp() override {
        ASSERT_TRUE(getCameraMode(originalMode_, 3000))
            << "No CAMERA_SETTINGS response while reading the original mode.";
        ASSERT_TRUE(isKnownCameraMode(originalMode_))
            << "Payload reported invalid initial camera mode " << originalMode_ << ".";
        std::cout << "[INFO] Original camera mode: "
                  << cameraModeName(static_cast<int>(originalMode_))
                  << " (" << originalMode_ << ")" << std::endl;
    }

    void TearDown() override {
        if (!setModeStateOrAcceptedAck(static_cast<CAMERA_MODE>(
                static_cast<int>(originalMode_)))) {
            ADD_FAILURE() << "Could not restore original camera mode "
                          << originalMode_ << ".";
        }
    }

    double originalMode_ = CAMERA_MODE_IMAGE;
};

TEST_F(CameraModeTest, GetMode_EventArrives) {
    // CameraModeTest.GetMode_EventArrives
    double mode = -1;
    ASSERT_TRUE(getCameraMode(mode, 3000))
        << "No new CAMERA_SETTINGS event within 3000 ms.";
    EXPECT_TRUE(isKnownCameraMode(mode))
        << "Unexpected camera mode value " << mode << ".";
}

TEST_F(CameraModeTest, SetImage_StateOrAcceptedAck) {
    // CameraModeTest.SetImage_StateOrAcceptedAck
    EXPECT_TRUE(setModeStateOrAcceptedAck(CAMERA_MODE_IMAGE))
        << "Camera neither reported IMAGE mode nor accepted the mode command.";
}

TEST_F(CameraModeTest, SetVideo_StateOrAcceptedAck) {
    // CameraModeTest.SetVideo_StateOrAcceptedAck
    EXPECT_TRUE(setModeStateOrAcceptedAck(CAMERA_MODE_VIDEO))
        << "Camera neither reported VIDEO mode nor accepted the mode command.";
}

TEST_F(CameraModeTest, InvalidMode_IsRejectedOrIgnored) {
    // CameraModeTest.InvalidMode_IsRejectedOrIgnored
    constexpr int kInvalidMode = 255;
    const uint64_t ackSeq = getCommandAckSeq(MAV_CMD_SET_CAMERA_MODE);
    g_payload->setPayloadCameraMode(static_cast<CAMERA_MODE>(kInvalidMode));

    AckInfo ack;
    const bool receivedAck =
        waitForCommandAck(MAV_CMD_SET_CAMERA_MODE, ackSeq, ack, 1500);
    const bool explicitlyRejected = receivedAck && ack.result != MAV_RESULT_ACCEPTED;

    double currentMode = -1;
    const bool receivedState = getCameraMode(currentMode, 3000);
    const bool ignored = receivedState &&
        static_cast<int>(currentMode) == static_cast<int>(originalMode_);

    EXPECT_TRUE(explicitlyRejected || ignored)
        << "Invalid mode " << kInvalidMode
        << " was not rejected and changed the reported mode from "
        << originalMode_ << " to " << currentMode
        << " (ACK received=" << receivedAck
        << ", ACK result=" << static_cast<int>(ack.result) << ").";
}

TEST_F(CameraModeTest, OriginalMode_IsRestored) {
    //  CameraModeTest.OriginalMode_IsRestored
    const CAMERA_MODE alternateMode =
        static_cast<int>(originalMode_) == CAMERA_MODE_VIDEO
            ? CAMERA_MODE_IMAGE
            : CAMERA_MODE_VIDEO;

    ASSERT_TRUE(setModeStateOrAcceptedAck(alternateMode))
        << "Could not exercise an alternate camera mode before restoration.";
    ASSERT_TRUE(setModeStateOrAcceptedAck(static_cast<CAMERA_MODE>(
        static_cast<int>(originalMode_))))
        << "Could not restore the original mode inside the test.";

    // If this firmware updates mode_id, verify it strictly. If it does not,
    // the accepted ACK above is the strongest signal exposed by this SDK.
    double restoredMode = -1;
    ASSERT_TRUE(getCameraMode(restoredMode, 3000))
        << "No CAMERA_SETTINGS response after restoring the original mode.";
    EXPECT_EQ(static_cast<int>(restoredMode), static_cast<int>(originalMode_))
        << "Reported camera mode did not return to its original value.";
}
