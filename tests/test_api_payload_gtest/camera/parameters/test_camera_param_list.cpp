/** @file test_camera_param_list.cpp
 *  @brief Tests getPayloadCameraSettingList(). */

#include "camera_param_test_helpers.h"

#include <cmath>
#include <set>

class CameraParamListTest : public PayloadTest {};

TEST_F(CameraParamListTest, Request_ReturnsParameters) {
    const auto params = config_param::collectCameraParamList();
    ASSERT_FALSE(params.empty())
        << "No PAYLOAD_CAM_PARAMS responses to the list request.";
    std::cout << "[INFO] Received " << params.size()
              << " unique camera parameters." << std::endl;
}

TEST_F(CameraParamListTest, Ids_AreNonemptyUniqueAndBounded) {
    const auto params = config_param::collectCameraParamList();
    ASSERT_FALSE(params.empty());
    std::set<std::string> ids;
    for (const auto& param : params) {
        EXPECT_FALSE(param.id.empty());
        EXPECT_LE(param.id.size(), static_cast<std::size_t>(CAM_PARAM_ID_LEN));
        EXPECT_TRUE(ids.insert(param.id).second)
            << "Duplicate parameter ID " << param.id << ".";
    }
}

TEST_F(CameraParamListTest, Indices_AreUnique) {
    const auto params = config_param::collectCameraParamList();
    ASSERT_FALSE(params.empty());
    std::set<uint16_t> indices;
    for (const auto& param : params) {
        EXPECT_TRUE(indices.insert(param.index).second)
            << "Two IDs reported parameter index " << param.index << ".";
    }
}

TEST_F(CameraParamListTest, Values_AreFinite) {
    const auto params = config_param::collectCameraParamList();
    ASSERT_FALSE(params.empty());
    for (const auto& param : params) {
        EXPECT_TRUE(std::isfinite(param.value))
            << "Non-finite value for " << param.id << ".";
    }
}

TEST_F(CameraParamListTest, RepeatedRequest_ReturnsStableIdsAndIndices) {
    const auto first = config_param::collectCameraParamList();
    const auto second = config_param::collectCameraParamList();
    ASSERT_FALSE(first.empty());
    ASSERT_EQ(second.size(), first.size())
        << "Parameter count changed between consecutive list requests.";

    for (std::size_t i = 0; i < first.size(); ++i) {
        EXPECT_EQ(second[i].index, first[i].index);
        EXPECT_EQ(second[i].id, first[i].id);
    }
}
