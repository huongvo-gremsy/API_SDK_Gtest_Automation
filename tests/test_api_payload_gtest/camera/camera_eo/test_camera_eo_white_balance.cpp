/**
 * @file test_camera_eo_white_balance.cpp
 * @brief EO white-balance modes, manual gains, and one-push trigger.
 */

#include "camera_eo_test_helpers.h"

#include <ostream>

namespace cet = camera_eo_test;

namespace {

struct WhiteBalanceCase {
    const char* name;
    uint32_t value;
};

std::ostream& operator<<(std::ostream& os, const WhiteBalanceCase& value) {
    return os << value.name << " (value=" << value.value << ")";
}

}  // namespace

class CameraEoWhiteBalanceTest : public cet::CameraEoTest {
protected:
    void SetUp() override {
        CameraEoTest::SetUp();
        if (::testing::Test::IsSkipped() || ::testing::Test::HasFatalFailure()) return;
#if defined(VIO) || defined(ZIO) || defined(ORUSL)
        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE,
                                          originalMode_, 3000));
        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_EO_R_GAIN,
                                          originalRed_, 3000));
        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_EO_B_GAIN,
                                          originalBlue_, 3000));
        haveSettings_ = true;
#else
        GTEST_SKIP() << "This payload uses a different EO white-balance catalog.";
#endif
    }

    void TearDown() override {
#if defined(VIO) || defined(ZIO) || defined(ORUSL)
        if (haveSettings_) {
            EXPECT_TRUE(cet::setUint32Param(
                PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE,
                PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE_MANUAL));
            EXPECT_TRUE(cet::setUint32Param(
                PAYLOAD_CAMERA_EO_R_GAIN,
                static_cast<uint32_t>(originalRed_)));
            EXPECT_TRUE(cet::setUint32Param(
                PAYLOAD_CAMERA_EO_B_GAIN,
                static_cast<uint32_t>(originalBlue_)));
            EXPECT_TRUE(cet::setUint32Param(
                PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE,
                static_cast<uint32_t>(originalMode_)));
        }
#endif
        CameraEoTest::TearDown();
    }

    double originalMode_ = 0;
    double originalRed_ = 0;
    double originalBlue_ = 0;
    bool haveSettings_ = false;
};

#if defined(VIO) || defined(ZIO) || defined(ORUSL)
class CameraEoWhiteBalanceModeTest
    : public CameraEoWhiteBalanceTest,
      public ::testing::WithParamInterface<WhiteBalanceCase> {};

TEST_P(CameraEoWhiteBalanceModeTest, SetAndReadBack) {
    const WhiteBalanceCase mode = GetParam();
    ASSERT_TRUE(cet::setUint32Param(PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE,
                                    mode.value));
    double actual = -1;
    ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE,
                                      actual, 3000));
    EXPECT_EQ(actual, mode.value);
}

INSTANTIATE_TEST_SUITE_P(
    EoModes,
    CameraEoWhiteBalanceModeTest,
    ::testing::Values(
        WhiteBalanceCase{"Auto", PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE_AUTO},
        WhiteBalanceCase{"Outdoor", PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE_OUTDOOR},
        WhiteBalanceCase{"Indoor", PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE_INDOOR},
        WhiteBalanceCase{"Manual", PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE_MANUAL},
        WhiteBalanceCase{"ATW", PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE_ATW}),
    [](const ::testing::TestParamInfo<WhiteBalanceCase>& info) {
        return info.param.name;
    });

TEST_F(CameraEoWhiteBalanceTest, Manual_RedBlueGainsReadBack) {
    ASSERT_TRUE(cet::setUint32Param(PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE,
                                    PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE_MANUAL));
    ASSERT_TRUE(cet::setUint32Param(PAYLOAD_CAMERA_EO_R_GAIN, 64));
    ASSERT_TRUE(cet::setUint32Param(PAYLOAD_CAMERA_EO_B_GAIN, 192));
    double red = -1;
    double blue = -1;
    ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_EO_R_GAIN, red, 3000));
    ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_EO_B_GAIN, blue, 3000));
    EXPECT_EQ(red, 64);
    EXPECT_EQ(blue, 192);
}

TEST_F(CameraEoWhiteBalanceTest, OnePushTrigger_AcceptedOrStreamResponsive) {
    ASSERT_TRUE(cet::setUint32Param(
        PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE,
        PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE_ONE_PUSH));
    EXPECT_TRUE(cet::commandAcceptedOrEoResponsive(
        MAV_CMD_USER_4,
        [] { g_payload->setPayloadCameraWBOnePushTrigg(); }));
}
#endif
