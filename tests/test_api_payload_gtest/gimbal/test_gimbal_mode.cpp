/** @file test_gimbal_mode.cpp */
#include "gimbal_test_helpers.h"

#include <ostream>

namespace gt = gimbal_test;

namespace {

struct GimbalModeCase {
    const char* name;
    uint32_t value;
};

std::ostream& operator<<(std::ostream& os, const GimbalModeCase& mode) {
    return os << mode.name << " (value=" << mode.value << ")";
}

}  // namespace

class GimbalModeTest : public PayloadTest {
protected:
    void SetUp() override {
        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_GIMBAL_MODE,
                                          originalMode_, 3000));
        haveOriginal_ = true;
    }

    void TearDown() override {
        if (haveOriginal_ && !gt::setControlParamAndVerify(
                PAYLOAD_CAMERA_GIMBAL_MODE,
                static_cast<uint32_t>(originalMode_))) {
            ADD_FAILURE() << "Could not restore original gimbal mode.";
        }
    }

    double originalMode_ = 0;
    bool haveOriginal_ = false;
};

class GimbalModeValueTest
    : public GimbalModeTest,
      public ::testing::WithParamInterface<GimbalModeCase> {};

TEST_P(GimbalModeValueTest, SetAndReadBack) {
    const GimbalModeCase mode = GetParam();
    ASSERT_TRUE(gt::setControlParamAndVerify(PAYLOAD_CAMERA_GIMBAL_MODE,
                                             mode.value));
    double actual = -1;
    ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_GIMBAL_MODE,
                                      actual, 3000));
    EXPECT_EQ(actual, mode.value);
}

INSTANTIATE_TEST_SUITE_P(
    SafeModes,
    GimbalModeValueTest,
    ::testing::Values(
        GimbalModeCase{"Lock", PAYLOAD_CAMERA_GIMBAL_MODE_LOCK},
        GimbalModeCase{"Follow", PAYLOAD_CAMERA_GIMBAL_MODE_FOLLOW},
        GimbalModeCase{"Mapping", PAYLOAD_CAMERA_GIMBAL_MODE_MAPPING}),
    [](const ::testing::TestParamInfo<GimbalModeCase>& info) {
        return info.param.name;
    });

TEST_F(GimbalModeTest, ExampleFlow_LockFollowMapping_ReadBack) {
    const uint32_t modes[] = {
        PAYLOAD_CAMERA_GIMBAL_MODE_LOCK,
        PAYLOAD_CAMERA_GIMBAL_MODE_FOLLOW,
        PAYLOAD_CAMERA_GIMBAL_MODE_MAPPING,
        PAYLOAD_CAMERA_GIMBAL_MODE_RESET
    };
    for (const uint32_t mode : modes) {
        ASSERT_TRUE(gt::setControlParamAndVerify(
            PAYLOAD_CAMERA_GIMBAL_MODE, mode));
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

TEST_F(GimbalModeTest, DISABLED_ExampleFlow_OffThenReset_ReadBack) {
    // OFF removes stabilization and RESET can cause large physical movement;
    // keep this opt-in for a cleared bench with an operator present.
    ASSERT_TRUE(gt::setControlParamAndVerify(
        PAYLOAD_CAMERA_GIMBAL_MODE, PAYLOAD_CAMERA_GIMBAL_MODE_OFF, 8000));
    ASSERT_TRUE(gt::setControlParamAndVerify(
        PAYLOAD_CAMERA_GIMBAL_MODE, PAYLOAD_CAMERA_GIMBAL_MODE_RESET, 10000));
    std::this_thread::sleep_for(std::chrono::seconds(100));
}
