#include "api_camera_eo_test_helper.h"

#include <cstdint>
#include <iomanip>
#include <ostream>
#include <set>
#include <thread>

namespace cet = camera_eo_test;

// ---------------------------------------------------------------------------
// Shared param-case type used by every "documented value" sweep below.
// ---------------------------------------------------------------------------
struct CameraEoParamValue {
    const char* name;
    uint32_t value;
};

inline std::ostream& operator<<(std::ostream& os,
                                 const CameraEoParamValue& value) {
    return os << value.name << "(" << value.value << ")";
}

// ---------------------------------------------------------------------------
// Lightweight fixture for parameter tests. CameraEoTest is intentionally not
// used here because its setup/teardown polls capture status while stopping
// active image/video operations.
// ---------------------------------------------------------------------------
class CameraEoParamSettingTest : public PayloadTest {
    protected:
    void SetUp() override {
        ASSERT_TRUE(cet::readCameraParam(PAYLOAD_CAMERA_VIEW_SRC, originalView_))
            << "Could not read the current camera view source before the test.";
        haveView_ = true;
 
        ASSERT_TRUE(cet::setCameraParam(PAYLOAD_CAMERA_VIEW_SRC,
                                         PAYLOAD_CAMERA_VIEW_EO))
            << "Could not switch the camera view to EO before the test.";
    }
 
    void TearDown() override {
        if (haveView_) {
            EXPECT_TRUE(cet::setCameraParam(
                PAYLOAD_CAMERA_VIEW_SRC, static_cast<uint32_t>(originalView_)))
                << "Could not restore the camera view source after the test.";
        }
    }
 
    double originalView_ = 0;
    bool haveView_ = false;
};
class ManualCameraEoParamSettingTest : public testing::Test {};

// ---------------------------------------------------------------------------
// Shared verification helper.
//
// NOTE: this only performs the set+readback+compare. The "payload does not
// report this parameter at all" skip check stays inside each TEST_P body,
// because GTEST_SKIP() only skips the *current* test when invoked directly
// in the test body — calling it from inside a plain helper function does
// not propagate the skip up to the caller.
// ---------------------------------------------------------------------------
namespace {

bool SetParamAndReadBack(const char* paramId, uint32_t value, double& actual) {
    if (!cet::setCameraParam(paramId, value, 4000)) {
        return false;
    }

    if (!cet::readCameraParam(paramId, actual, 3000)) {
        return false;
    }

    std::cout << "--> Param_id: " << paramId
              << ", value: " << std::fixed << std::setprecision(2)
              << actual << std::endl;
    return true;
}

}  // namespace

// =============================================================================
// 0. ADRC
// =============================================================================

// =============================================================================
// 1. EO Exposure Compensation
// =============================================================================
// Exposure Compensation	C_G_AE_COMP	-	-	x	-
#if defined(PAYLOAD_CAMERA_EO_AE_COMPENSATION) && defined(MB1)

using ExposureCompensationCase = CameraEoParamValue;

class CameraEoExposureCompensationSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<ExposureCompensationCase> {};

TEST_P(CameraEoExposureCompensationSettingValueTest, SetAndReadBack) {
    const ExposureCompensationCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_AE_COMPENSATION, original, 3000)) {
        GTEST_SKIP() << "Payload did not report EO exposure compensation ("
                     << PAYLOAD_CAMERA_EO_AE_COMPENSATION << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_AE_COMPENSATION,
                                     param.value, actual))
        << "No readback after setting EO exposure compensation to " << param
        << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO exposure compensation readback mismatch for " << param << ".";

    // Restore original value
    std::cout << "Restoring EO exposure compensation to original value "
              << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_AE_COMPENSATION,
                                     static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO exposure compensation to "
        << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoExposureCompensationSettingValueTest,
    ::testing::Values(ExposureCompensationCase{"Minus12", 0},
                       ExposureCompensationCase{"Minus6", 6},
                       ExposureCompensationCase{"Zero", 12},
                       ExposureCompensationCase{"Plus6", 18},
                       ExposureCompensationCase{"Plus12", 24}),
    [](const ::testing::TestParamInfo<ExposureCompensationCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_AE_COMPENSATION

// =============================================================================
// 2. Video Auto Exposure
// =============================================================================
// MB1 C_G_AE_MODE, VIO C_V_AE, ZIO C_V_AE
// Video Auto Exposure	C_G_AE_MODE	-	-	?	-
// #if defined(PAYLOAD_CAMERA_VIDEO_AUTO_EXPOSURE)

// using AutoExposureCase = CameraEoParamValue;

// class CameraEoVideoAutoExposureSettingValueTest
//     : public CameraEoParamSettingTest,
//       public ::testing::WithParamInterface<AutoExposureCase> {};

// TEST_P(CameraEoVideoAutoExposureSettingValueTest, SetAndReadBack) {
//     const AutoExposureCase param = GetParam();
//     double original = -1;
//     if (!cet::readCameraParam(PAYLOAD_CAMERA_VIDEO_AUTO_EXPOSURE, original, 3000)) {
//         GTEST_SKIP() << "Payload did not report video auto exposure ("
//                      << PAYLOAD_CAMERA_VIDEO_AUTO_EXPOSURE << ").";
//     }
//     double actual = -1;
//     ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_VIDEO_AUTO_EXPOSURE,
//                                      param.value, actual))
//         << "No readback after setting video auto exposure to " << param << ".";
//     EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
//         << "Video auto exposure readback mismatch for " << param << ".";
//     // Restore original value
//     std::cout << "Restoring video auto exposure to original value " << original << "." << std::endl;
//     ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_VIDEO_AUTO_EXPOSURE, static_cast<uint32_t>(original), actual))
//         << "No readback after restoring video auto exposure to " << original << ".";
// }

// INSTANTIATE_TEST_SUITE_P(
//     DocumentedValues, CameraEoVideoAutoExposureSettingValueTest,
//     ::testing::Values(AutoExposureCase{"Auto", 0},
//                        AutoExposureCase{"Manual", 3},
//                        AutoExposureCase{"Shutter", 10},
//                        AutoExposureCase{"Iris", 11},
//                        AutoExposureCase{"Bright", 13},
//                        AutoExposureCase{"Gain", 14}),
//     [](const ::testing::TestParamInfo<AutoExposureCase>& info) {
//         return info.param.name;
//     });

// #endif  // PAYLOAD_CAMERA_VIDEO_AUTO_EXPOSURE

// =============================================================================
// 3. Contrast C_G_CONTRA
// =============================================================================
// Contrast	C_G_CONTRA	-	-	x	-
#if defined(PAYLOAD_CAMERA_EO_CONTRAST) && defined(MB1)
using ContrastCase = CameraEoParamValue;

class CameraEoContrastSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<ContrastCase> {};

TEST_P(CameraEoContrastSettingValueTest, SetAndReadBack) {
    const ContrastCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_CONTRAST, original, 3000)) {
        GTEST_SKIP() << "Payload did not report EO contrast ("
                     << PAYLOAD_CAMERA_EO_CONTRAST << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_CONTRAST,
                                        param.value, actual))
        << "No readback after setting EO contrast to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO contrast readback mismatch for " << param << ".";
    std::cout << "Restoring EO contrast to original value " << original
              << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_CONTRAST,
                                        static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO contrast to " << original << ".";
        
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoContrastSettingValueTest,
    ::testing::Values(ContrastCase{"1", 1},
                       ContrastCase{"2", 2},
                       ContrastCase{"3", 3},
                       ContrastCase{"4", 4},
                       ContrastCase{"5", 5},
                       ContrastCase{"6", 6},
                       ContrastCase{"7", 7},
                       ContrastCase{"8", 8},
                       ContrastCase{"9", 9},
                       ContrastCase{"10", 10}),
    [](const ::testing::TestParamInfo<ContrastCase>& info) {
        return info.param.name;
    });
#endif  // PAYLOAD_CAMERA_EO_CONTRAST
// =============================================================================
// 4. EO Control Mode C_G_CTRL_M
// =============================================================================
//Control Mode	C_G_CTRL_M	-	-	x	-
#if defined(PAYLOAD_CAMERA_EO_CONTROL_MODE) && defined(MB1)

using ControlModeCase = CameraEoParamValue;

class CameraEoControlModeSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<ControlModeCase> {};

TEST_P(CameraEoControlModeSettingValueTest, SetAndReadBack) {
    const ControlModeCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_CONTROL_MODE, original, 3000)) {
        GTEST_SKIP() << "Payload did not report EO control mode ("
                     << PAYLOAD_CAMERA_EO_CONTROL_MODE << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_CONTROL_MODE,
                                     param.value, actual))
        << "No readback after setting EO control mode to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO control mode readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO control mode to original value " << original << "."
              << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_CONTROL_MODE,
                                     static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO control mode to " << original << ".";    
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoControlModeSettingValueTest,
    ::testing::Values(
        ControlModeCase{"Off", PAYLOAD_CAMERA_EO_CONTROL_MODE_OFF},
        ControlModeCase{"Auto", PAYLOAD_CAMERA_EO_CONTROL_MODE_AUTO},
        ControlModeCase{"UseScene", PAYLOAD_CAMERA_EO_CONTROL_MODE_USE_SCENE},
        ControlModeCase{"OffKeepState",
                        PAYLOAD_CAMERA_EO_CONTROL_MODE_OFF_KEEP_STATE}),
    [](const ::testing::TestParamInfo<ControlModeCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_CONTROL_MODE

// =============================================================================
// 5. EO Exposure Lock C_G_EXPO_L
// =============================================================================
// Exposure Lock	C_G_EXPO_L	-	-	x	-
#if defined(PAYLOAD_CAMERA_EO_EXPOSURE_LOCK) && defined(MB1)

using ExposureLockCase = CameraEoParamValue;

class CameraEoExposureLockSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<ExposureLockCase> {};

TEST_P(CameraEoExposureLockSettingValueTest, SetAndReadBack) {
    const ExposureLockCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_EXPOSURE_LOCK, original, 3000)) {
        GTEST_SKIP() << "Payload did not report EO exposure lock ("
                     << PAYLOAD_CAMERA_EO_EXPOSURE_LOCK << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_EXPOSURE_LOCK,
                                     param.value, actual))
        << "No readback after setting EO exposure lock to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO exposure lock readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO exposure lock to original value " << original << "."
              << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_EXPOSURE_LOCK,
                                     static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO exposure lock to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoExposureLockSettingValueTest,
    ::testing::Values(
        ExposureLockCase{"Off", PAYLOAD_CAMERA_EO_EXPOSURE_LOCK_OFF},
        ExposureLockCase{"On", PAYLOAD_CAMERA_EO_EXPOSURE_LOCK_ON}),
    [](const ::testing::TestParamInfo<ExposureLockCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_EXPOSURE_LOCK

// =============================================================================
// 6. EO Exposure Mode C_G_EXPO_M
// =============================================================================
// Exposure Mode	C_G_EXPO_M	-	-	x	-
#if defined(PAYLOAD_CAMERA_EO_EXPOSURE_MODE) && defined(MB1)

using ExposureModeCase = CameraEoParamValue;

class CameraEoExposureModeSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<ExposureModeCase> {};

TEST_P(CameraEoExposureModeSettingValueTest, SetAndReadBack) {
    const ExposureModeCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_EXPOSURE_MODE, original, 3000)) {
        GTEST_SKIP() << "Payload did not report EO exposure mode ("
                     << PAYLOAD_CAMERA_EO_EXPOSURE_MODE << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_EXPOSURE_MODE,
                                     param.value, actual))
        << "No readback after setting EO exposure mode to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO exposure mode readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO exposure mode to original value " << original << "."
              << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_EXPOSURE_MODE,
                                     static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO exposure mode to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoExposureModeSettingValueTest,
    ::testing::Values(
        ExposureModeCase{"Manual", PAYLOAD_CAMERA_EO_EXPOSURE_MODE_MANUAL},
        ExposureModeCase{"Auto", PAYLOAD_CAMERA_EO_EXPOSURE_MODE_AUTO}),
    [](const ::testing::TestParamInfo<ExposureModeCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_EXPOSURE_MODE

// =============================================================================
// 7. EO Exposure Metering C_G_EXPO_ME
// =============================================================================
// Exposure Metering	C_G_EXPO_ME	-	-	x	-
#if defined(PAYLOAD_CAMERA_EO_EXPOSURE_METERING) && defined(MB1)

using ExposureMeteringCase = CameraEoParamValue;

class CameraEoExposureMeteringSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<ExposureMeteringCase> {};

TEST_P(CameraEoExposureMeteringSettingValueTest, SetAndReadBack) {
    const ExposureMeteringCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_EXPOSURE_METERING, original,
                               3000)) {
        GTEST_SKIP() << "Payload did not report EO exposure metering ("
                     << PAYLOAD_CAMERA_EO_EXPOSURE_METERING << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_EXPOSURE_METERING,
                                     param.value, actual))
        << "No readback after setting EO exposure metering to " << param
        << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO exposure metering readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO exposure metering to original value " << original << "."
              << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_EXPOSURE_METERING,
                                     static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO exposure metering to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoExposureMeteringSettingValueTest,
    ::testing::Values(
        ExposureMeteringCase{"Average",
                             PAYLOAD_CAMERA_EO_EXPOSURE_METERING_AVERAGE},
        ExposureMeteringCase{"Center",
                             PAYLOAD_CAMERA_EO_EXPOSURE_METERING_CENTER},
        ExposureMeteringCase{"Spot",
                             PAYLOAD_CAMERA_EO_EXPOSURE_METERING_SPOT}),
    [](const ::testing::TestParamInfo<ExposureMeteringCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_EXPOSURE_METERING

// =============================================================================
// 8. EO Exposure Time C_G_EXPO_TIME
// =============================================================================
// Exposure Time(us)	C_G_EXPO_TIME	-	-	x	-
#if defined(PAYLOAD_CAMERA_EO_EXPOSURE_TIME) && defined(MB1)

using ExposureTimeCase = CameraEoParamValue;

class CameraEoExposureTimeSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<ExposureTimeCase> {};

TEST_P(CameraEoExposureTimeSettingValueTest, SetAndReadBack) {
    const ExposureTimeCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_EXPOSURE_TIME, original, 3000)) {
        GTEST_SKIP() << "Payload did not report EO exposure time ("
                     << PAYLOAD_CAMERA_EO_EXPOSURE_TIME << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_EXPOSURE_TIME,
                                     param.value, actual))
        << "No readback after setting EO exposure time to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO exposure time readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO exposure time to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_EXPOSURE_TIME,
                                     static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO exposure time to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoExposureTimeSettingValueTest,
    ::testing::Values(ExposureTimeCase{"Minimum", 200},
                       ExposureTimeCase{"Typical", 10000},
                       ExposureTimeCase{"Typical_2", 5000},
                       ExposureTimeCase{"Typical_3", 50000},
                       ExposureTimeCase{"Maximum", 100000}),
    [](const ::testing::TestParamInfo<ExposureTimeCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_EXPOSURE_TIME

// =============================================================================
// 9. EO Gain Ls C_G_Gain
// =============================================================================
// Eo Gain Ls	C_G_GAIN	-	-	?	-
#if defined(PAYLOAD_CAMERA_EO_GAIN) && defined(MB1) 

using GainCase = CameraEoParamValue;

class CameraEoGainSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<GainCase> {};

TEST_P(CameraEoGainSettingValueTest, SetAndReadBack) {
    const GainCase gain = GetParam();
    double actual = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_GAIN, actual, 3000)) {
        GTEST_SKIP() << "Payload did not report EO gain ("
                     << PAYLOAD_CAMERA_EO_GAIN << ").";
    }
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_GAIN, gain.value, actual))
        << "No readback after setting EO gain to " << gain << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), gain.value)
        << "EO gain readback mismatch for " << gain << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoGainSettingValueTest,
    ::testing::Values(GainCase{"Minimum", 0},
                       GainCase{"Typical", 1},
                       GainCase{"Maximum", 2}),
    [](const ::testing::TestParamInfo<GainCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_GAIN

// =============================================================================
// 10. EO Video Aperture Value C_G_IRIS
// =============================================================================
// Video Aperture Value	C_G_IRIS	-	-	?	-
#if defined(PAYLOAD_CAMERA_VIDEO_APERTURE_VALUE) && defined(MB1)

using ApertureValueCase = CameraEoParamValue;

class CameraEoVideoApertureValueSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<ApertureValueCase> {};

TEST_P(CameraEoVideoApertureValueSettingValueTest, SetAndReadBack) {
    const ApertureValueCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_VIDEO_APERTURE_VALUE, original, 3000)) {
        GTEST_SKIP() << "Payload did not report EO video aperture value ("
                     << PAYLOAD_CAMERA_VIDEO_APERTURE_VALUE << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_VIDEO_APERTURE_VALUE,
                                     param.value, actual))
        << "No readback after setting EO video aperture value to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO video aperture value readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO video aperture value to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_VIDEO_APERTURE_VALUE,
                                     static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO video aperture value to " << original << ".";    
}
INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoVideoApertureValueSettingValueTest,
    ::testing::Values(ApertureValueCase{"0", 0},
                       ApertureValueCase{"1", 1},
                       ApertureValueCase{"2", 2}),
    [](const ::testing::TestParamInfo<ApertureValueCase>& info) {
        return info.param.name;
    });
#endif  // PAYLOAD_CAMERA_VIDEO_APERTURE_VALUE

// =============================================================================
// 11. EO ISO C_G_ISO
// =============================================================================
// ISO	C_G_ISO	-	-	x	-
#if defined(PAYLOAD_CAMERA_EO_ISO) && defined(MB1)

using IsoCase = CameraEoParamValue;

class CameraEoIsoSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<IsoCase> {};

TEST_P(CameraEoIsoSettingValueTest, SetAndReadBack) {
    const IsoCase iso = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_ISO, original, 3000)) {
        GTEST_SKIP() << "Payload did not report EO ISO ("
                     << PAYLOAD_CAMERA_EO_ISO << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_ISO, iso.value, actual))
        << "No readback after setting EO ISO to " << iso << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), iso.value)
        << "EO ISO readback mismatch for " << iso << ".";
    // Restore original value
    std::cout << "Restoring EO ISO to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_ISO, static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO ISO to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoIsoSettingValueTest,
    ::testing::Values(IsoCase{"Auto", PAYLOAD_CAMERA_EO_ISO_AUTO},
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
    GTEST_SKIP() << "EO ISO parameter C_G_ISO is not defined for this product.";
}

#endif  // PAYLOAD_CAMERA_EO_ISO

// =============================================================================
// 12. EO Manual ISO C_G_ISO_M
// =============================================================================
// Manual ISO	C_G_ISO_M	-	-	x	-
#if defined(PAYLOAD_CAMERA_EO_ISO_MANUAL) && defined(MB1)

using ManualIsoCase = CameraEoParamValue;

class CameraEoManualIsoSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<ManualIsoCase> {};

TEST_P(CameraEoManualIsoSettingValueTest, SetAndReadBack) {
    const ManualIsoCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_ISO_MANUAL, original, 3000)) {
        GTEST_SKIP() << "Payload did not report EO manual ISO ("
                     << PAYLOAD_CAMERA_EO_ISO_MANUAL << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_ISO_MANUAL, param.value,
                                     actual))
        << "No readback after setting EO manual ISO to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO manual ISO readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO manual ISO to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_ISO_MANUAL, static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO manual ISO to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoManualIsoSettingValueTest,
    ::testing::Values(ManualIsoCase{"ISO100", 100},
                       ManualIsoCase{"ISO400", 400},
                       ManualIsoCase{"ISO800", 800},
                       ManualIsoCase{"ISO1600", 1600},
                       ManualIsoCase{"ISO3200", 3200}),
    [](const ::testing::TestParamInfo<ManualIsoCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_ISO_MANUAL

// =============================================================================
// 13. EO Noise Reduction C_G_NR
// =============================================================================
// Noise Reduction	C_G_NR	-	-	x	-
#if defined(PAYLOAD_CAMERA_EO_NOISE_REDUCTION) && defined(MB1)

using NoiseReductionCase = CameraEoParamValue;

class CameraEoNoiseReductionSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<NoiseReductionCase> {};

TEST_P(CameraEoNoiseReductionSettingValueTest, SetAndReadBack) {
    const NoiseReductionCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_NOISE_REDUCTION, original,
                               3000)) {
        GTEST_SKIP() << "Payload did not report EO noise reduction ("
                     << PAYLOAD_CAMERA_EO_NOISE_REDUCTION << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_NOISE_REDUCTION,
                                     param.value, actual))
        << "No readback after setting EO noise reduction to " << param
        << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO noise reduction readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO noise reduction to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_NOISE_REDUCTION, static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO noise reduction to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoNoiseReductionSettingValueTest,
    ::testing::Values(
        NoiseReductionCase{"Off", PAYLOAD_CAMERA_EO_NOISE_REDUCTION_OFF},
        NoiseReductionCase{"Fast", PAYLOAD_CAMERA_EO_NOISE_REDUCTION_FAST},
        NoiseReductionCase{"HighQuality",
                           PAYLOAD_CAMERA_EO_NOISE_REDUCTION_HQ}),
    [](const ::testing::TestParamInfo<NoiseReductionCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_NOISE_REDUCTION

// =============================================================================
// 14. Night Mode FPS C_G_N_MODE_FPS
// =============================================================================
// Night Mode FPS	C_G_N_MODE_FPS	-	-	x	-
#if defined(PAYLOAD_CAMERA_EO_NIGHT_MODE_FPS) && defined(MB1)

using NightModeFpsCase = CameraEoParamValue;

class CameraEoNightModeFpsSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<NightModeFpsCase> {};   

TEST_P(CameraEoNightModeFpsSettingValueTest, SetAndReadBack) {
    const NightModeFpsCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_NIGHT_MODE_FPS, original, 3000)) {
        GTEST_SKIP() << "Payload did not report EO night mode FPS ("
                     << PAYLOAD_CAMERA_EO_NIGHT_MODE_FPS << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_NIGHT_MODE_FPS,
                                     param.value, actual))
        << "No readback after setting EO night mode FPS to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO night mode FPS readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO night mode FPS to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_NIGHT_MODE_FPS, static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO night mode FPS to " << original << ".";
}
INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoNightModeFpsSettingValueTest,
    ::testing::Values(NightModeFpsCase{"Maximum", PAYLOAD_CAMERA_EO_NIGHT_MODE_FPS_30},
                       NightModeFpsCase{"Typical", PAYLOAD_CAMERA_EO_NIGHT_MODE_FPS_20},
                       NightModeFpsCase{"Minimum", PAYLOAD_CAMERA_EO_NIGHT_MODE_FPS_10}),
    [](const ::testing::TestParamInfo<NightModeFpsCase>& info) {
        return info.param.name;
    });

#endif PAYLOAD_CAMERA_EO_NIGHT_MODE_FPS

// =============================================================================
// 15. Night Mode Option C_G_N_MODE_OPT
// =============================================================================
// Night Mode Options	C_G_N_MODE_OPT	-	-	x	-
#if defined(PAYLOAD_CAMERA_EO_NIGHT_MODE) && defined(MB1)
using NightModeOptionCase = CameraEoParamValue;

class CameraEoNightModeOptionSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<NightModeOptionCase> {};

TEST_P(CameraEoNightModeOptionSettingValueTest, SetAndReadBack) {
    const NightModeOptionCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_NIGHT_MODE, original, 3000)) {
        GTEST_SKIP() << "Payload did not report EO night mode Options ("
                     << PAYLOAD_CAMERA_EO_NIGHT_MODE << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_NIGHT_MODE,
                                     param.value, actual))
        << "No readback after setting EO night mode Options to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO night mode Options readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO night mode FPS to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_NIGHT_MODE, static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO night mode Options to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoNightModeOptionSettingValueTest,
    ::testing::Values(
        NightModeOptionCase{"Auto", PAYLOAD_CAMERA_EO_NIGHT_MODE_AUTO},
        NightModeOptionCase{"Off", PAYLOAD_CAMERA_EO_NIGHT_MODE_OFF},
        NightModeOptionCase{"On", PAYLOAD_CAMERA_EO_NIGHT_MODE_ON}),
    [](const ::testing::TestParamInfo<NightModeOptionCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_NIGHT_MODE

// =============================================================================
// 16. EO Profile C_G_PROFILE
// =============================================================================
// Camera Profile	C_G_PROFILE	-	-	x	-
#if defined(PAYLOAD_CAMERA_EO_PROFILE) && defined(MB1)

using ProfileCase = CameraEoParamValue;

class CameraEoProfileSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<ProfileCase> {};

TEST_P(CameraEoProfileSettingValueTest, SetAndReadBack) {
    const ProfileCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_PROFILE, original, 3000)) {
        GTEST_SKIP() << "Payload did not report EO camera profile ("
                     << PAYLOAD_CAMERA_EO_PROFILE << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_PROFILE, param.value,
                                     actual))
        << "No readback after setting EO camera profile to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO camera profile readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO Camera Profile to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_PROFILE, static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO Camera Profile to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoProfileSettingValueTest,
    ::testing::Values(
        ProfileCase{"Custom", PAYLOAD_CAMERA_EO_PROFILE_CUSTOM},
        ProfileCase{"Daylight", PAYLOAD_CAMERA_EO_PROFILE_DAYLIGHT},
        ProfileCase{"NightMode", PAYLOAD_CAMERA_EO_PROFILE_NIGHT_MODE}),
    [](const ::testing::TestParamInfo<ProfileCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_PROFILE

// =============================================================================
// 17. EO Saturation
// =============================================================================
// Saturation	C_G_SATU	-	-	x	-
#if defined(PAYLOAD_CAMERA_EO_SATURATION) && defined(MB1)

using SaturationCase = CameraEoParamValue;

class CameraEoSaturationSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<SaturationCase> {};

TEST_P(CameraEoSaturationSettingValueTest, SetAndReadBack) {
    const SaturationCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_SATURATION, original, 3000)) {
        GTEST_SKIP() << "Payload did not report EO saturation ("
                     << PAYLOAD_CAMERA_EO_SATURATION << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_SATURATION, param.value,
                                     actual))
        << "No readback after setting EO saturation to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO saturation readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO Saturation to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_SATURATION, static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO Saturation to " << original << ".";

}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoSaturationSettingValueTest,
    ::testing::Values(SaturationCase{"0", 0}, SaturationCase{"1", 1},
                       SaturationCase{"2", 2}, SaturationCase{"3", 3},
                       SaturationCase{"4", 4}, SaturationCase{"5", 5},
                       SaturationCase{"6", 6}, SaturationCase{"7", 7},
                       SaturationCase{"8", 8}, SaturationCase{"9", 9},
                       SaturationCase{"10", 10}),
    [](const ::testing::TestParamInfo<SaturationCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_SATURATION

// =============================================================================
// 18. EO Camera optimazations C_G_SCENE
// =============================================================================
// Camera optimizations	C_G_SCENE	-	-	x	-
#if defined(PAYLOAD_CAMERA_EO_SCENE_MODE) && defined(MB1)

using SceneModeCase = CameraEoParamValue;

class CameraEoSceneSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<SceneModeCase> {};

TEST_P(CameraEoSceneSettingValueTest, SetAndReadBack) {
    const SceneModeCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_SCENE_MODE, original, 3000)) {
        GTEST_SKIP() << "Payload did not report EO scene mode ("
                     << PAYLOAD_CAMERA_EO_SCENE_MODE << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_SCENE_MODE, param.value,
                                     actual))
        << "No readback after setting EO scene mode to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO scene mode readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO Scene Mode to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_SCENE_MODE, static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO Scene Mode to " << original << ".";  
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoSceneSettingValueTest,
    ::testing::Values(
        SceneModeCase{"Disabled", PAYLOAD_CAMERA_EO_SCENE_DISABLED},
        SceneModeCase{"FacePriority",
                     PAYLOAD_CAMERA_EO_SCENE_FACE_PRIORITY},
        SceneModeCase{"Action", PAYLOAD_CAMERA_EO_SCENE_ACTION},
        SceneModeCase{"Portrait", PAYLOAD_CAMERA_EO_SCENE_PORTRAIT},
        SceneModeCase{"Landscape", PAYLOAD_CAMERA_EO_SCENE_LANDSCAPE},
        SceneModeCase{"Night", PAYLOAD_CAMERA_EO_SCENE_NIGHT},
        SceneModeCase{"NightPortrait",
                     PAYLOAD_CAMERA_EO_SCENE_NIGHT_PORTRAIT},
        SceneModeCase{"Theatre", PAYLOAD_CAMERA_EO_SCENE_THEATRE},
        SceneModeCase{"Beach", PAYLOAD_CAMERA_EO_SCENE_BEACH},
        SceneModeCase{"Snow", PAYLOAD_CAMERA_EO_SCENE_SNOW},
        SceneModeCase{"Sunset", PAYLOAD_CAMERA_EO_SCENE_SUNSET},
        SceneModeCase{"SteadyPhoto",
                     PAYLOAD_CAMERA_EO_SCENE_STEADY_PHOTO},
        SceneModeCase{"Fireworks", PAYLOAD_CAMERA_EO_SCENE_FIREWORKS},
        SceneModeCase{"Sports", PAYLOAD_CAMERA_EO_SCENE_SPORTS},
        SceneModeCase{"Party", PAYLOAD_CAMERA_EO_SCENE_PARTY},
        SceneModeCase{"Candlelight", PAYLOAD_CAMERA_EO_SCENE_CANDLELIGHT},
        SceneModeCase{"Hdr", PAYLOAD_CAMERA_EO_SCENE_HDR}),
    [](const ::testing::TestParamInfo<SceneModeCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_SCENE_MODE

// =============================================================================
// 19. EO Sharpness
// =============================================================================
// Sharpness	C_G_SHARPNESS	-	-	x	-
#if defined(PAYLOAD_CAMERA_EO_SHARPNESS) && defined(MB1)

using SharpnessCase = CameraEoParamValue;

class CameraEoSharpnessSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<SharpnessCase> {};

TEST_P(CameraEoSharpnessSettingValueTest, SetAndReadBack) {
    const SharpnessCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_SHARPNESS, original, 3000)) {
        GTEST_SKIP() << "Payload did not report EO sharpness ("
                     << PAYLOAD_CAMERA_EO_SHARPNESS << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_SHARPNESS, param.value,
                                     actual))
        << "No readback after setting EO sharpness to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO sharpness readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO Sharpness to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_SHARPNESS, static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO Sharpness to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoSharpnessSettingValueTest,
    ::testing::Values(SharpnessCase{"0", 0}, SharpnessCase{"1", 1},
                       SharpnessCase{"2", 2}, SharpnessCase{"3", 3},
                       SharpnessCase{"4", 4}, SharpnessCase{"5", 5},
                       SharpnessCase{"6", 6}),
    [](const ::testing::TestParamInfo<SharpnessCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_SHARPNESS

// =============================================================================
// 20. EO Super HDR C_G_SHDR
// =============================================================================
// Super HDR	C_G_SHDR	-	-	x	-
#if defined(PAYLOAD_CAMERA_EO_SUPER_HDR) && defined(MB1)

using SuperHdrCase = CameraEoParamValue;

class CameraEoSuperHdrSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<SuperHdrCase> {};

TEST_P(CameraEoSuperHdrSettingValueTest, SetAndReadBack) {
    const SuperHdrCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_SUPER_HDR, original, 3000)) {
        GTEST_SKIP() << "Payload did not report EO Super HDR ("
                     << PAYLOAD_CAMERA_EO_SUPER_HDR << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_SUPER_HDR, param.value,
                                     actual))
        << "No readback after setting EO Super HDR to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO Super HDR readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO Super HDR to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_SUPER_HDR, static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO Super HDR to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoSuperHdrSettingValueTest,
    ::testing::Values(
        SuperHdrCase{"Off", PAYLOAD_CAMERA_EO_SUPER_HDR_OFF},
        SuperHdrCase{"On", PAYLOAD_CAMERA_EO_SUPER_HDR_ON}),
    [](const ::testing::TestParamInfo<SuperHdrCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_SUPER_HDR

// =============================================================================
// 21. Video Shutter Speed C_G_SHUTTER
// =============================================================================
// Video Shutter Speed	C_G_SHUTTER	-	-	?	-


// =============================================================================
// 22. EO White Balance C_G_WB
// =============================================================================
// White Balance	C_G_WB	-	-	x	-
#if defined(MB1) && defined(PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE)
// #if defined(PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE)

using WhiteBalanceCase = CameraEoParamValue;

class CameraEoWhiteBalanceSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<WhiteBalanceCase> {};

TEST_P(CameraEoWhiteBalanceSettingValueTest, SetAndReadBack) {
    const WhiteBalanceCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE, original,
                               3000)) {
        GTEST_SKIP() << "Payload did not report EO white balance ("
                     << PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE,
                                     param.value, actual))
        << "No readback after setting EO white balance to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO white balance readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO White Balance to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE, static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO White Balance to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoWhiteBalanceSettingValueTest,
    ::testing::Values(
        WhiteBalanceCase{"Off", PAYLOAD_CAMERA_EO_WB_OFF},
        WhiteBalanceCase{"ManualCcTemp",
                         PAYLOAD_CAMERA_EO_WB_MANUAL_CC_TEMP},
        WhiteBalanceCase{"ManualRgbGains",
                         PAYLOAD_CAMERA_EO_WB_MANUAL_RGB_GAINS},
        WhiteBalanceCase{"Auto", PAYLOAD_CAMERA_EO_WB_AUTO},
        WhiteBalanceCase{"Shade", PAYLOAD_CAMERA_EO_WB_SHADE},
        WhiteBalanceCase{"Incandescent",
                         PAYLOAD_CAMERA_EO_WB_INCANDESCENT},
        WhiteBalanceCase{"Fluorescent",
                         PAYLOAD_CAMERA_EO_WB_FLUORESCENT},
        WhiteBalanceCase{"WarmFluorescent",
                         PAYLOAD_CAMERA_EO_WB_WARM_FLUORESCENT},
        WhiteBalanceCase{"Daylight", PAYLOAD_CAMERA_EO_WB_DAYLIGHT},
        WhiteBalanceCase{"CloudyDaylight",
                         PAYLOAD_CAMERA_EO_WB_CLOUDY_DAYLIGHT},
        WhiteBalanceCase{"Twilight", PAYLOAD_CAMERA_EO_WB_TWILIGHT}),
    [](const ::testing::TestParamInfo<WhiteBalanceCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE

// =============================================================================
// 23. Video Auto Exposure C_V_AE
// =============================================================================
// EO Auto Exposure Mode	C_V_AE	x	x	-	x
#if defined(PAYLOAD_CAMERA_VIDEO_AUTO_EXPOSURE) 

using AutoExposureCase = CameraEoParamValue;

class CameraEoVideoAutoExposureSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<AutoExposureCase> {};

TEST_P(CameraEoVideoAutoExposureSettingValueTest, SetAndReadBack) {
    const AutoExposureCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_VIDEO_AUTO_EXPOSURE, original, 3000)) {
        GTEST_SKIP() << "Payload did not report video auto exposure ("
                     << PAYLOAD_CAMERA_VIDEO_AUTO_EXPOSURE << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_VIDEO_AUTO_EXPOSURE,
                                     param.value, actual))
        << "No readback after setting video auto exposure to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "Video auto exposure readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring video auto exposure to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_VIDEO_AUTO_EXPOSURE, static_cast<uint32_t>(original), actual))
        << "No readback after restoring video auto exposure to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoVideoAutoExposureSettingValueTest,
    ::testing::Values(AutoExposureCase{"Auto", 0},
                       AutoExposureCase{"Manual", 3},
                       AutoExposureCase{"Shutter", 10},
                       AutoExposureCase{"Iris", 11},
                       AutoExposureCase{"Bright", 13},
                       AutoExposureCase{"Gain", 14}),
    [](const ::testing::TestParamInfo<AutoExposureCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_VIDEO_AUTO_EXPOSURE


// =============================================================================
// 24. EO B Gain Value C_V_BGAIN
// =============================================================================
// VIO || ORUSL || ZIO
// EO B Gain Value	C_V_BGAIN	x	x	-	x
#if defined(PAYLOAD_CAMERA_EO_B_GAIN)
 using BGainCase = CameraEoParamValue;

class CameraEoBGainSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<BGainCase> {};

TEST_P(CameraEoBGainSettingValueTest, SetAndReadBack) {
    const BGainCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_B_GAIN, original, 3000)) {
        GTEST_SKIP() << "Payload did not report EO B gain ("
                     << PAYLOAD_CAMERA_EO_B_GAIN << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_B_GAIN, param.value, actual))
        << "No readback after setting EO B gain to " << param << ". ";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO B gain readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO B Gain to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_B_GAIN, static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO B Gain to " << original << ".";
}
INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoBGainSettingValueTest,
    ::testing::Values(BGainCase{"0", 0},
                       BGainCase{"1", 1},
                       BGainCase{"2", 2},
                       BGainCase{"10", 10},
                       BGainCase{"15", 15},
                       BGainCase{"100", 100},
                       BGainCase{"101", 101},
                       BGainCase{"120", 120},
                       BGainCase{"200", 200},
                       BGainCase{"250", 250},
                       BGainCase{"255", 255},
                       BGainCase{"256", 256}),
    [](const ::testing::TestParamInfo<BGainCase>& info) {
        return info.param.name;
    });
#endif  // PAYLOAD_CAMERA_EO_B_GAIN

// =============================================================================
// 25. EO Brigth Value C_V_BrP_HS
// =============================================================================
// Bright Value	C_V_BrP_HS	?	?	-	x
#if defined(PAYLOAD_CAMERA_VIDEO_BRIGHT_HS_VALUE) && defined(ZIO)

using BrightHsCase = CameraEoParamValue;

class CameraEoBrightHsSettingValuetest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<BrightHsCase> {};

TEST_P(CameraEoBrightHsSettingValueTest, SetAndReadBack) {
    const BrightHsCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_VIDEO_BRIGHT_HS_VALUE, original, 3000)) {
        GTEST_SKIP() << "Payload did not report EO Bright HS Value ("
                     << PAYLOAD_CAMERA_VIDEO_BRIGHT_HS_VALUE << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_VIDEO_BRIGHT_HS_VALUE, param.value, actual))
                                     actual))
        << "No readback after setting EO Bright HS Value to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO Bright HS Value readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO Bright HS Value to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_VIDEO_BRIGHT_HS_VALUE, static_cast<uint32_t>(original), actual))
        << "No read back after restoring EO Bright HS Value to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoBrightHsSettingValuetest,
    ::testing::Values(BrightHsCase{"0", 0},
                       BrightHsCase{"1", 1},
                       BrightHsCase{"2", 2},
                       BrightHsCase{"3", 3},
                       BrightHsCase{"4", 4},
                       BrightHsCase{"20", 20},
                       BrightHsCase{"25", 25},
                       BrightHsCase{"30", 30},
                       BrightHsCase{"40", 40},
                       BrightHsCase{"41", 41},
                       BrightHsCase{"42", 42}),
    [](const ::testing::TestParamInfo<BrightHsCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_VIDEO_BRIGHT_HS_VALUE


// =============================================================================
// 26. EO Brigth Value C_V_BrP_LS
// =============================================================================
// Bright Value	C_V_BrP_LS	?	?	-	x
#if defined(PAYLOAD_CAMERA_VIDEO_BRIGHT_LS_VALUE) && defined(ZIO)

using BrightLsCase = CameraEoParamValue;

class CameraEoBrightLsSettingValuetest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<BrightLsCase> {};

TEST_P(CameraEoBrightLsSettingValueTest, SetAndReadBack) {
    const BrightLsCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_VIDEO_BRIGHT_LS_VALUE, original, 3000)) {
        GTEST_SKIP() << "Payload did not report EO Bright LS Value ("
                     << PAYLOAD_CAMERA_VIDEO_BRIGHT_LS_VALUE << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_VIDEO_BRIGHT_LS_VALUE, param.value,
                                     actual))
        << "No readback after setting EO Bright LS Value to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO Bright LS Value readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO Bright LS Value to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_VIDEO_BRIGHT_LS_VALUE, static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO Bright LS Value to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoBrightLsSettingValuetest,
    ::testing::Values(BrightLsCase{"0", 0},
                       BrightLsCase{"1", 1},
                       BrightLsCase{"2", 2},
                       BrightLsCase{"3", 3},
                       BrightLsCase{"4", 4},
                       BrightLsCase{"20", 20},
                       BrightLsCase{"25", 25},
                       BrightLsCase{"30", 30},
                       BrightLsCase{"36", 36},
                       BrightLsCase{"37", 37},
                       BrightLsCase{"38", 38}),
    [](const ::testing::TestParamInfo<BrightLsCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_VIDEO_BRIGHT_LS_VALUE

// =============================================================================
// 27. EO Defog C_V_DEFOG
// =============================================================================
// VIO || ORUSL || ZIO
// EO Defog Mode	C_V_DEFOG	x	x	-	x
#if defined(PAYLOAD_CAMERA_VIDEO_DEFOG)

using DefogCase = CameraEoParamValue;

class CameraEoDefogSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<DefogCase> {};

TEST_P(CameraEoDefogSettingValueTest, SetAndReadBack) {
    const DefogCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_VIDEO_DEFOG, original, 3000)) {
        GTEST_SKIP() << "Payload did not report EO defog ("
                     << PAYLOAD_CAMERA_VIDEO_DEFOG << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_VIDEO_DEFOG, param.value,
                                     actual))
        << "No readback after setting EO defog to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO defog readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO Defog to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_VIDEO_DEFOG, static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO Defog to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoDefogSettingValueTest,
    ::testing::Values(DefogCase{"Off", PAYLOAD_CAMERA_VIDEO_DEFOG_OFF},
                       DefogCase{"On", PAYLOAD_CAMERA_VIDEO_DEFOG_ON}),
    [](const ::testing::TestParamInfo<DefogCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_VIDEO_DEFOG

// =============================================================================
// 28. EO Defog Level C_V_DEFOG_LV
// =============================================================================
// VIO || ORUSL || ZIO
// EO Defog Level	C_V_DEFOG_LV	x	x	-	x
#if defined(PAYLOAD_CAMERA_VIDEO_DEFOG_LEVEL)

using DefogLevelCase = CameraEoParamValue;

class CameraEoDefogLevelSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<DefogLevelCase> {};

TEST_P(CameraEoDefogLevelSettingValueTest, SetAndReadBack) {
    const DefogLevelCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_VIDEO_DEFOG_LEVEL, original,
                               3000)) {
        GTEST_SKIP() << "Payload did not report EO defog level ("
                     << PAYLOAD_CAMERA_VIDEO_DEFOG_LEVEL << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_VIDEO_DEFOG_LEVEL,
                                     param.value, actual))
        << "No readback after setting EO defog level to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO defog level readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO Defog level to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_VIDEO_DEFOG_LEVEL, static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO Defog level to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoDefogLevelSettingValueTest,
    ::testing::Values(
        DefogLevelCase{"Lowest", PAYLOAD_CAMERA_VIDEO_DEFOG_LOWEST},
        DefogLevelCase{"Low", PAYLOAD_CAMERA_VIDEO_DEFOG_LOW},
        DefogLevelCase{"Mid", PAYLOAD_CAMERA_VIDEO_DEFOG_MID},
        DefogLevelCase{"High", PAYLOAD_CAMERA_VIDEO_DEFOG_HIGH}),
    [](const ::testing::TestParamInfo<DefogLevelCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_VIDEO_DEFOG_LEVEL

// =============================================================================
// 29. EO Image Stabilizer
// =============================================================================
// ORUSL
// EO Image Stabilizer	C_V_EIS	-	x	-	-
#if defined(PAYLOAD_CAMERA_EO_EIS_MODE) && defined(ORUSL)

using EISCase = CameraEoParamValue;

class CameraEoImageStabilizerSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<EISCase> {};

TEST_P(CameraEoImageStabilizerSettingValueTest, SetAndReadBack) {
    const EISCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_EIS_MODE, original,
                               3000)) {
        GTEST_SKIP() << "Payload did not report EO Image Stabilizer ("
                     << PAYLOAD_CAMERA_EO_EIS_MODE << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_EIS_MODE,
                                     param.value, actual))
        << "No readback after setting EO Image Stabilizer to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO Image Stabilizer readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO Image Stabilizer to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_EIS_MODE, static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO Image Stabilizer to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoImageStabilizerSettingValueTest,
    ::testing::Values(
        EISCase{"Hold", PAYLOAD_CAMERA_EO_EIS_MODE_HOLD},
        EISCase{"On", PAYLOAD_CAMERA_EO_EIS_MODE_ON},
        EISCase{"Off", PAYLOAD_CAMERA_EO_EIS_MODE_OFF}),
    [](const ::testing::TestParamInfo<EISCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_EIS_MODE

// =============================================================================
// 30. EO Image Stabilizer Level
// =============================================================================
// EO Image Stabilizer Level	C_V_EIS_LV	-	x	-	-
#if defined(PAYLOAD_CAMERA_EO_EIS_LEVEL) && defined(ORUSL)

using EISLevelCase = CameraEoParamValue;

class CameraEoImageStabilizerLevelSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<EISLevelCase> {};

TEST_P(CameraEoImageStabilizerLevelSettingValueTest, SetAndReadBack) {
    const EISLevelCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_EIS_LEVEL, original,
                               3000)) {
        GTEST_SKIP() << "Payload did not report EO Image Stabilizer Level("
                     << PAYLOAD_CAMERA_EO_EIS_LEVEL << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_EIS_LEVEL,
                                     param.value, actual))
        << "No readback after setting EO Image Stabilizer Level to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO Image Stabilizer Level readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO Image Stabilizer Level to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_EIS_LEVEL, static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO Image Stabilizer Level to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoImageStabilizerLevelSettingValueTest,
    ::testing::Values(
        EISLevelCase{"Super", PAYLOAD_CAMERA_EO_EIS_LEVEL_SUPER},
        EISLevelCase{"SuperPlus", PAYLOAD_CAMERA_EO_EIS_LEVEL_SUPER_PLUS},),
    [](const ::testing::TestParamInfo<EISLevelCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_EIS_LEVEL

// =============================================================================
// 31. EO Flip
// =============================================================================
// EO Flip	C_V_FLIP	x	x	x	x
#if defined(PAYLOAD_CAMERA_EO_FLIP)

using FlipCase = CameraEoParamValue;

class CameraEoFlipSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<FlipCase> {
protected:
    void TearDown() override {
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        CameraEoParamSettingTest::TearDown();
    }
      };

TEST_P(CameraEoFlipSettingValueTest, SetAndReadBack) {
    const FlipCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_FLIP, original, 3000)) {
        GTEST_SKIP() << "Payload did not report EO flip ("
                     << PAYLOAD_CAMERA_EO_FLIP << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_FLIP, param.value,
                                     actual))
        << "No readback after setting EO flip to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO flip readback mismatch for " << param << ".";
    // std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    // Restore original value
    std::cout << "Restoring EO Flip to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_FLIP, static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO Flip to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoFlipSettingValueTest,
    ::testing::Values(FlipCase{"Off", PAYLOAD_CAMERA_EO_FLIP_OFF},
                       FlipCase{"On", PAYLOAD_CAMERA_EO_FLIP_ON}),
    [](const ::testing::TestParamInfo<FlipCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_FLIP

// =============================================================================
// 32. EO Flicker Reduction
// =============================================================================
// EO Flicker Reduction	C_V_FLREDUCT	-	x	-	-
#if defined(PAYLOAD_CAMERA_EO_FLICKER_REDUCTION) && defined(ORUSL)

using FlickerReductionCase = CameraEoParamValue;

class CameraEoFlickerReductionSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<FlickerReductionCase> {};

TEST_P(CameraEoFlickerReductionSettingValueTest, SetAndReadBack) {
    const FlickerReductionCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_FLICKER_REDUCTION, original, 3000)) {
        GTEST_SKIP() << "Payload did not report EO Flicker Reduction ("
                     << PAYLOAD_CAMERA_EO_FLICKER_REDUCTION << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_FLICKER_REDUCTION, param.value,
                                     actual))
        << "No readback after setting EO Flicker Reduction to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO Flicker Reduction readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO Flicker Reduction to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_FLICKER_REDUCTION, static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO Flicker Reduction to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoFlickerReductionSettingValueTest,
    ::testing::Values(FlickerReductionCase{"On", PAYLOAD_CAMERA_EO_FLICKER_REDUCTION_ON},
                       FlickerReductionCase{"Off", PAYLOAD_CAMERA_EO_FLICKER_REDUCTION_OFF}),
    [](const ::testing::TestParamInfo<FlickerReductionCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_VIDEO_DEFOG

// // =============================================================================
// // 33. EO Focus Mode
// // =============================================================================
// VIO || ORUSL || ZIO
// EO Focus Mode	C_V_FM	x	x	-	x
#if defined(PAYLOAD_CAMERA_VIDEO_FOCUS_MODE)

using FocusModeCase = CameraEoParamValue;

class CameraEoFocusModeSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<DefogLevelCase> {};

TEST_P(CameraEoFocusModeSettingValueTest, SetAndReadBack) {
    const FocusModeCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_VIDEO_FOCUS_MODE, original,
                               3000)) {
        GTEST_SKIP() << "Payload did not report EO Focus Mode ("
                     << PAYLOAD_CAMERA_VIDEO_FOCUS_MODE << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_VIDEO_FOCUS_MODE,
                                     param.value, actual))
        << "No readback after setting EO Focus Mode to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO Focus Mode readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO Focus Mode to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_VIDEO_FOCUS_MODE, static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO Focus Mode to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoFocusModeSettingValueTest,
    ::testing::Values(
        FocusModeCase{"Manual", PAYLOAD_CAMERA_VIDEO_FOCUS_MODE_MANUAL},
        FocusModeCase{"AutoFocus", PAYLOAD_CAMERA_VIDEO_FOCUS_MODE_AUTO_FOCUS},
        FocusModeCase{"AutoFocusOnePush", PAYLOAD_CAMERA_VIDEO_FOCUS_MODE_AUTO_FOCUS_ONEPUSH}),
    [](const ::testing::TestParamInfo<FocusModeCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_VIDEO_FOCUS_MODE

// =============================================================================
// 34. Eo Focus Speed
// =============================================================================
// Eo Focus Speed	C_V_FOCUS_SPEED	-	-	?	-

// =============================================================================
// 35. EO Freeze
// =============================================================================
// VIO || ORUSL || ZIO
// EO Freeze	C_V_FREEZE	x	x	-	x
#if defined(PAYLOAD_CAMERA_EO_FREEZE)

using FreezeCase = CameraEoParamValue;

class CameraEoFreezeSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<FreezeCase> {};

TEST_P(CameraEoFreezeSettingValueTest, SetAndReadBack) {
    const FreezeCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_FREEZE, original, 3000)) {
        GTEST_SKIP() << "Payload did not report EO Freeze ("
                     << PAYLOAD_CAMERA_EO_FREEZE << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_FREEZE,
                                     param.value, actual))
        << "No readback after setting EO Freeze to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO Freeze readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO Freeze Value to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_FREEZE, static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO Freeze Value to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoFreezeSettingValueTest,
    ::testing::Values(
        FreezeCase{"Off", PAYLOAD_CAMERA_EO_FREEZE_OFF},
        FreezeCase{"On", PAYLOAD_CAMERA_EO_FREEZE_ON}),
    [](const ::testing::TestParamInfo<FreezeCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_FREEZE
// =============================================================================
// 36. EO Manual Focus Value
// =============================================================================
//EO Manual Focus Value	C_V_FV	x	x	-	x
#if defined(PAYLOAD_CAMERA_VIDEO_FOCUS_VALUE)

using ManualFocusCase = CameraEoParamValue;

class CameraEoManualFocusSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<ManualFocusCase> {};

TEST_P(CameraEoManualFocusSettingValueTest, SetAndReadBack) {
    const ManualFocusCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_VIDEO_FOCUS_VALUE, original,
                               3000)) {
        GTEST_SKIP() << "Payload did not report EO Manual Focus Value ("
                     << PAYLOAD_CAMERA_VIDEO_FOCUS_VALUE << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_VIDEO_FOCUS_VALUE,
                                     param.value, actual))
        << "No readback after setting EO Manual Focus Value to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO Manual Focus Value readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO Manual Focus Value to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_VIDEO_FOCUS_VALUE, static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO Manual Focus Value to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoManualFocusSettingValueTest,
    ::testing::Values(ManualFocusCase{"0", 0},
                       ManualFocusCase{"1", 1},
                       ManualFocusCase{"2", 2},
                       ManualFocusCase{"100", 100},
                       ManualFocusCase{"32110", 32110},
                       ManualFocusCase{"50000", 50000},
                       ManualFocusCase{"61439", 61439},
                       ManualFocusCase{"61440", 61440}),
    [](const ::testing::TestParamInfo<ManualFocusCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_VIDEO_FOCUS_VALUE
// =============================================================================
// 37. EO Focus Speed
// =============================================================================
// EO Focus Speed	C_V_F_SPD	x	x	-	-
#if defined(PAYLOAD_CAMERA_EO_FOCUS_SPEED)

using FocusSpeedCase = CameraEoParamValue;

class CameraEoFocusSpeedSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<FocusSpeedCase> {};

TEST_P(CameraEoFocusSpeedSettingValueTest, SetAndReadBack) {
    const FocusSpeedCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_FOCUS_SPEED, original,
                               3000)) {
        GTEST_SKIP() << "Payload did not report EO Focus Speed Value ("
                     << PAYLOAD_CAMERA_EO_FOCUS_SPEED << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_FOCUS_SPEED,
                                     param.value, actual))
        << "No readback after setting EO Focus Speed Value to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO Focus Speed Value readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO Focus Speed Value to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_FOCUS_SPEED, static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO Focus Speed Value to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraEoFocusSpeedSettingValueTest,
    ::testing::Values(FocusSpeedCase{"0", 0},
                       FocusSpeedCase{"1", 1},
                       FocusSpeedCase{"2", 2},
                       FocusSpeedCase{"3", 3},
                       FocusSpeedCase{"4", 4},
                       FocusSpeedCase{"5", 5},
                       FocusSpeedCase{"6", 6},
                       FocusSpeedCase{"7", 7},
                       FocusSpeedCase{"8", 8}),
    [](const ::testing::TestParamInfo<FocusSpeedCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_FOCUS_SPEED

// =============================================================================
// 38. Gain Value C_V_GAIN_HS
// =============================================================================
// Gain Value	C_V_GAIN_HS	?	-	-	x
#if defined(PAYLOAD_CAMERA_EO_GAIN_HS)  && defined(ZIO)

using GainHsCase = CameraEoParamValue;

class CameraEoGainHsSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<GainHsCase> {};

TEST_P(CameraEoGainHsSettingValueTest, SetAndReadBack) {
    const GainHsCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_GAIN_HS, original,
                               3000)) {
        GTEST_SKIP() << "Payload did not report EO Gain HS Value ("
                     << PAYLOAD_CAMERA_EO_GAIN_HS << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_GAIN_HS,
                                     param.value, actual))
        << "No readback after setting EO Gain HS Value to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO Gain HS Value readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO Gain HS Value to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_GAIN_HS, static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO Gain HS Value to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues,
    CameraEoGainHsSettingValueTest,
    ::testing::Values(
        GainHsCase{"0DB",  PAYLOAD_CAMERA_EO_GAIN_HS_0DB},
        GainHsCase{"3DB",  PAYLOAD_CAMERA_EO_GAIN_HS_3DB},
        GainHsCase{"6DB",  PAYLOAD_CAMERA_EO_GAIN_HS_6DB},
        GainHsCase{"9DB",  PAYLOAD_CAMERA_EO_GAIN_HS_9DB},
        GainHsCase{"12DB", PAYLOAD_CAMERA_EO_GAIN_HS_12DB},
        GainHsCase{"15DB", PAYLOAD_CAMERA_EO_GAIN_HS_15DB},
        GainHsCase{"18DB", PAYLOAD_CAMERA_EO_GAIN_HS_18DB},
        GainHsCase{"21DB", PAYLOAD_CAMERA_EO_GAIN_HS_21DB},
        GainHsCase{"24DB", PAYLOAD_CAMERA_EO_GAIN_HS_24DB},
        GainHsCase{"27DB", PAYLOAD_CAMERA_EO_GAIN_HS_27DB},
        GainHsCase{"30DB", PAYLOAD_CAMERA_EO_GAIN_HS_30DB},
        GainHsCase{"33DB", PAYLOAD_CAMERA_EO_GAIN_HS_33DB},
        GainHsCase{"36DB", PAYLOAD_CAMERA_EO_GAIN_HS_36DB},
        GainHsCase{"39DB", PAYLOAD_CAMERA_EO_GAIN_HS_39DB},
        GainHsCase{"42DB", PAYLOAD_CAMERA_EO_GAIN_HS_42DB},
        GainHsCase{"45DB", PAYLOAD_CAMERA_EO_GAIN_HS_45DB},
        GainHsCase{"48DB", PAYLOAD_CAMERA_EO_GAIN_HS_48DB}
    ),
    [](const ::testing::TestParamInfo<GainHsCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_GAIN_HS
// =============================================================================
// 39. Gain Value C_V_GAIN_LS
// =============================================================================
// Gain Value	C_V_GAIN_LS	?	-	-	x
#if defined(PAYLOAD_CAMERA_EO_GAIN_LS) && defined(ZIO)

using GainLsCase = CameraEoParamValue;

class CameraEoGainLsSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<GainLsCase> {};

TEST_P(CameraEoGainLsSettingValueTest, SetAndReadBack) {
    const GainLsCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_GAIN_LS, original,
                               3000)) {
        GTEST_SKIP() << "Payload did not report EO Gain LS Value ("
                     << PAYLOAD_CAMERA_EO_GAIN_LS << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_GAIN_LS,
                                     param.value, actual))
        << "No readback after setting EO Gain LS Value to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO Gain LS Value readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO Gain LS Value to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_GAIN_LS, static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO Gain LS Value to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues,
    CameraEoGainLsSettingValueTest,
    ::testing::Values(
        GainLsCase{"0DB",  PAYLOAD_CAMERA_EO_GAIN_LS_0DB},
        GainLsCase{"3DB",  PAYLOAD_CAMERA_EO_GAIN_LS_3DB},
        GainLsCase{"6DB",  PAYLOAD_CAMERA_EO_GAIN_LS_6DB},
        GainLsCase{"9DB",  PAYLOAD_CAMERA_EO_GAIN_LS_9DB},
        GainLsCase{"12DB", PAYLOAD_CAMERA_EO_GAIN_LS_12DB},
        GainLsCase{"15DB", PAYLOAD_CAMERA_EO_GAIN_LS_15DB},
        GainLsCase{"18DB", PAYLOAD_CAMERA_EO_GAIN_LS_18DB},
        GainLsCase{"21DB", PAYLOAD_CAMERA_EO_GAIN_LS_21DB},
        GainLsCase{"24DB", PAYLOAD_CAMERA_EO_GAIN_LS_24DB},
        GainLsCase{"27DB", PAYLOAD_CAMERA_EO_GAIN_LS_27DB},
        GainLsCase{"30DB", PAYLOAD_CAMERA_EO_GAIN_LS_30DB},
        GainLsCase{"33DB", PAYLOAD_CAMERA_EO_GAIN_LS_33DB},
        GainLsCase{"36DB", PAYLOAD_CAMERA_EO_GAIN_LS_36DB},
        GainLsCase{"48DB", 14}
    ),
    [](const ::testing::TestParamInfo<GainLsCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_GAIN_LS
// =============================================================================
// 40. EO High Sensitivity C_V_HS
// =============================================================================
// EO High Sensitivity	C_V_HS	x	x	-	x
#if defined(PAYLOAD_CAMERA_EO_HS)

using HightSensitivityCase = CameraEoParamValue;

class CameraEoHightSensitivitySettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<HightSensitivityCase> {};

TEST_P(CameraEoHightSensitivitySettingValueTest, SetAndReadBack) {
    const HightSensitivityCase param = GetParam();
    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_HS, original,
                               3000)) {
        GTEST_SKIP() << "Payload did not report EO Hight Sensitivity Value ("
                     << PAYLOAD_CAMERA_EO_HS << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_HS,
                                     param.value, actual))
        << "No readback after setting EO Hight Sensitivity Value to " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO Hight Sensitivity Value readback mismatch for " << param << ".";
    // Restore original value
    std::cout << "Restoring EO Hight Sensitivity Value to original value " << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_EO_HS, static_cast<uint32_t>(original), actual))
        << "No readback after restoring EO Hight Sensitivity Value to " << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues,
    CameraEoHightSensitivitySettingValueTest,
    ::testing::Values(
        HightSensitivityCase{"Off",  PAYLOAD_CAMERA_EO_HS_OFF},
        HightSensitivityCase{"On", PAYLOAD_CAMERA_EO_HS_ON},
        HightSensitivityCase{"10", 10}
    ),
    [](const ::testing::TestParamInfo<HightSensitivityCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_HS

// =============================================================================
// 41. EO Auto ICR Mode C_V_ICR
// =============================================================================
// EO AutoICR Mode  C_V_ICR  x   x   -   x
#if defined(PAYLOAD_CAMERA_EO_ICR_MODE)

using IcrModeCase = CameraEoParamValue;

class CameraEoIcrModeSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<IcrModeCase> {};

TEST_P(CameraEoIcrModeSettingValueTest, SetAndReadBack) {
    const IcrModeCase param = GetParam();

    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_ICR_MODE,
                               original,
                               3000)) {
        GTEST_SKIP() << "Payload did not report EO ICR Mode ("
                     << PAYLOAD_CAMERA_EO_ICR_MODE << ").";
    }

    double actual = -1;

    ASSERT_TRUE(
        SetParamAndReadBack(PAYLOAD_CAMERA_EO_ICR_MODE,param.value,actual))
        << "No readback after setting EO ICR Mode to "
        << param << ".";

    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO ICR Mode readback mismatch for "
        << param << ".";

    // Restore original value
    std::cout << "Restoring EO ICR Mode to original value "
              << original << "." << std::endl;

    ASSERT_TRUE(
        SetParamAndReadBack(
            PAYLOAD_CAMERA_EO_ICR_MODE,
            static_cast<uint32_t>(original),
            actual))
        << "No readback after restoring EO ICR Mode to "
        << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues,
    CameraEoIcrModeSettingValueTest,
    ::testing::Values(
        IcrModeCase{"Auto",   PAYLOAD_CAMERA_EO_ICR_MODE_AUTO},
        IcrModeCase{"Manual", PAYLOAD_CAMERA_EO_ICR_MODE_MANUAL}
    ),
    [](const ::testing::TestParamInfo<IcrModeCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_ICR_MODE

// =============================================================================
// 42. EO ICR Mode C_V_ICR_MAN
// =============================================================================
// EO ICR Mode  C_V_ICR_MAN  x   x   -   x
#if defined(PAYLOAD_CAMERA_EO_ICR_MANUAL)

using IcrManualCase = CameraEoParamValue;

class CameraEoIcrManualSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<IcrManualCase> {};

TEST_P(CameraEoIcrManualSettingValueTest, SetAndReadBack) {
    const IcrManualCase param = GetParam();

    double original = -1;
    if (!cet::readCameraParam(PAYLOAD_CAMERA_EO_ICR_MANUAL,
                               original,
                               3000)) {
        GTEST_SKIP() << "Payload did not report EO ICR Manual Mode ("
                     << PAYLOAD_CAMERA_EO_ICR_MANUAL << ").";
    }

    double actual = -1;

    ASSERT_TRUE(
        SetParamAndReadBack(PAYLOAD_CAMERA_EO_ICR_MANUAL,
                            param.value,
                            actual))
        << "No readback after setting EO ICR Manual Mode to "
        << param << ".";

    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO ICR Manual Mode readback mismatch for "
        << param << ".";

    // Restore original value
    std::cout << "Restoring EO ICR Manual Mode to original value "
              << original << "." << std::endl;

    ASSERT_TRUE(
        SetParamAndReadBack(
            PAYLOAD_CAMERA_EO_ICR_MANUAL,
            static_cast<uint32_t>(original),
            actual))
        << "No readback after restoring EO ICR Manual Mode to "
        << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues,
    CameraEoIcrManualSettingValueTest,
    ::testing::Values(
        IcrManualCase{"On",  PAYLOAD_CAMERA_EO_ICR_MANUAL_ON},
        IcrManualCase{"Off", PAYLOAD_CAMERA_EO_ICR_MANUAL_OFF}
    ),
    [](const ::testing::TestParamInfo<IcrManualCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_ICR_MANUAL

// =============================================================================
// 43. EO ICR Threshold C_V_ICR_THR
// =============================================================================
// EO ICR Threshold  C_V_ICR_THR  x   x   -   x
#if defined(PAYLOAD_CAMERA_EO_ICR_MODE_AUTO_THRESHOLD)

using IcrThresholdCase = CameraEoParamValue;

class CameraEoIcrThresholdSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<IcrThresholdCase> {};

TEST_P(CameraEoIcrThresholdSettingValueTest, SetAndReadBack) {
    const IcrThresholdCase param = GetParam();

    double original = -1;
    if (!cet::readCameraParam(
            PAYLOAD_CAMERA_EO_ICR_MODE_AUTO_THRESHOLD,
            original,
            3000)) {
        GTEST_SKIP() << "Payload did not report EO ICR Auto Threshold ("
                     << PAYLOAD_CAMERA_EO_ICR_MODE_AUTO_THRESHOLD
                     << ").";
    }

    double actual = -1;

    ASSERT_TRUE(
        SetParamAndReadBack(
            PAYLOAD_CAMERA_EO_ICR_MODE_AUTO_THRESHOLD,
            param.value,
            actual))
        << "No readback after setting EO ICR Auto Threshold to "
        << param << ".";

    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO ICR Auto Threshold readback mismatch for "
        << param << ".";

    // Restore original value
    std::cout << "Restoring EO ICR Auto Threshold to original value "
              << original << "." << std::endl;

    ASSERT_TRUE(
        SetParamAndReadBack(
            PAYLOAD_CAMERA_EO_ICR_MODE_AUTO_THRESHOLD,
            static_cast<uint32_t>(original),
            actual))
        << "No readback after restoring EO ICR Auto Threshold to "
        << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues,
    CameraEoIcrThresholdSettingValueTest,
    ::testing::Values(
        IcrThresholdCase{"0",   0},
        IcrThresholdCase{"1",   1},
        IcrThresholdCase{"64",  64},
        IcrThresholdCase{"128", 128},
        IcrThresholdCase{"192", 192},
        IcrThresholdCase{"254", 254},
        IcrThresholdCase{"255", 255}
    ),
    [](const ::testing::TestParamInfo<IcrThresholdCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_ICR_MODE_AUTO_THRESHOLD

// =============================================================================
// 44. EO Aperture Value C_V_IrP
// =============================================================================
// EO Aperture Value   C_V_IrP   x   x   -   x
#if defined(PAYLOAD_CAMERA_VIDEO_APERTURE_VALUE) && (defined(VIO) || defined(ORUSL) || defined(ZIO))

using ApertureCase = CameraEoParamValue;

class CameraEoApertureSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<ApertureCase> {};

TEST_P(CameraEoApertureSettingValueTest, SetAndReadBack) {
    const ApertureCase param = GetParam();

    double original = -1;
    if (!cet::readCameraParam(
            PAYLOAD_CAMERA_VIDEO_APERTURE_VALUE,
            original,
            3000)) {
        GTEST_SKIP() << "Payload did not report EO Aperture Value ("
                     << PAYLOAD_CAMERA_VIDEO_APERTURE_VALUE << ").";
    }

    double actual = -1;

    ASSERT_TRUE(
        SetParamAndReadBack(
            PAYLOAD_CAMERA_VIDEO_APERTURE_VALUE,
            param.value,
            actual))
        << "No readback after setting EO Aperture Value to "
        << param << ".";

    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO Aperture Value readback mismatch for "
        << param << ".";

    // Restore original value
    std::cout << "Restoring EO Aperture Value to original value "
              << original << "." << std::endl;

    ASSERT_TRUE(
        SetParamAndReadBack(
            PAYLOAD_CAMERA_VIDEO_APERTURE_VALUE,
            static_cast<uint32_t>(original),
            actual))
        << "No readback after restoring EO Aperture Value to "
        << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues,
    CameraEoApertureSettingValueTest,
    ::testing::Values(
        ApertureCase{"F2_0",  PAYLOAD_CAMERA_EO_APERTURE_F2_0},
        ApertureCase{"F2_2",  PAYLOAD_CAMERA_EO_APERTURE_F2_2},
        ApertureCase{"F2_4",  PAYLOAD_CAMERA_EO_APERTURE_F2_4},
        ApertureCase{"F2_6",  PAYLOAD_CAMERA_EO_APERTURE_F2_6},
        ApertureCase{"F2_8",  PAYLOAD_CAMERA_EO_APERTURE_F2_8},
        ApertureCase{"F3_1",  PAYLOAD_CAMERA_EO_APERTURE_F3_1},
        ApertureCase{"F3_4",  PAYLOAD_CAMERA_EO_APERTURE_F3_4},
        ApertureCase{"F4_0",  PAYLOAD_CAMERA_EO_APERTURE_F4_0},
        ApertureCase{"F5_2",  PAYLOAD_CAMERA_EO_APERTURE_F5_2},
        ApertureCase{"F6_8",  PAYLOAD_CAMERA_EO_APERTURE_F6_8},
        ApertureCase{"F7_3",  PAYLOAD_CAMERA_EO_APERTURE_F7_3},
        ApertureCase{"F8_7",  PAYLOAD_CAMERA_EO_APERTURE_F8_7},
        ApertureCase{"F9_6",  PAYLOAD_CAMERA_EO_APERTURE_F9_6},
        ApertureCase{"F10_0", PAYLOAD_CAMERA_EO_APERTURE_F10_0},
        ApertureCase{"F11_0", PAYLOAD_CAMERA_EO_APERTURE_F11_0},
        ApertureCase{"Close",  PAYLOAD_CAMERA_EO_APERTURE_CLOSE}
    ),
    [](const ::testing::TestParamInfo<ApertureCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_VIDEO_APERTURE_VALUE

// =============================================================================
// 45. EO Min Shutter Limit C_V_MinSP
// =============================================================================
// EO Min Shutter Limit   C_V_MinSP   x   x   -   x
#if defined(PAYLOAD_CAMERA_EO_SHUTTER_MIN_LIMIT)

using ShutterMinLimitCase = CameraEoParamValue;

class CameraEoShutterMinLimitSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<ShutterMinLimitCase> {};

TEST_P(CameraEoShutterMinLimitSettingValueTest, SetAndReadBack) {
    const ShutterMinLimitCase param = GetParam();

    double original = -1;
    if (!cet::readCameraParam(
            PAYLOAD_CAMERA_EO_SHUTTER_MIN_LIMIT,
            original,
            3000)) {
        GTEST_SKIP() << "Payload did not report EO Min Shutter Limit ("
                     << PAYLOAD_CAMERA_EO_SHUTTER_MIN_LIMIT << ").";
    }

    double actual = -1;

    ASSERT_TRUE(
        SetParamAndReadBack(
            PAYLOAD_CAMERA_EO_SHUTTER_MIN_LIMIT,
            param.value,
            actual))
        << "No readback after setting EO Min Shutter Limit to "
        << param << ".";

    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO Min Shutter Limit readback mismatch for "
        << param << ".";

    // Restore original value
    std::cout << "Restoring EO Min Shutter Limit to original value "
              << original << "." << std::endl;

    ASSERT_TRUE(
        SetParamAndReadBack(
            PAYLOAD_CAMERA_EO_SHUTTER_MIN_LIMIT,
            static_cast<uint32_t>(original),
            actual))
        << "No readback after restoring EO Min Shutter Limit to "
        << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues,
    CameraEoShutterMinLimitSettingValueTest,
    ::testing::Values(
        ShutterMinLimitCase{"1_10",   PAYLOAD_CAMERA_EO_SHUTTER_MIN_LIMIT_1_10},
        ShutterMinLimitCase{"1_15",   PAYLOAD_CAMERA_EO_SHUTTER_MIN_LIMIT_1_15},
        ShutterMinLimitCase{"1_20",   PAYLOAD_CAMERA_EO_SHUTTER_MIN_LIMIT_1_20},
        ShutterMinLimitCase{"1_30",   PAYLOAD_CAMERA_EO_SHUTTER_MIN_LIMIT_1_30},
        ShutterMinLimitCase{"1_50",   PAYLOAD_CAMERA_EO_SHUTTER_MIN_LIMIT_1_50},
        ShutterMinLimitCase{"1_60",   PAYLOAD_CAMERA_EO_SHUTTER_MIN_LIMIT_1_60},
        ShutterMinLimitCase{"1_90",   PAYLOAD_CAMERA_EO_SHUTTER_MIN_LIMIT_1_90},
        ShutterMinLimitCase{"1_100",  PAYLOAD_CAMERA_EO_SHUTTER_MIN_LIMIT_1_100},
        ShutterMinLimitCase{"1_125",  PAYLOAD_CAMERA_EO_SHUTTER_MIN_LIMIT_1_125},
        ShutterMinLimitCase{"1_180",  PAYLOAD_CAMERA_EO_SHUTTER_MIN_LIMIT_1_180},
        ShutterMinLimitCase{"1_250",  PAYLOAD_CAMERA_EO_SHUTTER_MIN_LIMIT_1_250},
        ShutterMinLimitCase{"1_350",  PAYLOAD_CAMERA_EO_SHUTTER_MIN_LIMIT_1_350},
        ShutterMinLimitCase{"1_500",  PAYLOAD_CAMERA_EO_SHUTTER_MIN_LIMIT_1_500},
        ShutterMinLimitCase{"1_725",  PAYLOAD_CAMERA_EO_SHUTTER_MIN_LIMIT_1_725},
        ShutterMinLimitCase{"1_1000", PAYLOAD_CAMERA_EO_SHUTTER_MIN_LIMIT_1_1000},
        ShutterMinLimitCase{"1_1500", PAYLOAD_CAMERA_EO_SHUTTER_MIN_LIMIT_1_1500}
    ),
    [](const ::testing::TestParamInfo<ShutterMinLimitCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_SHUTTER_MIN_LIMIT
// =============================================================================
// 46. EO Noise Reduction C_V_NSREDUCT
// =============================================================================
// EO Noise Reduction   C_V_NSREDUCT   -   x   -   -
#if defined(PAYLOAD_CAMERA_EO_NOISE_REDUCTION) && defined(ORUSL)

using NoiseReductionCase = CameraEoParamValue;

class CameraEoNoiseReductionSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<NoiseReductionCase> {};

TEST_P(CameraEoNoiseReductionSettingValueTest, SetAndReadBack) {
    const NoiseReductionCase param = GetParam();

    double original = -1;
    if (!cet::readCameraParam(
            PAYLOAD_CAMERA_EO_NOISE_REDUCTION,
            original,
            3000)) {
        GTEST_SKIP() << "Payload did not report EO Noise Reduction ("
                     << PAYLOAD_CAMERA_EO_NOISE_REDUCTION << ").";
    }

    double actual = -1;

    ASSERT_TRUE(
        SetParamAndReadBack(
            PAYLOAD_CAMERA_EO_NOISE_REDUCTION,
            param.value,
            actual))
        << "No readback after setting EO Noise Reduction to "
        << param << ".";

    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO Noise Reduction readback mismatch for "
        << param << ".";

    // Restore original value
    std::cout << "Restoring EO Noise Reduction to original value "
              << original << "." << std::endl;

    ASSERT_TRUE(
        SetParamAndReadBack(
            PAYLOAD_CAMERA_EO_NOISE_REDUCTION,
            static_cast<uint32_t>(original),
            actual))
        << "No readback after restoring EO Noise Reduction to "
        << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues,
    CameraEoNoiseReductionSettingValueTest,
    ::testing::Values(
        NoiseReductionCase{"Off",    PAYLOAD_CAMERA_EO_NOISE_REDUCTION_OFF},
        NoiseReductionCase{"Lv1",    PAYLOAD_CAMERA_EO_NOISE_REDUCTION_LV1},
        NoiseReductionCase{"Lv2",    PAYLOAD_CAMERA_EO_NOISE_REDUCTION_LV2},
        NoiseReductionCase{"Lv3",    PAYLOAD_CAMERA_EO_NOISE_REDUCTION_LV3},
        NoiseReductionCase{"Lv4",    PAYLOAD_CAMERA_EO_NOISE_REDUCTION_LV4},
        NoiseReductionCase{"Lv5",    PAYLOAD_CAMERA_EO_NOISE_REDUCTION_LV5},
        NoiseReductionCase{"Manual", PAYLOAD_CAMERA_EO_NOISE_REDUCTION_MANUAL}
    ),
    [](const ::testing::TestParamInfo<NoiseReductionCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_NOISE_REDUCTION

// =============================================================================
// 47. EO 2DNR Level C_V_NSREDUCT2D
// =============================================================================
// 2DNR level   C_V_NSREDUCT2D   -   x   -   -
#if defined(PAYLOAD_CAMERA_EO_NOISE_REDUCTION_2D) && defined(ORUSL)

using NoiseReduction2DCase = CameraEoParamValue;

class CameraEoNoiseReduction2DSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<NoiseReduction2DCase> {};

TEST_P(CameraEoNoiseReduction2DSettingValueTest, SetAndReadBack) {
    const NoiseReduction2DCase param = GetParam();

    // Save original Noise Reduction mode.
    double originalNoiseReduction = -1;
    if (!cet::readCameraParam(
            PAYLOAD_CAMERA_EO_NOISE_REDUCTION,
            originalNoiseReduction,
            3000)) {
        GTEST_SKIP() << "Payload did not report EO Noise Reduction.";
    }

    // Save original 2DNR value.
    double original = -1;
    if (!cet::readCameraParam(
            PAYLOAD_CAMERA_EO_NOISE_REDUCTION_2D,
            original,
            3000)) {
        GTEST_SKIP() << "Payload did not report EO 2DNR Level ("
                     << PAYLOAD_CAMERA_EO_NOISE_REDUCTION_2D
                     << ").";
    }

    double actual = -1;

    // 2DNR is applicable when Noise Reduction is MANUAL.
    ASSERT_TRUE(
        SetParamAndReadBack(
            PAYLOAD_CAMERA_EO_NOISE_REDUCTION,
            PAYLOAD_CAMERA_EO_NOISE_REDUCTION_MANUAL,
            actual))
        << "Could not set EO Noise Reduction to MANUAL.";

    // Set 2DNR level.
    ASSERT_TRUE(
        SetParamAndReadBack(
            PAYLOAD_CAMERA_EO_NOISE_REDUCTION_2D,
            param.value,
            actual))
        << "No readback after setting EO 2DNR Level to "
        << param << ".";

    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO 2DNR Level readback mismatch for "
        << param << ".";

    // Restore original 2DNR value.
    std::cout << "Restoring EO 2DNR Level to original value "
              << original << "." << std::endl;

    ASSERT_TRUE(
        SetParamAndReadBack(
            PAYLOAD_CAMERA_EO_NOISE_REDUCTION_2D,
            static_cast<uint32_t>(original),
            actual))
        << "No readback after restoring EO 2DNR Level to "
        << original << ".";

    // Restore original Noise Reduction mode.
    std::cout << "Restoring EO Noise Reduction to original value "
              << originalNoiseReduction << "." << std::endl;

    ASSERT_TRUE(
        SetParamAndReadBack(
            PAYLOAD_CAMERA_EO_NOISE_REDUCTION,
            static_cast<uint32_t>(originalNoiseReduction),
            actual))
        << "No readback after restoring EO Noise Reduction to "
        << originalNoiseReduction << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues,
    CameraEoNoiseReduction2DSettingValueTest,
    ::testing::Values(
        NoiseReduction2DCase{"Off", PAYLOAD_CAMERA_EO_NOISE_REDUCTION_2D_OFF},
        NoiseReduction2DCase{"Lv1", PAYLOAD_CAMERA_EO_NOISE_REDUCTION_2D_LV1},
        NoiseReduction2DCase{"Lv2", PAYLOAD_CAMERA_EO_NOISE_REDUCTION_2D_LV2},
        NoiseReduction2DCase{"Lv3", PAYLOAD_CAMERA_EO_NOISE_REDUCTION_2D_LV3},
        NoiseReduction2DCase{"Lv4", PAYLOAD_CAMERA_EO_NOISE_REDUCTION_2D_LV4},
        NoiseReduction2DCase{"Lv5", PAYLOAD_CAMERA_EO_NOISE_REDUCTION_2D_LV5}
    ),
    [](const ::testing::TestParamInfo<NoiseReduction2DCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_NOISE_REDUCTION_2D
// =============================================================================
// 48. EO 3DNR Level C_V_NSREDUCT3D
// =============================================================================
// 3DNR level   C_V_NSREDUCT3D   -   x   -   -
#if defined(PAYLOAD_CAMERA_EO_NOISE_REDUCTION_3D) && \
    defined(PAYLOAD_CAMERA_EO_NOISE_REDUCTION)  && defined(ORUSL)

using NoiseReduction3DCase = CameraEoParamValue;

class CameraEoNoiseReduction3DSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<NoiseReduction3DCase> {};

TEST_P(CameraEoNoiseReduction3DSettingValueTest, SetAndReadBack) {
    const NoiseReduction3DCase param = GetParam();

    // Save original Noise Reduction mode.
    double originalNoiseReduction = -1;
    if (!cet::readCameraParam(
            PAYLOAD_CAMERA_EO_NOISE_REDUCTION,
            originalNoiseReduction,
            3000)) {
        GTEST_SKIP() << "Payload did not report EO Noise Reduction.";
    }

    // Save original 3DNR value.
    double original = -1;
    if (!cet::readCameraParam(
            PAYLOAD_CAMERA_EO_NOISE_REDUCTION_3D,
            original,
            3000)) {
        GTEST_SKIP() << "Payload did not report EO 3DNR Level ("
                     << PAYLOAD_CAMERA_EO_NOISE_REDUCTION_3D
                     << ").";
    }

    double actual = -1;

    // 3DNR is applicable when Noise Reduction is MANUAL.
    ASSERT_TRUE(
        SetParamAndReadBack(
            PAYLOAD_CAMERA_EO_NOISE_REDUCTION,
            PAYLOAD_CAMERA_EO_NOISE_REDUCTION_MANUAL,
            actual))
        << "Could not set EO Noise Reduction to MANUAL.";

    // Set 3DNR level.
    ASSERT_TRUE(
        SetParamAndReadBack(
            PAYLOAD_CAMERA_EO_NOISE_REDUCTION_3D,
            param.value,
            actual))
        << "No readback after setting EO 3DNR Level to "
        << param << ".";

    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO 3DNR Level readback mismatch for "
        << param << ".";

    // Restore original 3DNR value.
    std::cout << "Restoring EO 3DNR Level to original value "
              << original << "." << std::endl;

    ASSERT_TRUE(
        SetParamAndReadBack(
            PAYLOAD_CAMERA_EO_NOISE_REDUCTION_3D,
            static_cast<uint32_t>(original),
            actual))
        << "No readback after restoring EO 3DNR Level to "
        << original << ".";

    // Restore original Noise Reduction mode.
    std::cout << "Restoring EO Noise Reduction to original value "
              << originalNoiseReduction << "." << std::endl;

    ASSERT_TRUE(
        SetParamAndReadBack(
            PAYLOAD_CAMERA_EO_NOISE_REDUCTION,
            static_cast<uint32_t>(originalNoiseReduction),
            actual))
        << "No readback after restoring EO Noise Reduction to "
        << originalNoiseReduction << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues,
    CameraEoNoiseReduction3DSettingValueTest,
    ::testing::Values(
        NoiseReduction3DCase{
            "Off",
            PAYLOAD_CAMERA_EO_NOISE_REDUCTION_3D_OFF
        },
        NoiseReduction3DCase{
            "Lv1",
            PAYLOAD_CAMERA_EO_NOISE_REDUCTION_3D_LV1
        },
        NoiseReduction3DCase{
            "Lv2",
            PAYLOAD_CAMERA_EO_NOISE_REDUCTION_3D_LV2
        },
        NoiseReduction3DCase{
            "Lv3",
            PAYLOAD_CAMERA_EO_NOISE_REDUCTION_3D_LV3
        },
        NoiseReduction3DCase{
            "Lv4",
            PAYLOAD_CAMERA_EO_NOISE_REDUCTION_3D_LV4
        },
        NoiseReduction3DCase{
            "Lv5",
            PAYLOAD_CAMERA_EO_NOISE_REDUCTION_3D_LV5
        }
    ),
    [](const ::testing::TestParamInfo<NoiseReduction3DCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_NOISE_REDUCTION_3D
// =============================================================================
// 49. EO R Gain Value C_V_RGAIN
// =============================================================================
// EO R Gain Value   C_V_RGAIN   x   x   -   x
#if defined(PAYLOAD_CAMERA_EO_R_GAIN) && (defined(VIO) || defined(ORUSL) || defined(ZIO))

using RGainCase = CameraEoParamValue;

class CameraEoRGainSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<RGainCase> {};

TEST_P(CameraEoRGainSettingValueTest, SetAndReadBack) {
    const RGainCase param = GetParam();

    double original = -1;
    if (!cet::readCameraParam(
            PAYLOAD_CAMERA_EO_R_GAIN,
            original,
            3000)) {
        GTEST_SKIP() << "Payload did not report EO R Gain Value ("
                     << PAYLOAD_CAMERA_EO_R_GAIN << ").";
    }

    double actual = -1;

    ASSERT_TRUE(
        SetParamAndReadBack(
            PAYLOAD_CAMERA_EO_R_GAIN,
            param.value,
            actual))
        << "No readback after setting EO R Gain Value to "
        << param << ".";

    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO R Gain Value readback mismatch for "
        << param << ".";

    // Restore original value.
    std::cout << "Restoring EO R Gain Value to original value "
              << original << "." << std::endl;

    ASSERT_TRUE(
        SetParamAndReadBack(
            PAYLOAD_CAMERA_EO_R_GAIN,
            static_cast<uint32_t>(original),
            actual))
        << "No readback after restoring EO R Gain Value to "
        << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues,
    CameraEoRGainSettingValueTest,
    ::testing::Values(
        RGainCase{"0",   0},
        RGainCase{"1",   1},
        RGainCase{"64",  64},
        RGainCase{"128", 128},
        RGainCase{"192", 192},
        RGainCase{"254", 254},
        RGainCase{"255", 255}
    ),
    [](const ::testing::TestParamInfo<RGainCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_R_GAIN
// =============================================================================
// 50. EO Shutter Value C_V_SP
// =============================================================================
// EO Shutter Value   C_V_SP   x   x   -   x
#if defined(PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED) && (defined(VIO) || defined(ORUSL) || defined(ZIO))

using ShutterSpeedCase = CameraEoParamValue;

class CameraEoShutterSpeedSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<ShutterSpeedCase> {};

TEST_P(CameraEoShutterSpeedSettingValueTest, SetAndReadBack) {
    const ShutterSpeedCase param = GetParam();

    double original = -1;
    if (!cet::readCameraParam(
            PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED,
            original,
            3000)) {
        GTEST_SKIP() << "Payload did not report EO Shutter Value ("
                     << PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED << ").";
    }

    double actual = -1;

    ASSERT_TRUE(
        SetParamAndReadBack(
            PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED,
            param.value,
            actual))
        << "No readback after setting EO Shutter Value to "
        << param << ".";

    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO Shutter Value readback mismatch for "
        << param << ".";

    // Restore original value.
    std::cout << "Restoring EO Shutter Value to original value "
              << original << "." << std::endl;

    ASSERT_TRUE(
        SetParamAndReadBack(
            PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED,
            static_cast<uint32_t>(original),
            actual))
        << "No readback after restoring EO Shutter Value to "
        << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues,
    CameraEoShutterSpeedSettingValueTest,
    ::testing::Values(
        ShutterSpeedCase{"1_1",     PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_1},
        ShutterSpeedCase{"2_3",     PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_2_3},
        ShutterSpeedCase{"1_2",     PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_2},
        ShutterSpeedCase{"1_3",     PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_3},
        ShutterSpeedCase{"1_4",     PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_4},
        ShutterSpeedCase{"1_6",     PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_6},
        ShutterSpeedCase{"1_8",     PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_8},
        ShutterSpeedCase{"1_10",    PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_10},
        ShutterSpeedCase{"1_15",    PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_15},
        ShutterSpeedCase{"1_20",    PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_20},
        ShutterSpeedCase{"1_30",    PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_30},
        ShutterSpeedCase{"1_50",    PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_50},
        ShutterSpeedCase{"1_60",    PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_60},
        ShutterSpeedCase{"1_90",    PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_90},
        ShutterSpeedCase{"1_100",   PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_100},
        ShutterSpeedCase{"1_125",   PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_125},
        ShutterSpeedCase{"1_180",   PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_180},
        ShutterSpeedCase{"1_250",   PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_250},
        ShutterSpeedCase{"1_350",   PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_350},
        ShutterSpeedCase{"1_500",   PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_500},
        ShutterSpeedCase{"1_725",   PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_725},
        ShutterSpeedCase{"1_1000",  PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_1000},
        ShutterSpeedCase{"1_1500",  PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_1500},
        ShutterSpeedCase{"1_2000",  PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_2000},
        ShutterSpeedCase{"1_3000",  PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_3000},
        ShutterSpeedCase{"1_4000",  PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_4000},
        ShutterSpeedCase{"1_6000",  PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_6000},
        ShutterSpeedCase{"1_10000", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED_1_10000}
    ),
    [](const ::testing::TestParamInfo<ShutterSpeedCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED
// =============================================================================
// 51. EO Spot Light Avoidance C_V_SPAVOID
// =============================================================================
// EO Spot Light Avoidance   C_V_SPAVOID   -   x   -   -
#if defined(PAYLOAD_CAMERA_EO_SPOT_LIGHT_AVOIDANCE) && defined(ORUSL)

using SpotLightAvoidanceCase = CameraEoParamValue;

class CameraEoSpotLightAvoidanceSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<SpotLightAvoidanceCase> {};

TEST_P(CameraEoSpotLightAvoidanceSettingValueTest, SetAndReadBack) {
    const SpotLightAvoidanceCase param = GetParam();

    double original = -1;
    if (!cet::readCameraParam(
            PAYLOAD_CAMERA_EO_SPOT_LIGHT_AVOIDANCE,
            original,
            3000)) {
        GTEST_SKIP() << "Payload did not report EO Spot Light Avoidance ("
                     << PAYLOAD_CAMERA_EO_SPOT_LIGHT_AVOIDANCE << ").";
    }

    double actual = -1;

    ASSERT_TRUE(
        SetParamAndReadBack(
            PAYLOAD_CAMERA_EO_SPOT_LIGHT_AVOIDANCE,
            param.value,
            actual))
        << "No readback after setting EO Spot Light Avoidance to "
        << param << ".";

    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO Spot Light Avoidance readback mismatch for "
        << param << ".";

    // Restore original value.
    std::cout << "Restoring EO Spot Light Avoidance to original value "
              << original << "." << std::endl;

    ASSERT_TRUE(
        SetParamAndReadBack(
            PAYLOAD_CAMERA_EO_SPOT_LIGHT_AVOIDANCE,
            static_cast<uint32_t>(original),
            actual))
        << "No readback after restoring EO Spot Light Avoidance to "
        << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues,
    CameraEoSpotLightAvoidanceSettingValueTest,
    ::testing::Values(
        SpotLightAvoidanceCase{
            "On",
            PAYLOAD_CAMERA_EO_SPOT_LIGHT_AVOIDANCE_ON
        },
        SpotLightAvoidanceCase{
            "Off",
            PAYLOAD_CAMERA_EO_SPOT_LIGHT_AVOIDANCE_OFF
        }
    ),
    [](const ::testing::TestParamInfo<SpotLightAvoidanceCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_SPOT_LIGHT_AVOIDANCE
// =============================================================================
// 52. EO Stable Zoom C_V_STZOOM
// =============================================================================
// EO Stable Zoom   C_V_STZOOM   -   x   -   -
#if defined(PAYLOAD_CAMERA_EO_STABLE_ZOOM) && defined(ORUSL)

using StableZoomCase = CameraEoParamValue;

class CameraEoStableZoomSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<StableZoomCase> {};

TEST_P(CameraEoStableZoomSettingValueTest, SetAndReadBack) {
    const StableZoomCase param = GetParam();

    double original = -1;
    if (!cet::readCameraParam(
            PAYLOAD_CAMERA_EO_STABLE_ZOOM,
            original,
            3000)) {
        GTEST_SKIP() << "Payload did not report EO Stable Zoom ("
                     << PAYLOAD_CAMERA_EO_STABLE_ZOOM << ").";
    }

    double actual = -1;

    ASSERT_TRUE(
        SetParamAndReadBack(
            PAYLOAD_CAMERA_EO_STABLE_ZOOM,
            param.value,
            actual))
        << "No readback after setting EO Stable Zoom to "
        << param << ".";

    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "EO Stable Zoom readback mismatch for "
        << param << ".";

    // Restore original value.
    std::cout << "Restoring EO Stable Zoom to original value "
              << original << "." << std::endl;

    ASSERT_TRUE(
        SetParamAndReadBack(
            PAYLOAD_CAMERA_EO_STABLE_ZOOM,
            static_cast<uint32_t>(original),
            actual))
        << "No readback after restoring EO Stable Zoom to "
        << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues,
    CameraEoStableZoomSettingValueTest,
    ::testing::Values(
        StableZoomCase{
            "Off",
            PAYLOAD_CAMERA_EO_STABLE_ZOOM_OFF
        },
        StableZoomCase{
            "On",
            PAYLOAD_CAMERA_EO_STABLE_ZOOM_ON
        }
    ),
    [](const ::testing::TestParamInfo<StableZoomCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_EO_STABLE_ZOOM
// ============================================================
// 53. EO WB Mode
// Parameter: C_V_WB
// Values: Auto / Indoor / Outdoor / One Push / ATW / Manual
// ============================================================
#if defined(PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE) && (defined(VIO) || defined(ORUSL) || defined(ZIO))

using CameraEoWbModeCase = CameraEoParamValue;

class CameraEoWbModeSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<CameraEoWbModeCase> {};

TEST_P(CameraEoWbModeSettingValueTest, SetAndReadBack)
{
    const CameraEoWbModeCase param = GetParam();

    double original = -1;
    if (!cet::readCameraParam(
            PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE,
            original,
            3000)) {
        GTEST_SKIP()
            << "Parameter " << PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE
            << " is not readable on this payload.";
    }

    double actual = -1;

    ASSERT_TRUE(SetParamAndReadBack(
        PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE,
        param.value,
        actual))
        << "Failed to set "
        << PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE
        << " to " << param.value
        << " (" << param.name << ")";

    EXPECT_EQ(static_cast<uint32_t>(actual),
              param.value)
        << "Unexpected WB mode after setting "
        << param.name;

    // Restore original value.
    std::cout
        << "Restoring "
        << PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE
        << " to " << original
        << std::endl;

    ASSERT_TRUE(SetParamAndReadBack(
        PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE,
        static_cast<uint32_t>(original),
        actual))
        << "Failed to restore "
        << PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE
        << " to " << original;
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues,
    CameraEoWbModeSettingValueTest,
    ::testing::Values(
        CameraEoWbModeCase{
            "Auto",
            PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE_AUTO},

        CameraEoWbModeCase{
            "Indoor",
            PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE_INDOOR},

        CameraEoWbModeCase{
            "Outdoor",
            PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE_OUTDOOR},

        CameraEoWbModeCase{
            "OnePush",
            PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE_ONE_PUSH},

        CameraEoWbModeCase{
            "ATW",
            PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE_ATW},

        CameraEoWbModeCase{
            "Manual",
            PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE_MANUAL}),
    [](const ::testing::TestParamInfo<CameraEoWbModeCase>& info) {
        return info.param.name;
    });

#endif

// ============================================================
// 54. EO Zoom Level - Combine Zoom
// Parameter: C_V_ZM_CB_LV
// Values: 1x ~ 240x
// ============================================================
#if defined(PAYLOAD_CAMERA_VIDEO_ZOOM_COMBINE_FACTOR) && (defined(VIO) || defined(ORUSL) || defined(ZIO))

using CameraEoZoomCombineLevelCase = CameraEoParamValue;

class CameraEoZoomCombineLevelSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<CameraEoZoomCombineLevelCase> {};

TEST_P(CameraEoZoomCombineLevelSettingValueTest, SetAndReadBack)
{
    const CameraEoZoomCombineLevelCase param = GetParam();

    double original = -1;
    if (!cet::readCameraParam(
            PAYLOAD_CAMERA_VIDEO_ZOOM_COMBINE_FACTOR,
            original,
            3000)) {
        GTEST_SKIP()
            << "Parameter "
            << PAYLOAD_CAMERA_VIDEO_ZOOM_COMBINE_FACTOR
            << " is not readable on this payload.";
    }

    double actual = -1;

    ASSERT_TRUE(SetParamAndReadBack(
        PAYLOAD_CAMERA_VIDEO_ZOOM_COMBINE_FACTOR,
        param.value,
        actual))
        << "Failed to set "
        << PAYLOAD_CAMERA_VIDEO_ZOOM_COMBINE_FACTOR
        << " to " << param.value
        << " (" << param.name << ")";

    EXPECT_EQ(static_cast<uint32_t>(actual),
              param.value)
        << "Unexpected combine zoom level after setting "
        << param.name;

    // Restore original value.
    std::cout
        << "Restoring "
        << PAYLOAD_CAMERA_VIDEO_ZOOM_COMBINE_FACTOR
        << " to " << original
        << std::endl;

    ASSERT_TRUE(SetParamAndReadBack(
        PAYLOAD_CAMERA_VIDEO_ZOOM_COMBINE_FACTOR,
        static_cast<uint32_t>(original),
        actual))
        << "Failed to restore "
        << PAYLOAD_CAMERA_VIDEO_ZOOM_COMBINE_FACTOR
        << " to " << original;
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues,
    CameraEoZoomCombineLevelSettingValueTest,
    ::testing::Values(
        CameraEoZoomCombineLevelCase{"1X",  ZOOM_COMBINE_1X},
        CameraEoZoomCombineLevelCase{"2X",  ZOOM_COMBINE_2X},
        CameraEoZoomCombineLevelCase{"4X",  ZOOM_COMBINE_4X},
        CameraEoZoomCombineLevelCase{"6X",  ZOOM_COMBINE_6X},
        CameraEoZoomCombineLevelCase{"8X",  ZOOM_COMBINE_8X},
        CameraEoZoomCombineLevelCase{"10X", ZOOM_COMBINE_10X},
        CameraEoZoomCombineLevelCase{"12X", ZOOM_COMBINE_12X},
        CameraEoZoomCombineLevelCase{"14X", ZOOM_COMBINE_14X},
        CameraEoZoomCombineLevelCase{"16X", ZOOM_COMBINE_16X},
        CameraEoZoomCombineLevelCase{"18X", ZOOM_COMBINE_18X},
        CameraEoZoomCombineLevelCase{"20X", ZOOM_COMBINE_20X},
        CameraEoZoomCombineLevelCase{"40X", ZOOM_COMBINE_40X},
        CameraEoZoomCombineLevelCase{"60X", ZOOM_COMBINE_60X},
        CameraEoZoomCombineLevelCase{"80X", ZOOM_COMBINE_80X},
        CameraEoZoomCombineLevelCase{"100X", ZOOM_COMBINE_100X},
        CameraEoZoomCombineLevelCase{"120X", ZOOM_COMBINE_120X},
        CameraEoZoomCombineLevelCase{"140X", ZOOM_COMBINE_140X},
        CameraEoZoomCombineLevelCase{"160X", ZOOM_COMBINE_160X},
        CameraEoZoomCombineLevelCase{"180X", ZOOM_COMBINE_180X},
        CameraEoZoomCombineLevelCase{"200X", ZOOM_COMBINE_200X},
        CameraEoZoomCombineLevelCase{"220X", ZOOM_COMBINE_220X},
        CameraEoZoomCombineLevelCase{"240X", ZOOM_COMBINE_240X}),
    [](const ::testing::TestParamInfo<CameraEoZoomCombineLevelCase>& info) {
        return info.param.name;
    });

#endif

// ============================================================
// 55. EO dZoom Mode
// Parameter: C_V_ZM_MODE
// Values: Combine / Super Resolution
// ============================================================
#if defined(PAYLOAD_CAMERA_VIDEO_ZOOM_MODE) && (defined(VIO) || defined(ORUSL) || defined(ZIO))

using CameraEoZoomModeCase = CameraEoParamValue;

class CameraEoZoomModeSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<CameraEoZoomModeCase> {};

TEST_P(CameraEoZoomModeSettingValueTest, SetAndReadBack)
{
    const CameraEoZoomModeCase param = GetParam();

    double original = -1;
    if (!cet::readCameraParam(
            PAYLOAD_CAMERA_VIDEO_ZOOM_MODE,
            original,
            3000)) {
        GTEST_SKIP()
            << "Parameter "
            << PAYLOAD_CAMERA_VIDEO_ZOOM_MODE
            << " is not readable on this payload.";
    }

    double actual = -1;

    ASSERT_TRUE(SetParamAndReadBack(
        PAYLOAD_CAMERA_VIDEO_ZOOM_MODE,
        param.value,
        actual))
        << "Failed to set "
        << PAYLOAD_CAMERA_VIDEO_ZOOM_MODE
        << " to " << param.value
        << " (" << param.name << ")";

    EXPECT_EQ(static_cast<uint32_t>(actual),
              param.value)
        << "Unexpected zoom mode after setting "
        << param.name;

    // Restore original value.
    std::cout
        << "Restoring "
        << PAYLOAD_CAMERA_VIDEO_ZOOM_MODE
        << " to " << original
        << std::endl;

    ASSERT_TRUE(SetParamAndReadBack(
        PAYLOAD_CAMERA_VIDEO_ZOOM_MODE,
        static_cast<uint32_t>(original),
        actual))
        << "Failed to restore "
        << PAYLOAD_CAMERA_VIDEO_ZOOM_MODE
        << " to " << original;
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues,
    CameraEoZoomModeSettingValueTest,
    ::testing::Values(
        CameraEoZoomModeCase{
            "Combine",
            PAYLOAD_CAMERA_VIDEO_ZOOM_MODE_COMBINE},

        CameraEoZoomModeCase{
            "SuperResolution",
            PAYLOAD_CAMERA_VIDEO_ZOOM_MODE_SUPER_RESOLUTION}),
    [](const ::testing::TestParamInfo<CameraEoZoomModeCase>& info) {
        return info.param.name;
    });

#endif
// ============================================================
// 56. EO Zoom Level - Super Resolution
// Parameter: C_V_ZM_SR_LV
// Values: 1x ~ 30x
// ============================================================
#if defined(PAYLOAD_CAMERA_VIDEO_ZOOM_SUPER_RESOLUTION_FACTOR) && (defined(VIO) || defined(ORUSL) || defined(ZIO))

using CameraEoZoomSuperResolutionLevelCase = CameraEoParamValue;

class CameraEoZoomSuperResolutionLevelSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<
          CameraEoZoomSuperResolutionLevelCase> {};

TEST_P(CameraEoZoomSuperResolutionLevelSettingValueTest, SetAndReadBack)
{
    const CameraEoZoomSuperResolutionLevelCase param = GetParam();

    double original = -1;
    if (!cet::readCameraParam(
            PAYLOAD_CAMERA_VIDEO_ZOOM_SUPER_RESOLUTION_FACTOR,
            original,
            3000)) {
        GTEST_SKIP()
            << "Parameter "
            << PAYLOAD_CAMERA_VIDEO_ZOOM_SUPER_RESOLUTION_FACTOR
            << " is not readable on this payload.";
    }

    double actual = -1;

    ASSERT_TRUE(SetParamAndReadBack(
        PAYLOAD_CAMERA_VIDEO_ZOOM_SUPER_RESOLUTION_FACTOR,
        param.value,
        actual))
        << "Failed to set "
        << PAYLOAD_CAMERA_VIDEO_ZOOM_SUPER_RESOLUTION_FACTOR
        << " to " << param.value
        << " (" << param.name << ")";

    EXPECT_EQ(static_cast<uint32_t>(actual),
              param.value)
        << "Unexpected super-resolution zoom level after setting "
        << param.name;

    // Restore original value.
    std::cout
        << "Restoring "
        << PAYLOAD_CAMERA_VIDEO_ZOOM_SUPER_RESOLUTION_FACTOR
        << " to " << original
        << std::endl;

    ASSERT_TRUE(SetParamAndReadBack(
        PAYLOAD_CAMERA_VIDEO_ZOOM_SUPER_RESOLUTION_FACTOR,
        static_cast<uint32_t>(original),
        actual))
        << "Failed to restore "
        << PAYLOAD_CAMERA_VIDEO_ZOOM_SUPER_RESOLUTION_FACTOR
        << " to " << original;
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues,
    CameraEoZoomSuperResolutionLevelSettingValueTest,
    ::testing::Values(
        CameraEoZoomSuperResolutionLevelCase{
            "1X",
            ZOOM_SUPER_RESOLUTION_1X},

        CameraEoZoomSuperResolutionLevelCase{
            "2X",
            ZOOM_SUPER_RESOLUTION_2X},

        CameraEoZoomSuperResolutionLevelCase{
            "4X",
            ZOOM_SUPER_RESOLUTION_4X},

        CameraEoZoomSuperResolutionLevelCase{
            "6X",
            ZOOM_SUPER_RESOLUTION_6X},

        CameraEoZoomSuperResolutionLevelCase{
            "8X",
            ZOOM_SUPER_RESOLUTION_8X},

        CameraEoZoomSuperResolutionLevelCase{
            "10X",
            ZOOM_SUPER_RESOLUTION_10X},

        CameraEoZoomSuperResolutionLevelCase{
            "12X",
            ZOOM_SUPER_RESOLUTION_12X},

        CameraEoZoomSuperResolutionLevelCase{
            "14X",
            ZOOM_SUPER_RESOLUTION_14X},

        CameraEoZoomSuperResolutionLevelCase{
            "16X",
            ZOOM_SUPER_RESOLUTION_16X},

        CameraEoZoomSuperResolutionLevelCase{
            "18X",
            ZOOM_SUPER_RESOLUTION_18X},

        CameraEoZoomSuperResolutionLevelCase{
            "20X",
            ZOOM_SUPER_RESOLUTION_20X},

        CameraEoZoomSuperResolutionLevelCase{
            "30X",
            ZOOM_SUPER_RESOLUTION_30X}),
    [](const ::testing::TestParamInfo<
           CameraEoZoomSuperResolutionLevelCase>& info) {
        return info.param.name;
    });

#endif

// ============================================================
// 57. EO Zoom
// Parameter: C_V_ZOOM
// Values: 1x ~ 12x
// ============================================================
#if defined(PAYLOAD_CAMERA_VIDEO_ZOOM_FACTOR) && defined(MB1)

using CameraEoZoomFactorCase = CameraEoParamValue;

class CameraEoZoomFactorSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<CameraEoZoomFactorCase> {};

TEST_P(CameraEoZoomFactorSettingValueTest, SetAndReadBack)
{
    const CameraEoZoomFactorCase param = GetParam();

    double original = -1;
    if (!cet::readCameraParam(
            PAYLOAD_CAMERA_VIDEO_ZOOM_FACTOR,
            original,
            3000)) {
        GTEST_SKIP()
            << "Parameter "
            << PAYLOAD_CAMERA_VIDEO_ZOOM_FACTOR
            << " is not readable on this payload.";
    }

    double actual = -1;

    ASSERT_TRUE(SetParamAndReadBack(
        PAYLOAD_CAMERA_VIDEO_ZOOM_FACTOR,
        param.value,
        actual))
        << "Failed to set "
        << PAYLOAD_CAMERA_VIDEO_ZOOM_FACTOR
        << " to " << param.value
        << " (" << param.name << ")";

    EXPECT_EQ(static_cast<uint32_t>(actual),
              param.value)
        << "Unexpected EO zoom factor after setting "
        << param.name;
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    // Restore original value.
    std::cout
        << "Restoring "
        << PAYLOAD_CAMERA_VIDEO_ZOOM_FACTOR
        << " to " << original
        << std::endl;

    ASSERT_TRUE(SetParamAndReadBack(
        PAYLOAD_CAMERA_VIDEO_ZOOM_FACTOR,
        static_cast<uint32_t>(original),
        actual))
        << "Failed to restore "
        << PAYLOAD_CAMERA_VIDEO_ZOOM_FACTOR
        << " to " << original;
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues,
    CameraEoZoomFactorSettingValueTest,
    ::testing::Values(
        CameraEoZoomFactorCase{"1X",  ZOOM_EO_1X},
        CameraEoZoomFactorCase{"2X",  ZOOM_EO_2X},
        CameraEoZoomFactorCase{"3X",  ZOOM_EO_3X},
        CameraEoZoomFactorCase{"4X",  ZOOM_EO_4X},
        CameraEoZoomFactorCase{"5X",  ZOOM_EO_5X},
        CameraEoZoomFactorCase{"6X",  ZOOM_EO_6X},
        CameraEoZoomFactorCase{"7X",  ZOOM_EO_7X},
        CameraEoZoomFactorCase{"8X",  ZOOM_EO_8X},
        CameraEoZoomFactorCase{"9X",  ZOOM_EO_9X},
        CameraEoZoomFactorCase{"10X", ZOOM_EO_10X},
        CameraEoZoomFactorCase{"11X", ZOOM_EO_11X},
        CameraEoZoomFactorCase{"12X", ZOOM_EO_12X}),
    [](const ::testing::TestParamInfo<CameraEoZoomFactorCase>& info) {
        return info.param.name;
    });

#endif
// ============================================================
// 58. EO Zoom Speed
// Parameter: C_V_ZOOM_SPEED
// Value range: Not documented in current definition
// ============================================================
// #if defined(PAYLOAD_CAMERA_EO_ZOOM_SPEED)

// TEST_F(CameraEoParamSettingTest, ZoomSpeed_IsReadable)
// {
//     double actual = -1;

//     if (!cet::readCameraParam(
//             PAYLOAD_CAMERA_EO_ZOOM_SPEED,
//             actual,
//             3000)) {
//         GTEST_SKIP()
//             << "Parameter "
//             << PAYLOAD_CAMERA_EO_ZOOM_SPEED
//             << " is not readable on this payload.";
//     }

//     EXPECT_GE(actual, 0.0)
//         << "Unexpected negative value for "
//         << PAYLOAD_CAMERA_EO_ZOOM_SPEED;

//     std::cout
//         << PAYLOAD_CAMERA_EO_ZOOM_SPEED
//         << " = "
//         << actual
//         << std::endl;
// }
// ============================================================
// 59. EO Zoom Speed
// Parameter: C_V_Z_SPD
// Values: 0 ~ 7, step 1
// ============================================================
#if defined(PAYLOAD_CAMERA_EO_ZOOM_SPEED_LEVEL) && (defined(VIO) || defined(ORUSL))

using CameraEoZoomSpeedCase = CameraEoParamValue;

class CameraEoZoomSpeedSettingValueTest
    : public CameraEoParamSettingTest,
      public ::testing::WithParamInterface<CameraEoZoomSpeedCase> {};

TEST_P(CameraEoZoomSpeedSettingValueTest, SetAndReadBack)
{
    const CameraEoZoomSpeedCase param = GetParam();

    double original = -1;
    if (!cet::readCameraParam(
            PAYLOAD_CAMERA_EO_ZOOM_SPEED_LEVEL,
            original,
            3000)) {
        GTEST_SKIP()
            << "Parameter "
            << PAYLOAD_CAMERA_EO_ZOOM_SPEED_LEVEL
            << " is not readable on this payload.";
    }

    double actual = -1;

    ASSERT_TRUE(SetParamAndReadBack(
        PAYLOAD_CAMERA_EO_ZOOM_SPEED_LEVEL,
        param.value,
        actual))
        << "Failed to set "
        << PAYLOAD_CAMERA_EO_ZOOM_SPEED_LEVEL
        << " to " << param.value
        << " (" << param.name << ")";

    EXPECT_EQ(static_cast<uint32_t>(actual),
              param.value)
        << "Unexpected EO zoom speed after setting "
        << param.name;

    // Restore original value.
    std::cout
        << "Restoring "
        << PAYLOAD_CAMERA_EO_ZOOM_SPEED_LEVEL
        << " to " << original
        << std::endl;

    ASSERT_TRUE(SetParamAndReadBack(
        PAYLOAD_CAMERA_EO_ZOOM_SPEED_LEVEL,
        static_cast<uint32_t>(original),
        actual))
        << "Failed to restore "
        << PAYLOAD_CAMERA_EO_ZOOM_SPEED_LEVEL
        << " to " << original;
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues,
    CameraEoZoomSpeedSettingValueTest,
    ::testing::Values(
        CameraEoZoomSpeedCase{"Speed0", 0},
        CameraEoZoomSpeedCase{"Speed1", 1},
        CameraEoZoomSpeedCase{"Speed2", 2},
        CameraEoZoomSpeedCase{"Speed3", 3},
        CameraEoZoomSpeedCase{"Speed4", 4},
        CameraEoZoomSpeedCase{"Speed5", 5},
        CameraEoZoomSpeedCase{"Speed6", 6},
        CameraEoZoomSpeedCase{"Speed7", 7}),
    [](const ::testing::TestParamInfo<CameraEoZoomSpeedCase>& info) {
        return info.param.name;
    });

#endif


// Manual test

// TEST_F(ManualCameraEoParamSettingTest, ManualSetCameraFocus) {
//     g_payload->setCameraFocus(FOCUS_TYPE_AUTO);
// }
// TEST_F(ManualCameraEoParamSettingTest, ManualSetCameraFocusEoFocusMode) {
//     g_payload->setPayloadCameraParam(PAYLOAD_CAMERA_VIDEO_FOCUS_MODE, PAYLOAD_CAMERA_VIDEO_FOCUS_MODE_MANUAL, PARAM_TYPE_UINT32);
// }
// TEST_F(ManualCameraEoParamSettingTest, ManualGetSingleParamValuesById) {
// char paramId[] = "C_SOURCE";

//     const uint64_t seq = g_cb.paramSeq.load();
//     g_payload->getPayloadCameraSettingByID(paramId);

//     ASSERT_TRUE(waitForSeq(g_cb.paramSeq, seq, 10000))
//         << "No response received for " << paramId;

//     std::lock_guard<std::mutex> lock(g_cb.m);

//     ASSERT_EQ(g_cb.lastParamId, "C_SOURCE");

//     const double value = g_cb.paramValues[1];

//     std::cout << "--> Param_id: " << g_cb.lastParamId
//               << ", value: " << value << std::endl;
// }

// TEST_F(ManualCameraEoParamSettingTest, ManualGetSomeParamValuesById) {

//     const std::vector<std::string> paramIds = {
//         "C_V_FM",
//         "C_V_FV",
//         "C_V_FV_SPD"
//         // Add more parameter IDs here
//     };

//     for (const auto& paramIdStr : paramIds) {

//         char paramId[64];
//         std::strncpy(paramId, paramIdStr.c_str(), sizeof(paramId));
//         paramId[sizeof(paramId) - 1] = '\0';

//         const uint64_t seq = g_cb.paramSeq.load();

//         g_payload->getPayloadCameraSettingByID(paramId);

//         ASSERT_TRUE(waitForSeq(g_cb.paramSeq, seq, 10000))
//             << "No response received for " << paramIdStr;

//         std::lock_guard<std::mutex> lock(g_cb.m);

//         ASSERT_EQ(g_cb.lastParamId, paramIdStr)
//             << "Returned parameter ID does not match requested ID";

//         const double value = g_cb.paramValues[1];

//         std::cout << "--> Param_id: "
//                   << g_cb.lastParamId
//                   << ", value: "
//                   << value
//                   << std::endl;
//     }
// }
// TEST_F(ManualCameraEoParamSettingTest, ManualSetVideoView) {
//     g_payload->sdkInitConnection();
//     g_payload->checkPayloadConnection();
//     g_payload->setPayloadCameraParam(PAYLOAD_CAMERA_VIEW_SRC, PAYLOAD_CAMERA_VIEW_EO, PARAM_TYPE_UINT32);
//     std::this_thread::sleep_for(std::chrono::milliseconds(150));
// }