/** @file test_camera_param_by_id.cpp
 *  @brief Tests getPayloadCameraSettingByID(). */

#include "camera_param_test_helpers.h"

#include <algorithm>

class CameraParamByIdTest : public PayloadTest {};

TEST_F(CameraParamByIdTest, KnownId_ReturnsValue) {
    double value = 0;
    ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIEW_SRC, value, 3000))
        << "No response for known ID " << PAYLOAD_CAMERA_VIEW_SRC << ".";
}

TEST_F(CameraParamByIdTest, ResponseIsRoutedToExactRequestedId) {
    double viewSource = 0;
    double rcMode = 0;
    ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIEW_SRC, viewSource, 3000));
    ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_RC_MODE, rcMode, 3000));

    std::lock_guard<std::mutex> lock(g_cb.m);
    EXPECT_TRUE(g_cb.paramValueById.count(PAYLOAD_CAMERA_VIEW_SRC));
    EXPECT_TRUE(g_cb.paramValueById.count(PAYLOAD_CAMERA_RC_MODE));
    EXPECT_EQ(g_cb.paramValueById.at(PAYLOAD_CAMERA_VIEW_SRC), viewSource);
    EXPECT_EQ(g_cb.paramValueById.at(PAYLOAD_CAMERA_RC_MODE), rcMode);
}

TEST_F(CameraParamByIdTest, Value_MatchesListEntry) {
    const auto params = config_param::collectCameraParamList();
    ASSERT_FALSE(params.empty());
    const auto expected = std::find_if(
        params.begin(), params.end(), [](const CameraParamSample& sample) {
            return sample.id == PAYLOAD_CAMERA_VIEW_SRC;
        });
    ASSERT_NE(expected, params.end())
        << PAYLOAD_CAMERA_VIEW_SRC << " was absent from the parameter list.";

    double actual = 0;
    ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIEW_SRC, actual, 3000));
    EXPECT_EQ(actual, expected->value);
}

TEST_F(CameraParamByIdTest, GetAllCameraParamsById_AndVerifyValues) {
    const auto params = config_param::collectCameraParamList();
    ASSERT_FALSE(params.empty())
        << "No camera parameters were returned by the list request.";

    std::cout << "[INFO] Verifying " << params.size()
              << " camera parameters using getPayloadCameraSettingByID()."
              << std::endl;

    for (const auto& expected : params) {
        CameraParamSample actual;
        const bool received = config_param::readCameraParamById(
            expected.id.c_str(), actual, 3000);

        EXPECT_TRUE(received)
            << "No response for parameter ID " << expected.id << ".";
        if (!received) continue;

        std::cout << "[RAW PARAM] requestedId=" << expected.id
                  << " listIndex=" << expected.index
                  << " returnedId=" << actual.id
                  << " returnedIndex=" << actual.index
                  << " listValue=" << expected.value
                  << " returnedValue=" << actual.value
                  << std::endl;

        EXPECT_EQ(actual.id, expected.id)
            << "Response was not routed to the requested parameter ID.";
        EXPECT_EQ(actual.index, expected.index)
            << "Parameter " << expected.id
            << " has a different index when read directly by ID.";
        EXPECT_EQ(actual.value, expected.value)
            << "Parameter " << expected.id
            << " changed or returned a different value when read by ID.";
    }
}

TEST_F(CameraParamByIdTest, RepeatedRead_IsStable) {
    double first = 0;
    double second = 0;
    ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIEW_SRC, first, 3000));
    ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_VIEW_SRC, second, 3000));
    EXPECT_EQ(second, first);
}

TEST_F(CameraParamByIdTest, UnknownId_DoesNotProduceFalseMatchingResponse) {
    constexpr char kUnknown[] = "NO_SUCH_PARAM";
    double value = 0;
    EXPECT_FALSE(getCameraSettingByID(kUnknown, value, 1200))
        << "Payload unexpectedly created/responded to unknown ID " << kUnknown << ".";
}

TEST_F(CameraParamByIdTest, SixteenCharacterUnknownId_IsHandledSafely) {
    constexpr char kMaxSafeUnknown[] = "UNKNOWN_PARAM_15";
    static_assert(sizeof(kMaxSafeUnknown) - 1 == 16,
                  "This ID intentionally fills the MAVLink field.");
    double value = 0;
    EXPECT_FALSE(getCameraSettingByID(kMaxSafeUnknown, value, 1200));
}
