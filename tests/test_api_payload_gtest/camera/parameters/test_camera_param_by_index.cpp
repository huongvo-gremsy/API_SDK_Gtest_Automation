/** @file test_camera_param_by_index.cpp
 *  @brief Tests getPayloadCameraSettingByIndex(). */

#include "camera_param_test_helpers.h"

#include <array>

class CameraParamByIndexTest : public PayloadTest {};

TEST_F(CameraParamByIndexTest, FirstMiddleLast_ReturnExpectedEntries) {
    const auto params = config_param::collectCameraParamList();
    ASSERT_FALSE(params.empty());
    const std::array<std::size_t, 3> positions = {
        0, params.size() / 2, params.size() - 1};

    for (const std::size_t position : positions) {
        const auto& expected = params[position];
        ASSERT_LE(expected.index, 255u);
        CameraParamSample actual;
        ASSERT_TRUE(config_param::readCameraParamByIndex(
            static_cast<uint8_t>(expected.index), actual, 3000))
            << "No response for index " << expected.index << ".";
        EXPECT_EQ(actual.index, expected.index);
        EXPECT_EQ(actual.id, expected.id);
        EXPECT_EQ(actual.value, expected.value);
    }
}

TEST_F(CameraParamByIndexTest, ReturnedIndex_MatchesRequest) {
    const auto params = config_param::collectCameraParamList();
    ASSERT_FALSE(params.empty());
    const auto& expected = params.front();
    ASSERT_LE(expected.index, 255u);
    CameraParamSample actual;
    ASSERT_TRUE(config_param::readCameraParamByIndex(
        static_cast<uint8_t>(expected.index), actual, 3000));
    EXPECT_EQ(actual.index, expected.index);
}

TEST_F(CameraParamByIndexTest, EveryListedIndex_RoundTripsToSameId) {
    const auto params = config_param::collectCameraParamList();
    ASSERT_FALSE(params.empty());
    for (const auto& expected : params) {
        ASSERT_LE(expected.index, 255u);
        CameraParamSample actual;
        ASSERT_TRUE(config_param::readCameraParamByIndex(
            static_cast<uint8_t>(expected.index), actual, 3000))
            << "No response for listed index " << expected.index << ".";
        EXPECT_EQ(actual.id, expected.id)
            << "Index " << expected.index << " returned the wrong ID.";
    }
}

TEST_F(CameraParamByIndexTest, InvalidIndex_DoesNotProduceMatchingResponse) {
    const auto params = config_param::collectCameraParamList();
    ASSERT_FALSE(params.empty());
    uint16_t maximum = 0;
    for (const auto& param : params) maximum = std::max(maximum, param.index);
    if (maximum >= 255) GTEST_SKIP() << "No unused uint8 index is available.";

    const uint8_t invalid = static_cast<uint8_t>(maximum + 1);
    CameraParamSample result;
    EXPECT_FALSE(config_param::readCameraParamByIndex(invalid, result, 1200))
        << "Payload responded to out-of-list index "
        << static_cast<int>(invalid) << ".";
}
