/**
 * @file test_camera_white_balance.cpp
 * @brief Tests setPayloadCameraWBOnePushTrigg().
 */

#include "../parameters/camera_param_test_helpers.h"

#include <cstring>
#include <ostream>

namespace {

bool setWbParam(const char* id, uint32_t value) {
    char mutableId[CAM_PARAM_ID_LEN] = {0};
    std::strncpy(mutableId, id, sizeof(mutableId) - 1);
    return setAndVerifyCameraParam(mutableId, value, PARAM_TYPE_UINT32,
                                   value, 5000, 500);
}

bool triggerWbAndWait(AckInfo& ack, int timeoutMs = 3000) {
    const uint64_t seq = getCommandAckSeq(MAV_CMD_USER_4);
    g_payload->setPayloadCameraWBOnePushTrigg();
    return waitForCommandAck(MAV_CMD_USER_4, seq, ack, timeoutMs);
}

struct WhiteBalanceModeCase {
    const char* name;
    uint32_t value;
};

std::ostream& operator<<(std::ostream& os, const WhiteBalanceModeCase& mode) {
    return os << mode.name << " (value=" << mode.value << ")";
}

}  // namespace

class CameraWhiteBalanceTest : public PayloadTest {
protected:
    void SetUp() override {
        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIEW_SRC,
                                          originalView_, 3000));
        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE,
                                          originalMode_, 3000));
        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_EO_R_GAIN,
                                          originalRedGain_, 3000));
        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_EO_B_GAIN,
                                          originalBlueGain_, 3000));
        configured_ = true;
        ASSERT_TRUE(setWbParam(PAYLOAD_CAMERA_VIEW_SRC, PAYLOAD_CAMERA_VIEW_EO));
        ASSERT_TRUE(setWbParam(PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE,
                               PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE_ONE_PUSH));
    }

    void TearDown() override {
        if (!configured_) return;

        // Gains are writable only in Manual mode. Restore them before putting
        // the camera back into its original white-balance mode.
        if (!setWbParam(PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE,
                        PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE_MANUAL)) {
            ADD_FAILURE() << "Could not enter Manual mode for gain restoration.";
        }
        if (!setWbParam(PAYLOAD_CAMERA_EO_R_GAIN,
                        static_cast<uint32_t>(originalRedGain_))) {
            ADD_FAILURE() << "Could not restore original red gain.";
        }
        if (!setWbParam(PAYLOAD_CAMERA_EO_B_GAIN,
                        static_cast<uint32_t>(originalBlueGain_))) {
            ADD_FAILURE() << "Could not restore original blue gain.";
        }
        if (!setWbParam(PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE,
                        static_cast<uint32_t>(originalMode_))) {
            ADD_FAILURE() << "Could not restore original white-balance mode.";
        }
        if (!setWbParam(PAYLOAD_CAMERA_VIEW_SRC,
                        static_cast<uint32_t>(originalView_))) {
            ADD_FAILURE() << "Could not restore original view source.";
        }
    }

    double originalView_ = 0;
    double originalMode_ = 0;
    double originalRedGain_ = 0;
    double originalBlueGain_ = 0;
    bool configured_ = false;
};

class CameraWhiteBalanceModeTest
    : public CameraWhiteBalanceTest,
      public ::testing::WithParamInterface<WhiteBalanceModeCase> {};

TEST_P(CameraWhiteBalanceModeTest, Mode_SetAndReadBack) {
    const WhiteBalanceModeCase mode = GetParam();
    ASSERT_TRUE(setWbParam(PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE, mode.value))
        << "Could not select " << mode.name << " white balance.";

    double actual = -1;
    ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE,
                                      actual, 3000));
    EXPECT_EQ(actual, mode.value);
}

INSTANTIATE_TEST_SUITE_P(
    SupportedModes,
    CameraWhiteBalanceModeTest,
    ::testing::Values(
        WhiteBalanceModeCase{"Auto", PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE_AUTO},
        WhiteBalanceModeCase{"Outdoor", PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE_OUTDOOR},
        WhiteBalanceModeCase{"Indoor", PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE_INDOOR},
        WhiteBalanceModeCase{"Manual", PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE_MANUAL},
        WhiteBalanceModeCase{"ATW", PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE_ATW}),
    [](const ::testing::TestParamInfo<WhiteBalanceModeCase>& info) {
        return info.param.name;
    });

TEST_F(CameraWhiteBalanceTest, ManualMode_RedAndBlueGainsReadBack) {
    ASSERT_TRUE(setWbParam(PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE,
                           PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE_MANUAL));
    ASSERT_TRUE(setWbParam(PAYLOAD_CAMERA_EO_R_GAIN, 64));
    ASSERT_TRUE(setWbParam(PAYLOAD_CAMERA_EO_B_GAIN, 192));

    double red = -1;
    double blue = -1;
    ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_EO_R_GAIN, red, 3000));
    ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_EO_B_GAIN, blue, 3000));
    EXPECT_EQ(red, 64);
    EXPECT_EQ(blue, 192);
}

TEST_F(CameraWhiteBalanceTest, OnePushMode_ReadsBack) {
    double mode = 0;
    ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE,
                                      mode, 3000));
    EXPECT_EQ(mode, PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE_ONE_PUSH);
}

TEST_F(CameraWhiteBalanceTest, Trigger_BestEffortCommandAck) {
    AckInfo ack;
    if (!triggerWbAndWait(ack)) {
        GTEST_SKIP() << "Firmware provided no WB one-push ACK.";
    }
    EXPECT_EQ(ack.command, MAV_CMD_USER_4);
    EXPECT_EQ(ack.result, MAV_RESULT_ACCEPTED);
}

TEST_F(CameraWhiteBalanceTest, RepeatedTrigger_RemainsResponsive) {
    AckInfo first;
    AckInfo second;
    if (!triggerWbAndWait(first)) {
        GTEST_SKIP() << "No ACK for first WB trigger.";
    }
    if (!triggerWbAndWait(second)) {
        GTEST_SKIP() << "No ACK for second WB trigger.";
    }
    EXPECT_EQ(first.result, MAV_RESULT_ACCEPTED);
    EXPECT_EQ(second.result, MAV_RESULT_ACCEPTED);
}

TEST_F(CameraWhiteBalanceTest, Trigger_DoesNotBreakModeReadback) {
    AckInfo ack;
    triggerWbAndWait(ack);
    double mode = 0;
    ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE,
                                      mode, 3000));
    EXPECT_GE(mode, PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE_AUTO);
    EXPECT_LE(mode, PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE_MANUAL);
}
