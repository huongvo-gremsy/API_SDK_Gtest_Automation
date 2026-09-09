/** @file test_gimbal_param_set.cpp */
#include "gimbal_test_helpers.h"

namespace gt = gimbal_test;

class GimbalParamSetTest : public PayloadTest {
protected:
    void SetUp() override {
        ASSERT_TRUE(gt::getParamById("STIFF_TILT", original_));
        haveOriginal_ = true;
    }
    void TearDown() override {
        if (haveOriginal_) {
            EXPECT_TRUE(gt::setParamAndVerify(original_.id, original_.value))
                << "Could not restore STIFF_TILT.";
        }
    }
    gt::GimbalParamSample original_;
    bool haveOriginal_ = false;
};

TEST_F(GimbalParamSetTest, StiffTilt_SetReadBackAndRestore) {
    const double target = original_.value >= 99.0
        ? original_.value - 1.0 : original_.value + 1.0;
    ASSERT_TRUE(gt::setParamAndVerify(original_.id, target));
    gt::GimbalParamSample actual;
    ASSERT_TRUE(gt::getParamById(original_.id, actual));
    EXPECT_NEAR(actual.value, target, 0.01);
}

TEST_F(GimbalParamSetTest, SameValue_IsIdempotent) {
    EXPECT_TRUE(gt::setParamAndVerify(original_.id, original_.value));
    EXPECT_TRUE(gt::setParamAndVerify(original_.id, original_.value));
}
