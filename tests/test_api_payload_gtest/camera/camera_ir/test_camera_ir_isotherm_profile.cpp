/**
 * @file test_camera_ir_isotherm_profile.cpp
 * @brief Tests the VIO F1 human/fire isotherm profiles from the SDK example.
 */

#include "camera_ir_test_helpers.h"

#include <map>
#include <string>
#include <vector>

namespace cit = camera_ir_test;

// #if defined(VIO)
namespace {
constexpr const char* kIrGain = "C_T_G";
constexpr const char* kIsoUnit = "C_T_ISO_U";
constexpr const char* kIsoMode = "C_T_ISO_M";
constexpr const char* kRegionModes[] = {
    "C_T_RG0_CLR_M", "C_T_RG1_CLR_M", "C_T_RG2_CLR_M",
    "C_T_RG3_CLR_M", "C_T_RG4_CLR_M", "C_T_RG5_CLR_M"};
constexpr const char* kRegion0Temp = "C_T_RG0_TMP";
constexpr const char* kRegion1Temp = "C_T_RG1_TMP";
constexpr const char* kRegion1ColorLow = "C_T_RG1_CLR_MN";
constexpr const char* kRegion1ColorHigh = "C_T_RG1_CLR_MX";

bool applyProfile(uint32_t lowTemp, uint32_t highTemp,
                  uint32_t highColor) {
    if (!cit::setUint32Param(kIsoMode, 1)) return false;
    for (const char* id : kRegionModes) {
        if (!cit::setUint32Param(id, 0)) return false;
    }
    return cit::setUint32Param(kIrGain, 0) &&
           cit::setUint32Param(kIsoUnit, 1) &&
           cit::setUint32Param(kRegion0Temp, lowTemp) &&
           cit::setUint32Param(kRegion1Temp, highTemp) &&
           cit::setUint32Param(kRegionModes[1], 2) &&
           cit::setUint32Param(kRegion1ColorLow, 0) &&
           cit::setUint32Param(kRegion1ColorHigh, highColor);
}
}  // namespace

class CameraIrIsothermProfileTest : public cit::CameraIrTest {
protected:
    void SetUp() override {
        CameraIrTest::SetUp();
        if (::testing::Test::IsSkipped()) return;
        const char* ids[] = {
            kIrGain, kIsoUnit, kIsoMode,
            kRegionModes[0], kRegionModes[1], kRegionModes[2],
            kRegionModes[3], kRegionModes[4], kRegionModes[5],
            kRegion0Temp, kRegion1Temp,
            kRegion1ColorLow, kRegion1ColorHigh};
        for (const char* id : ids) {
            double value = 0;
            if (!cit::getCameraSettingByID(id, value, 2500)) {
                GTEST_SKIP() << "Isotherm parameter " << id
                             << " is unavailable; requires VIO F1 firmware v3.0.3+.";
            }
            originals_[id] = value;
        }
    }

    void TearDown() override {
        for (const auto& original : originals_) {
            EXPECT_TRUE(cit::setUint32Param(
                original.first.c_str(), static_cast<uint32_t>(original.second)))
                << "Could not restore isotherm parameter " << original.first;
        }
        CameraIrTest::TearDown();
    }

    std::map<std::string, double> originals_;
};

TEST_F(CameraIrIsothermProfileTest, HumanProfile_35To40C_ReadsBack) {
    ASSERT_TRUE(applyProfile(35, 40, 9));
    double low = 0, high = 0, mode = 0;
    ASSERT_TRUE(cit::getCameraSettingByID(kRegion0Temp, low));
    ASSERT_TRUE(cit::getCameraSettingByID(kRegion1Temp, high));
    ASSERT_TRUE(cit::getCameraSettingByID(kRegionModes[1], mode));
    EXPECT_EQ(low, 35);
    EXPECT_EQ(high, 40);
    EXPECT_EQ(mode, 2);
}

TEST_F(CameraIrIsothermProfileTest, FireProfile_100To140C_ReadsBack) {
    ASSERT_TRUE(applyProfile(100, 140, 10));
    double low = 0, high = 0, color = 0;
    ASSERT_TRUE(cit::getCameraSettingByID(kRegion0Temp, low));
    ASSERT_TRUE(cit::getCameraSettingByID(kRegion1Temp, high));
    ASSERT_TRUE(cit::getCameraSettingByID(kRegion1ColorHigh, color));
    EXPECT_EQ(low, 100);
    EXPECT_EQ(high, 140);
    EXPECT_EQ(color, 10);
}
// #else
// TEST(CameraIrIsothermProfileTest, UnsupportedProduct) {
//     GTEST_SKIP() << "The isotherm profile example is supported only on VIO F1.";
// }
// #endif
