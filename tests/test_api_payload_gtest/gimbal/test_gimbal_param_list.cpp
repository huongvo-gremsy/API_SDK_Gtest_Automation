/** @file test_gimbal_param_list.cpp */
#include "gimbal_test_helpers.h"

#include <iostream>
#include <set>

namespace gt = gimbal_test;
class GimbalParamListTest : public PayloadTest {};

TEST_F(GimbalParamListTest, List_ReturnsNonemptyUniqueCatalog) {
    const auto values = gt::getParamList();
    ASSERT_FALSE(values.empty()) << "No gimbal PARAM_VALUE list response.";
    std::set<std::string> ids;
    std::set<uint16_t> indexes;
    for (const auto& value : values) {
        EXPECT_FALSE(value.id.empty());
        EXPECT_TRUE(ids.insert(value.id).second) << "Duplicate ID " << value.id;
        EXPECT_TRUE(indexes.insert(value.index).second)
            << "Duplicate index " << value.index;
    }
    std::cout << "[INFO] Collected " << values.size()
              << " unique gimbal parameters." << std::endl;
}

TEST_F(GimbalParamListTest, RepeatedList_HasStableIdSet) {
    const auto first = gt::getParamList();
    const auto second = gt::getParamList();
    ASSERT_FALSE(first.empty());
    ASSERT_FALSE(second.empty());
    std::set<std::string> a, b;
    for (const auto& item : first) a.insert(item.id);
    for (const auto& item : second) b.insert(item.id);
    EXPECT_EQ(a, b);
}
