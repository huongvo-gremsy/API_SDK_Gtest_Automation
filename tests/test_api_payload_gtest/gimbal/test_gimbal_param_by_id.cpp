/** @file test_gimbal_param_by_id.cpp */
#include "gimbal_test_helpers.h"

namespace gt = gimbal_test;
class GimbalParamByIdTest : public PayloadTest {};

TEST_F(GimbalParamByIdTest, KnownVersionId_ReturnsExactId) {
    gt::GimbalParamSample value;
    ASSERT_TRUE(gt::getParamById("VERSION_X", value));
    EXPECT_EQ(value.id, "VERSION_X");
    EXPECT_TRUE(std::isfinite(value.value));
}

TEST_F(GimbalParamByIdTest, KnownStiffnessId_IsRepeatable) {
    gt::GimbalParamSample first, second;
    ASSERT_TRUE(gt::getParamById("STIFF_TILT", first));
    ASSERT_TRUE(gt::getParamById("STIFF_TILT", second));
    EXPECT_EQ(first.index, second.index);
    EXPECT_DOUBLE_EQ(first.value, second.value);
}

TEST_F(GimbalParamByIdTest, UnknownId_DoesNotProduceFalseMatchingResponse) {
    gt::GimbalParamSample value;
    EXPECT_FALSE(gt::getParamById("NO_SUCH_GB_PARAM", value, 1200));
}
