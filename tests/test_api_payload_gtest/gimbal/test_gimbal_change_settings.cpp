/** @file test_gimbal_change_settings.cpp */
#include "gimbal_test_helpers.h"

namespace gt = gimbal_test;

class GimbalChangeSettingsExampleTest : public PayloadTest {
protected:
    void SetUp() override {
#if defined(VIO) || defined(ORUSL)
        ASSERT_TRUE(gt::getParamById("STIFF_TILT", original_));
        haveOriginal_ = true;
#else
        GTEST_SKIP() << "The example supports STIFF_TILT on VIO/OrusL.";
#endif
    }

    void TearDown() override {
        if (haveOriginal_ &&
            !gt::setParamAndVerify("STIFF_TILT", original_.value)) {
            ADD_FAILURE() << "Could not restore STIFF_TILT=" << original_.value;
        }
    }

    gt::GimbalParamSample original_;
    bool haveOriginal_ = false;
};

TEST_F(GimbalChangeSettingsExampleTest, ExampleFlow_StiffTilt50ReadBackRestore) {
    ASSERT_TRUE(gt::setParamAndVerify("STIFF_TILT", 50));
    gt::GimbalParamSample changed;
    ASSERT_TRUE(gt::getParamById("STIFF_TILT", changed));
    EXPECT_NEAR(changed.value, 50.0, 0.01);

    ASSERT_TRUE(gt::setParamAndVerify("STIFF_TILT", original_.value));
    gt::GimbalParamSample restored;
    ASSERT_TRUE(gt::getParamById("STIFF_TILT", restored));
    EXPECT_NEAR(restored.value, original_.value, 0.01);
}

TEST_F(GimbalChangeSettingsExampleTest, SameStiffnessValue_IsIdempotent) {
    EXPECT_TRUE(gt::setParamAndVerify("STIFF_TILT", original_.value));
    EXPECT_TRUE(gt::setParamAndVerify("STIFF_TILT", original_.value));
}
