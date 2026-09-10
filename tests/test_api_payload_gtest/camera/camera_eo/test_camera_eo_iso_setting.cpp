/**
 * @file test_camera_eo_iso_setting.cpp
 * @brief MB1 EO ISO setting tests for C_G_ISO.
 */

#include "camera_eo_test_helpers.h"

#include <chrono>
#include <cstdint>
#include <ostream>
#include <thread>

namespace cet = camera_eo_test;

#if defined(MB1)

namespace {

struct IsoCase {
    const char* name;
    uint32_t value;
};

std::ostream& operator<<(std::ostream& os, const IsoCase& iso) {
    return os << iso.name << " (value=" << iso.value << ")";
}

bool setIsoAndReadBack(uint32_t value, double& actual,
                       int timeoutMs = 5000) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        char paramId[] = PAYLOAD_CAMERA_EO_ISO;
        g_payload->setPayloadCameraParam(paramId, value, PARAM_TYPE_UINT32);
        if (cet::getCameraSettingByID(PAYLOAD_CAMERA_EO_ISO, actual, 1000) &&
            static_cast<uint32_t>(actual) == value) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    return false;
}

}  // namespace

class CameraEoIsoSettingTest : public PayloadTest {
protected:
    void SetUp() override {
        ASSERT_TRUE(cet::getCameraSettingByID(PAYLOAD_CAMERA_EO_ISO,
                                              originalIso_, 3000))
            << "Could not read the original EO ISO setting.";
        haveOriginalIso_ = true;
    }

    void TearDown() override {
        if (haveOriginalIso_) {
            double restored = -1;
            EXPECT_TRUE(setIsoAndReadBack(static_cast<uint32_t>(originalIso_),
                                          restored, 5000))
                << "Could not restore the original EO ISO setting.";
            EXPECT_EQ(static_cast<uint32_t>(restored),
                      static_cast<uint32_t>(originalIso_));
        }
    }

    double originalIso_ = 0;
    bool haveOriginalIso_ = false;
};

class CameraEoIsoSettingValueTest
    : public CameraEoIsoSettingTest,
      public ::testing::WithParamInterface<IsoCase> {};

TEST_P(CameraEoIsoSettingValueTest, SetAndReadBack) {
    const IsoCase iso = GetParam();
    double actual = -1;

    ASSERT_TRUE(setIsoAndReadBack(iso.value, actual))
        << "No readback after setting EO ISO to " << iso;
    EXPECT_EQ(static_cast<uint32_t>(actual), iso.value);
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues,
    CameraEoIsoSettingValueTest,
    ::testing::Values(
        IsoCase{"Auto", PAYLOAD_CAMERA_EO_ISO_AUTO},
        IsoCase{"Deblur", PAYLOAD_CAMERA_EO_ISO_DEBLUR},
        IsoCase{"ISO100", PAYLOAD_CAMERA_EO_ISO_100},
        IsoCase{"ISO200", PAYLOAD_CAMERA_EO_ISO_200},
        IsoCase{"ISO400", PAYLOAD_CAMERA_EO_ISO_400},
        IsoCase{"ISO800", PAYLOAD_CAMERA_EO_ISO_800},
        IsoCase{"ISO1600", PAYLOAD_CAMERA_EO_ISO_1600},
        IsoCase{"ISO3200", PAYLOAD_CAMERA_EO_ISO_3200}),
    [](const ::testing::TestParamInfo<IsoCase>& info) {
        return info.param.name;
    });

#else

TEST(CameraEoIsoSettingTest, UnsupportedProduct) {
    GTEST_SKIP() << "EO ISO parameter C_G_ISO is only defined for MB1.";
}

#endif
