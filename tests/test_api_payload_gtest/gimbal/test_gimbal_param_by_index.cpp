/** @file test_gimbal_param_by_index.cpp */
#include "gimbal_test_helpers.h"

namespace gt = gimbal_test;
class GimbalParamByIndexTest : public PayloadTest {};

TEST_F(GimbalParamByIndexTest, FirstCatalogIndex_ReturnsSameEntry) {
    const auto catalog = gt::getParamList();
    ASSERT_FALSE(catalog.empty());
    gt::GimbalParamSample actual;
    ASSERT_TRUE(gt::getParamByIndex(static_cast<uint8_t>(catalog.front().index),
                                    actual));
    EXPECT_EQ(actual.index, catalog.front().index);
    EXPECT_EQ(actual.id, catalog.front().id);
    EXPECT_DOUBLE_EQ(actual.value, catalog.front().value);
}

TEST_F(GimbalParamByIndexTest, MiddleCatalogIndex_MatchesIdQuery) {
    const auto catalog = gt::getParamList();
    ASSERT_FALSE(catalog.empty());
    const auto expected = catalog[catalog.size() / 2];
    ASSERT_LE(expected.index, 255u);
    gt::GimbalParamSample byIndex, byId;
    ASSERT_TRUE(gt::getParamByIndex(static_cast<uint8_t>(expected.index), byIndex));
    ASSERT_TRUE(gt::getParamById(byIndex.id, byId));
    EXPECT_EQ(byIndex.index, byId.index);
    EXPECT_DOUBLE_EQ(byIndex.value, byId.value);
}
