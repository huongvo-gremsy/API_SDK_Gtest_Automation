#include "api_camera_ir_test_helper.h"

#include <cstdint>
#include <iomanip>
#include <ostream>
#include <set>
#include <thread>

namespace cit = camera_ir_test;

// ---------------------------------------------------------------------------
// Shared param-case type used by every "documented value" sweep below.
// ---------------------------------------------------------------------------
struct CameraIrParamValue {
    const char* name;
    uint32_t value;
};

inline std::ostream& operator<<(std::ostream& os,
                                 const CameraIrParamValue& value) {
    return os << value.name << "(" << value.value << ")";
}
#define CAMERA_F1 0
// ---------------------------------------------------------------------------
// Lightweight fixture for parameter tests. CameraIrTest is intentionally not
// used here because its setup/teardown polls capture status while stopping
// active image/video operations.
// ---------------------------------------------------------------------------
class CameraIrParamSettingTest : public PayloadTest {
    protected:
    void SetUp() override {
        ASSERT_TRUE(cit::readCameraParam(PAYLOAD_CAMERA_VIEW_SRC, originalView_))
            << "Could not read the current camera view source before the test.";
        haveView_ = true;
 
        ASSERT_TRUE(cit::setCameraParam(PAYLOAD_CAMERA_VIEW_SRC,
                                         PAYLOAD_CAMERA_VIEW_IR))
            << "Could not switch the camera view to IR before the test.";
    }
 
    void TearDown() override {
        if (haveView_) {
            EXPECT_TRUE(cit::setCameraParam(
                PAYLOAD_CAMERA_VIEW_SRC, static_cast<uint32_t>(originalView_)))
                << "Could not restore the camera view source after the test.";
        }
    }
 
    double originalView_ = 0;
    bool haveView_ = false;
};
namespace {

bool SetParamAndReadBack(const char* paramId, uint32_t value, double& actual) {
    if (!cit::setCameraParam(paramId, value, 4000)) {
        return false;
    }

    if (!cit::readCameraParam(paramId, actual, 3000)) {
        return false;
    }

    std::cout << "--> Param_id: " << paramId
              << ", value: " << std::fixed << std::setprecision(2)
              << actual << std::endl;
    return true;
}

}  // namespace

// =============================================================================
// 0. Adv Ir Image Enhance
// =============================================================================
// Adv Ir Image Enhance	ADV_IMG_ENHANCE	-	-	?	-	?	?

// =============================================================================
// 1. IR AGC Linear Percent
// =============================================================================
// C_T_AGC_LNP	-	-	x	-	-	x
#if defined(CAMERA_F1) && defined(PAYLOAD_CAMERA_IR_AGC_LINEAR_PERCENT) && defined(MB1)

using IrAGCLinearPercentCase = CameraIrParamValue;   // was CameraEoParamValue

class CameraIrAGClinearPercentSettingValueTest
    : public CameraIrParamSettingTest,               // was CameraEoParamSettingTest
      public ::testing::WithParamInterface<IrAGCLinearPercentCase> {};

TEST_P(CameraIrAGClinearPercentSettingValueTest, SetAndReadBack) {
    const IrAGCLinearPercentCase param = GetParam();
    double original = -1;
    if (!cit::readCameraParam(PAYLOAD_CAMERA_IR_AGC_LINEAR_PERCENT, original, 3000)) {
        GTEST_SKIP() << "Payload did not report IR AGC Linear Percent ("
                     << PAYLOAD_CAMERA_IR_AGC_LINEAR_PERCENT << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_IR_AGC_LINEAR_PERCENT,
                                     param.value, actual))
        << "No readback after setting IR AGC Linear Percent to " << param
        << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "IR AGC Linear Percent readback mismatch for " << param << ".";

    // Restore original value
    std::cout << "Restoring IR AGC Linear Percent to original value "
              << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_IR_AGC_LINEAR_PERCENT,
                                     static_cast<uint32_t>(original), actual))
        << "No readback after restoring IR AGC Linear Percent to "
        << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraIrAGClinearPercentSettingValueTest,
    ::testing::Values(IrAGCLinearPercentCase{"Percent0", 0},
                       IrAGCLinearPercentCase{"Percent10", 10},
                       IrAGCLinearPercentCase{"Percent20", 20},
                       IrAGCLinearPercentCase{"Percent30", 30},
                       IrAGCLinearPercentCase{"Percent40", 40},
                       IrAGCLinearPercentCase{"Percent50", 50},
                       IrAGCLinearPercentCase{"Percent60", 60},
                       IrAGCLinearPercentCase{"Percent70", 70},
                       IrAGCLinearPercentCase{"Percent80", 80},
                       IrAGCLinearPercentCase{"Percent90", 90},
                       IrAGCLinearPercentCase{"Percent100", 100},
                       IrAGCLinearPercentCase{"Percent101", 101}),
    [](const ::testing::TestParamInfo<IrAGCLinearPercentCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_IR_AGC_LINEAR_PERCENT

// =============================================================================
// 2. IR AGC Mode PAYLOAD_CAMERA_IR_AGC_MODE
// =============================================================================
// IR AGC Mode	C_T_AGC_M	-	-	x	-	-	x
#if defined(CAMERA_F1) && defined(PAYLOAD_CAMERA_IR_AGC_MODE) && defined(MB1)

using IrAGCModeCase = CameraIrParamValue;   // was CameraEoParamValue

class CameraIrAGCModeSettingValueTest
    : public CameraIrParamSettingTest,               // was CameraEoParamSettingTest
      public ::testing::WithParamInterface<IrAGCModeCase> {};

TEST_P(CameraIrAGCModeSettingValueTest, SetAndReadBack) {
    const IrAGCModeCase param = GetParam();
    double original = -1;
    if (!cit::readCameraParam(PAYLOAD_CAMERA_IR_AGC_MODE, original, 3000)) {
        GTEST_SKIP() << "Payload did not report IR AGC Mode ("
                     << PAYLOAD_CAMERA_IR_AGC_MODE << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_IR_AGC_MODE,
                                     param.value, actual))
        << "No readback after setting IR AGC Mode to " << param
        << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "IR AGC Mode readback mismatch for " << param << ".";

    // Restore original value
    std::cout << "Restoring IR AGC Mode to original value "
              << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_IR_AGC_MODE,
                                     static_cast<uint32_t>(original), actual))
        << "No readback after restoring IR AGC Mode to "
        << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraIrAGCModeSettingValueTest,
    ::testing::Values(IrAGCModeCase{"PAYLOAD_CAMERA_IR_AGC_MODE_NORMAL", PAYLOAD_CAMERA_IR_AGC_MODE_NORMAL},
                       IrAGCModeCase{"PAYLOAD_CAMERA_IR_AGC_MODE_HOLD", PAYLOAD_CAMERA_IR_AGC_MODE_HOLD},
                       IrAGCModeCase{"PAYLOAD_CAMERA_IR_AGC_MODE_THRESHOLD", PAYLOAD_CAMERA_IR_AGC_MODE_THRESHOLD},
                       IrAGCModeCase{"PAYLOAD_CAMERA_IR_AGC_MODE_BRIGHT", PAYLOAD_CAMERA_IR_AGC_MODE_BRIGHT},
                       IrAGCModeCase{"PAYLOAD_CAMERA_IR_AGC_MODE_LINEAR", PAYLOAD_CAMERA_IR_AGC_MODE_LINEAR},
                       IrAGCModeCase{"PAYLOAD_CAMERA_IR_AGC_MODE_MANUAL", PAYLOAD_CAMERA_IR_AGC_MODE_MANUAL}),
    [](const ::testing::TestParamInfo<IrAGCModeCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_IR_AGC_MODE

// =============================================================================
// 3. IR Contrast Mode	PAYLOAD_CAMERA_IR_CONTRAST_MODE
// =============================================================================
// IR Contrast Mode	C_T_CONST_M	-	-	x	-	-	x
#if defined(CAMERA_F1) && defined(PAYLOAD_CAMERA_IR_CONTRAST_MODE) && defined(MB1)

using IrContrastModeCase = CameraIrParamValue;   // was CameraEoParamValue

class CameraIrContrastModeSettingValueTest
    : public CameraIrParamSettingTest,               // was CameraEoParamSettingTest
      public ::testing::WithParamInterface<IrContrastModeCase> {};

TEST_P(CameraIrContrastModeSettingValueTest, SetAndReadBack) {
    const IrContrastModeCase param = GetParam();
    double original = -1;
    if (!cit::readCameraParam(PAYLOAD_CAMERA_IR_CONTRAST_MODE, original, 3000)) {
        GTEST_SKIP() << "Payload did not report IR Contrast Mode ("
                     << PAYLOAD_CAMERA_IR_CONTRAST_MODE << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_IR_CONTRAST_MODE,
                                     param.value, actual))
        << "No readback after setting IR Contrast Mode to " << param
        << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "IR Contrast Mode readback mismatch for " << param << ".";

    // Restore original value
    std::cout << "Restoring IR Contrast Mode to original value "
              << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_IR_CONTRAST_MODE,
                                     static_cast<uint32_t>(original), actual))
        << "No readback after restoring IR Contrast Mode to "
        << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraIrContrastModeSettingValueTest,
    ::testing::Values(IrContrastModeCase{"PAYLOAD_CAMERA_IR_CONTRAST_MODE_DEFAULT", PAYLOAD_CAMERA_IR_CONTRAST_MODE_DEFAULT},
                       IrContrastModeCase{"PAYLOAD_CAMERA_IR_CONTRAST_MODE_CUSTOM", PAYLOAD_CAMERA_IR_CONTRAST_MODE_CUSTOM}),
    [](const ::testing::TestParamInfo<IrContrastModeCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_IR_CONTRAST_MODE
// =============================================================================
// 4. IR Gain	PAYLOAD_CAMERA_IR_GAIN
// =============================================================================
// IR Gain	C_T_G	x	-	x	-	x	x
#if defined(PAYLOAD_CAMERA_IR_GAIN) && (defined(MB1) || defined(VIO))

using IrGainCase = CameraIrParamValue;   // was CameraEoParamValue

class CameraIrGainSettingValueTest
    : public CameraIrParamSettingTest,               // was CameraEoParamSettingTest
      public ::testing::WithParamInterface<IrGainCase> {};

TEST_P(CameraIrGainSettingValueTest, SetAndReadBack) {
    const IrGainCase param = GetParam();
    double original = -1;
    if (!cit::readCameraParam(PAYLOAD_CAMERA_IR_GAIN, original, 3000)) {
        GTEST_SKIP() << "Payload did not report IR Gain ("
                     << PAYLOAD_CAMERA_IR_GAIN << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_IR_GAIN,
                                     param.value, actual))
        << "No readback after setting IR Gain to " << param
        << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "IR Gain readback mismatch for " << param << ".";

    // Restore original value
    std::cout << "Restoring IR Gain to original value "
              << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_IR_GAIN,
                                     static_cast<uint32_t>(original), actual))
        << "No readback after restoring IR Gain to "
        << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraIrGainSettingValueTest,
    ::testing::Values(IrGainCase{"PAYLOAD_CAMERA_IR_GAIN_LOW", PAYLOAD_CAMERA_IR_GAIN_LOW},
                       IrGainCase{"PAYLOAD_CAMERA_IR_GAIN_HIGH", PAYLOAD_CAMERA_IR_GAIN_HIGH}),
    [](const ::testing::TestParamInfo<IrGainCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_IR_GAIN
// =============================================================================
// 5. IR Isotherm Mode PAYLOAD_CAMERA_IR_ISOTHERM_MODE
// =============================================================================
// C_T_ISO_M	x	-	x	-	x	x
#if defined(PAYLOAD_CAMERA_IR_ISOTHERM_MODE) && (defined(MB1) || defined(VIO))

using IrIsoThermModeCase = CameraIrParamValue;   // was CameraEoParamValue

class CameraIrIsoThermModeSettingValueTest
    : public CameraIrParamSettingTest,               // was CameraEoParamSettingTest
      public ::testing::WithParamInterface<IrIsoThermModeCase> {};

TEST_P(CameraIrIsoThermModeSettingValueTest, SetAndReadBack) {
    const IrIsoThermModeCase param = GetParam();
    double original = -1;
    if (!cit::readCameraParam(PAYLOAD_CAMERA_IR_ISOTHERM_MODE, original, 3000)) {
        GTEST_SKIP() << "Payload did not report IR IsoTherm Mode ("
                     << PAYLOAD_CAMERA_IR_ISOTHERM_MODE << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_IR_ISOTHERM_MODE,
                                     param.value, actual))
        << "No readback after setting IR IsoTherm Mode to " << param
        << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "IR IsoTherm Mode readback mismatch for " << param << ".";

    // Restore original value
    std::cout << "Restoring IR IsoTherm Mode to original value "
              << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_IR_ISOTHERM_MODE,
                                     static_cast<uint32_t>(original), actual))
        << "No readback after restoring IR IsoTherm Mode to "
        << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraIrIsoThermModeSettingValueTest,
    ::testing::Values(IrIsoThermModeCase{"PAYLOAD_CAMERA_IR_ISOTHERM_MODE_DISABLE", PAYLOAD_CAMERA_IR_ISOTHERM_MODE_DISABLE},
                       IrIsoThermModeCase{"PAYLOAD_CAMERA_IR_ISOTHERM_MODE_ENABLE", PAYLOAD_CAMERA_IR_ISOTHERM_MODE_ENABLE}),
    [](const ::testing::TestParamInfo<IrIsoThermModeCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_IR_ISOTHERM_MODE


// =============================================================================
// 6. IR Isotherm Threshold PAYLOAD_CAMERA_IR_ISOTHERM_THRESHOLD
// =============================================================================
// C_T_ISO_THR	-	-	x	-	x	-
#if defined(PAYLOAD_CAMERA_IR_ISOTHERM_THRESHOLD) && defined(MB1) && !defined(CAMERA_F1)

using IrIsoThermThresholdCase = CameraIrParamValue;   // was CameraEoParamValue

class CameraIrIsoThermThresholdSettingValueTest
    : public CameraIrParamSettingTest,               // was CameraEoParamSettingTest
      public ::testing::WithParamInterface<IrIsoThermThresholdCase> {};

TEST_P(CameraIrIsoThermThresholdSettingValueTest, SetAndReadBack) {
    const IrIsoThermThresholdCase param = GetParam();
    double original = -1;
    if (!cit::readCameraParam(PAYLOAD_CAMERA_IR_ISOTHERM_THRESHOLD, original, 3000)) {
        GTEST_SKIP() << "Payload did not report IR IsoTherm Threshold ("
                     << PAYLOAD_CAMERA_IR_ISOTHERM_THRESHOLD << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_IR_ISOTHERM_THRESHOLD,
                                     param.value, actual))
        << "No readback after setting IR IsoTherm Threshold to " << param
        << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "IR IsoTherm Threshold readback mismatch for " << param << ".";

    // Restore original value
    std::cout << "Restoring IR IsoTherm Threshold to original value "
              << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_IR_ISOTHERM_THRESHOLD,
                                     static_cast<uint32_t>(original), actual))
        << "No readback after restoring IR IsoTherm Threshold to "
        << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraIrIsoThermThresholdSettingValueTest,
    ::testing::Values(IrIsoThermThresholdCase{"PAYLOAD_CAMERA_IR_ISOTHERM_THRESHOLD_0", 0},
                      IrIsoThermThresholdCase{"PAYLOAD_CAMERA_IR_ISOTHERM_THRESHOLD_5", 5},
                      IrIsoThermThresholdCase{"PAYLOAD_CAMERA_IR_ISOTHERM_THRESHOLD_10", 10},
                      IrIsoThermThresholdCase{"PAYLOAD_CAMERA_IR_ISOTHERM_THRESHOLD_20", 20},
                      IrIsoThermThresholdCase{"PAYLOAD_CAMERA_IR_ISOTHERM_THRESHOLD_25", 25},
                      IrIsoThermThresholdCase{"PAYLOAD_CAMERA_IR_ISOTHERM_THRESHOLD_30", 30},
                      IrIsoThermThresholdCase{"PAYLOAD_CAMERA_IR_ISOTHERM_THRESHOLD_32", 32},
                      IrIsoThermThresholdCase{"PAYLOAD_CAMERA_IR_ISOTHERM_THRESHOLD_35", 35},
                      IrIsoThermThresholdCase{"PAYLOAD_CAMERA_IR_ISOTHERM_THRESHOLD_37", 37},
                      IrIsoThermThresholdCase{"PAYLOAD_CAMERA_IR_ISOTHERM_THRESHOLD_40", 40},
                      IrIsoThermThresholdCase{"PAYLOAD_CAMERA_IR_ISOTHERM_THRESHOLD_50", 50},
                      IrIsoThermThresholdCase{"PAYLOAD_CAMERA_IR_ISOTHERM_THRESHOLD_60", 60},
                      IrIsoThermThresholdCase{"PAYLOAD_CAMERA_IR_ISOTHERM_THRESHOLD_100", 100},
                       IrIsoThermThresholdCase{"PAYLOAD_CAMERA_IR_ISOTHERM_THRESHOLD_150", 150}),
    [](const ::testing::TestParamInfo<IrIsoThermThresholdCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_IR_ISOTHERM_THRESHOLD
// =============================================================================
// 7. IR Isitherm Units PAYLOAD_CAMERA_IR_ISOTHERM_UNITS
// =============================================================================
// C_T_ISO_U	x	-	x	-	-	x
#if defined(PAYLOAD_CAMERA_IR_ISOTHERM_UNITS) && (defined(MB1) || defined(VIO)) && defined(CAMERA_F1)

using IrIsoThermUnitsCase = CameraIrParamValue;   // was CameraEoParamValue

class CameraIrIsoThermUnitsSettingValueTest
    : public CameraIrParamSettingTest,               // was CameraEoParamSettingTest
      public ::testing::WithParamInterface<IrIsoThermUnitsCase> {};

TEST_P(CameraIrIsoThermUnitsSettingValueTest, SetAndReadBack) {
    const IrIsoThermUnitsCase param = GetParam();
    double original = -1;
    if (!cit::readCameraParam(PAYLOAD_CAMERA_IR_ISOTHERM_UNITS, original, 3000)) {
        GTEST_SKIP() << "Payload did not report IR IsoTherm Units ("
                     << PAYLOAD_CAMERA_IR_ISOTHERM_UNITS << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_IR_ISOTHERM_UNITS,
                                     param.value, actual))
        << "No readback after setting IR IsoTherm Units to " << param
        << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "IR IsoTherm Units readback mismatch for " << param << ".";

    // Restore original value
    std::cout << "Restoring IR IsoTherm Units to original value "
              << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_IR_ISOTHERM_UNITS,
                                     static_cast<uint32_t>(original), actual))
        << "No readback after restoring IR IsoTherm Units to "
        << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraIrIsoThermUnitsSettingValueTest,
    ::testing::Values(IrIsoThermUnitsCase{"PAYLOAD_CAMERA_IR_ISOTHERM_UNITS_KELVIN", PAYLOAD_CAMERA_IR_ISOTHERM_UNITS_KELVIN},
                      IrIsoThermUnitsCase{"PAYLOAD_CAMERA_IR_ISOTHERM_UNITS_CELSIUS", PAYLOAD_CAMERA_IR_ISOTHERM_UNITS_CELSIUS},
                      IrIsoThermUnitsCase{"PAYLOAD_CAMERA_IR_ISOTHERM_UNITS_FAHRENHEIT", PAYLOAD_CAMERA_IR_ISOTHERM_UNITS_FAHRENHEIT},
                      IrIsoThermUnitsCase{"PAYLOAD_CAMERA_IR_ISOTHERM_UNITS_PERCENT", PAYLOAD_CAMERA_IR_ISOTHERM_UNITS_PERCENT},
                       IrIsoThermUnitsCase{"PAYLOAD_CAMERA_IR_ISOTHERM_UNITS_COUNTS", PAYLOAD_CAMERA_IR_ISOTHERM_UNITS_COUNTS}),
    [](const ::testing::TestParamInfo<IrIsoThermUnitsCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_IR_ISOTHERM_UNITS

// =============================================================================
// 8. IR Palette PAYLOAD_CAMERA_IR_PALETTE
// =============================================================================
// C_T_PALETTE	x	x	x	-	x	x
#if defined(PAYLOAD_CAMERA_IR_PALETTE)

using IrPaletteCase = CameraIrParamValue;   // was CameraEoParamValue

class CameraIrPaletteSettingValueTest
    : public CameraIrParamSettingTest,               // was CameraEoParamSettingTest
      public ::testing::WithParamInterface<IrPaletteCase> {};

TEST_P(CameraIrPaletteSettingValueTest, SetAndReadBack) {
     // Set and read back IR SpotMeter Mode first, need isotherm off before set palette
     #if defined(MB1)
    cit::setCameraParam(PAYLOAD_CAMERA_IR_ISOTHERM_MODE, PAYLOAD_CAMERA_IR_ISOTHERM_MODE_DISABLE, 3000);
    #endif
    // cit::setCameraParam(C_T_ISO_M, 0, 3000);
    //
    const IrPaletteCase param = GetParam();
    double original = -1;
    if (!cit::readCameraParam(PAYLOAD_CAMERA_IR_PALETTE, original, 3000)) {
        GTEST_SKIP() << "Payload did not report IR Palette ("
                     << PAYLOAD_CAMERA_IR_PALETTE << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_IR_PALETTE,
                                     param.value, actual))
        << "No readback after setting IR Palette to " << param
        << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "IR Palette readback mismatch for " << param << ".";
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // Restore original value
    std::cout << "Restoring IR Palette to original value "
              << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_IR_PALETTE,
                                     static_cast<uint32_t>(original), actual))
        << "No readback after restoring IR Palette to "
        << original << ".";
    
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraIrPaletteSettingValueTest,
    ::testing::Values(IrPaletteCase{"PAYLOAD_CAMERA_IR_PALETTE_1", PAYLOAD_CAMERA_IR_PALETTE_1},
                      IrPaletteCase{"PAYLOAD_CAMERA_IR_PALETTE_2", PAYLOAD_CAMERA_IR_PALETTE_2},
                      IrPaletteCase{"PAYLOAD_CAMERA_IR_PALETTE_3", PAYLOAD_CAMERA_IR_PALETTE_3},
                      IrPaletteCase{"PAYLOAD_CAMERA_IR_PALETTE_4", PAYLOAD_CAMERA_IR_PALETTE_4},
                      IrPaletteCase{"PAYLOAD_CAMERA_IR_PALETTE_5", PAYLOAD_CAMERA_IR_PALETTE_5},
                      IrPaletteCase{"PAYLOAD_CAMERA_IR_PALETTE_6", PAYLOAD_CAMERA_IR_PALETTE_6},
                      IrPaletteCase{"PAYLOAD_CAMERA_IR_PALETTE_7", PAYLOAD_CAMERA_IR_PALETTE_7},
                      IrPaletteCase{"PAYLOAD_CAMERA_IR_PALETTE_8", PAYLOAD_CAMERA_IR_PALETTE_8},
                      IrPaletteCase{"PAYLOAD_CAMERA_IR_PALETTE_9", PAYLOAD_CAMERA_IR_PALETTE_9},
                       IrPaletteCase{"PAYLOAD_CAMERA_IR_PALETTE_10", PAYLOAD_CAMERA_IR_PALETTE_10}),
    [](const ::testing::TestParamInfo<IrPaletteCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_IR_PALETTE


// ---------------------------------------------------------------------------
// 9/10/11. IR Isotherm Color Mode / Color Min / Color Max, regions 0..5
// ---------------------------------------------------------------------------

// #if (defined(VIO) || defined(MB1)) && defined(CAMERA_F1)
 #if (defined(MB1)) && defined(CAMERA_F1)
// ---------------------------------------------------------------------------
// One region's param id + the value to set for it.
// ---------------------------------------------------------------------------
struct IsoThermRegionCase {
    std::string name;      // gtest instantiation name
    const char* paramId;   // e.g. PAYLOAD_CAMERA_IR_ISOTHERM_RG0_COLOR_MODE
    uint32_t value;
};
 
inline std::ostream& operator<<(std::ostream& os, const IsoThermRegionCase& c) {
    return os << c.paramId << "=" << c.value;
}
 
// ---------------------------------------------------------------------------
// 9/10/11. IR Isotherm Color Mode / Color Min / Color Max, regions 0..5
// ---------------------------------------------------------------------------

class CameraIrIsothermRegionColorSettingTest
    : public CameraIrParamSettingTest,
      public ::testing::WithParamInterface<IsoThermRegionCase> {};
 
TEST_P(CameraIrIsothermRegionColorSettingTest, SetAndReadBack) {
    const IsoThermRegionCase param = GetParam();
 
    double original = -1;
    if (!cit::readCameraParam(param.paramId, original, 3000)) {
        GTEST_SKIP() << "Payload did not report " << param.paramId << ".";
    }
 
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(param.paramId, param.value, actual))
        << "No readback after setting " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "Readback mismatch for " << param << ".";
 
    std::cout << "Restoring " << param.paramId << " to original value "
              << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(param.paramId, static_cast<uint32_t>(original), actual))
        << "No readback after restoring " << param.paramId << " to " << original << ".";
}
 
// Color Mode: 0=Disable 1=Standard 2=Linear RGB 3=Linear HSV 4=Non-linear 5=Single
// (enum values are only documented for RG0 in the header; assumed to apply
// the same way to RG1..RG5 — confirm with firmware docs if unsure)
INSTANTIATE_TEST_SUITE_P(
    ColorMode, CameraIrIsothermRegionColorSettingTest,
    ::testing::Values(
        IsoThermRegionCase{"RG0_Disable", PAYLOAD_CAMERA_IR_ISOTHERM_RG0_COLOR_MODE,
                            PAYLOAD_CAMERA_IR_ISOTHERM_RG0_COLOR_MODE_DISABLE},
        IsoThermRegionCase{"RG0_LinearRGB", PAYLOAD_CAMERA_IR_ISOTHERM_RG0_COLOR_MODE,
                            PAYLOAD_CAMERA_IR_ISOTHERM_RG0_COLOR_MODE_LINEAR_RGB},
        IsoThermRegionCase{"RG0_Single", PAYLOAD_CAMERA_IR_ISOTHERM_RG0_COLOR_MODE,
                            PAYLOAD_CAMERA_IR_ISOTHERM_RG0_COLOR_MODE_SINGLE},
        IsoThermRegionCase{"RG1_LinearRGB", PAYLOAD_CAMERA_IR_ISOTHERM_RG1_COLOR_MODE, 2},
        IsoThermRegionCase{"RG2_LinearRGB", PAYLOAD_CAMERA_IR_ISOTHERM_RG2_COLOR_MODE, 2},
        IsoThermRegionCase{"RG3_LinearRGB", PAYLOAD_CAMERA_IR_ISOTHERM_RG3_COLOR_MODE, 2},
        IsoThermRegionCase{"RG4_LinearRGB", PAYLOAD_CAMERA_IR_ISOTHERM_RG4_COLOR_MODE, 2},
        IsoThermRegionCase{"RG5_LinearRGB", PAYLOAD_CAMERA_IR_ISOTHERM_RG5_COLOR_MODE, 2}),
    [](const ::testing::TestParamInfo<IsoThermRegionCase>& info) { return info.param.name; });
 
// Color Min / Max: palette index (0=Black .. 9=Red .. 10=Magenta, per the
// vendor sample). Adjust the palette range if your firmware documents more.
INSTANTIATE_TEST_SUITE_P(
    ColorMin, CameraIrIsothermRegionColorSettingTest,
    ::testing::Values(
        IsoThermRegionCase{"RG0_Min", PAYLOAD_CAMERA_IR_ISOTHERM_RG0_COLOR_MIN, 0},
        IsoThermRegionCase{"RG1_Min", PAYLOAD_CAMERA_IR_ISOTHERM_RG1_COLOR_MIN, 0},
        IsoThermRegionCase{"RG2_Min", PAYLOAD_CAMERA_IR_ISOTHERM_RG2_COLOR_MIN, 0},
        IsoThermRegionCase{"RG3_Min", PAYLOAD_CAMERA_IR_ISOTHERM_RG3_COLOR_MIN, 0},
        IsoThermRegionCase{"RG4_Min", PAYLOAD_CAMERA_IR_ISOTHERM_RG4_COLOR_MIN, 0},
        IsoThermRegionCase{"RG5_Min", PAYLOAD_CAMERA_IR_ISOTHERM_RG5_COLOR_MIN, 0}),
    [](const ::testing::TestParamInfo<IsoThermRegionCase>& info) { return info.param.name; });
 
INSTANTIATE_TEST_SUITE_P(
    ColorMax, CameraIrIsothermRegionColorSettingTest,
    ::testing::Values(
        IsoThermRegionCase{"RG0_Max", PAYLOAD_CAMERA_IR_ISOTHERM_RG0_COLOR_MAX, 9},
        IsoThermRegionCase{"RG1_Max", PAYLOAD_CAMERA_IR_ISOTHERM_RG1_COLOR_MAX, 9},
        IsoThermRegionCase{"RG2_Max", PAYLOAD_CAMERA_IR_ISOTHERM_RG2_COLOR_MAX, 9},
        IsoThermRegionCase{"RG3_Max", PAYLOAD_CAMERA_IR_ISOTHERM_RG3_COLOR_MAX, 9},
        IsoThermRegionCase{"RG4_Max", PAYLOAD_CAMERA_IR_ISOTHERM_RG4_COLOR_MAX, 9},
        IsoThermRegionCase{"RG5_Max", PAYLOAD_CAMERA_IR_ISOTHERM_RG5_COLOR_MAX, 10}),
    [](const ::testing::TestParamInfo<IsoThermRegionCase>& info) { return info.param.name; });
 
// ---------------------------------------------------------------------------
// 12. IR Isotherm region temperature, RG1..RG4
// RG0 is intentionally excluded: per the vendor sample, RG0's low
// temperature is fixed at the sensor's Kelvin minimum and is not settable.
// ---------------------------------------------------------------------------
class CameraIrIsothermRegionTempSettingTest
    : public CameraIrParamSettingTest,
      public ::testing::WithParamInterface<IsoThermRegionCase> {};
 
TEST_P(CameraIrIsothermRegionTempSettingTest, SetAndReadBack) {
    const IsoThermRegionCase param = GetParam();
 
    double original = -1;
    if (!cit::readCameraParam(param.paramId, original, 3000)) {
        GTEST_SKIP() << "Payload did not report " << param.paramId << ".";
    }
 
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(param.paramId, param.value, actual))
        << "No readback after setting " << param << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "Readback mismatch for " << param << ".";
 
    std::cout << "Restoring " << param.paramId << " to original value "
              << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(param.paramId, static_cast<uint32_t>(original), actual))
        << "No readback after restoring " << param.paramId << " to " << original << ".";
}
 
// Header comment gives the raw range as -1000..1000, step 10, but the
// vendor sample sets plain Celsius integers (35, 40, 100, 140) directly —
// that only works once ID_ISOTHERM_UNIT is set to Celsius. If your test
// fixture doesn't already do this, set it once before this suite runs:
//   g_payload->setPayloadCameraParam("C_T_ISO_U", 1 /*Celsius*/, PARAM_TYPE_UINT32);
//   g_payload->setPayloadCameraParam("C_T_ISO_M", 1 /*Enable*/, PARAM_TYPE_UINT32);
INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraIrIsothermRegionTempSettingTest,
    ::testing::Values(
        IsoThermRegionCase{"RG1_35", PAYLOAD_CAMERA_IR_ISOTHERM_RG1_TEMP, 35},
        IsoThermRegionCase{"RG1_40", PAYLOAD_CAMERA_IR_ISOTHERM_RG1_TEMP, 40},
        IsoThermRegionCase{"RG2_50", PAYLOAD_CAMERA_IR_ISOTHERM_RG2_TEMP, 50},
        IsoThermRegionCase{"RG3_100", PAYLOAD_CAMERA_IR_ISOTHERM_RG3_TEMP, 100},
        IsoThermRegionCase{"RG4_140", PAYLOAD_CAMERA_IR_ISOTHERM_RG4_TEMP, 140}),
    [](const ::testing::TestParamInfo<IsoThermRegionCase>& info) { return info.param.name; });
 
#endif // (VIO || MB1) && CAMERA_F1
// =============================================================================
// 13. IR SpotMeter Mode PAYLOAD_CAMERA_IR_SPOTMETER_MODE
// =============================================================================
// C_T_SPOT_M	-	-	x	-	x	x
#if defined(PAYLOAD_CAMERA_IR_SPOTMETER_MODE) && defined(MB1)

using IrSpotMeterModeCase = CameraIrParamValue;   // was CameraEoParamValue

class CameraIrSpotMeterModeSettingValueTest
    : public CameraIrParamSettingTest,               // was CameraEoParamSettingTest
      public ::testing::WithParamInterface<IrSpotMeterModeCase> {};

TEST_P(CameraIrSpotMeterModeSettingValueTest, SetAndReadBack) {
    const IrSpotMeterModeCase param = GetParam();
    double original = -1;
    if (!cit::readCameraParam(PAYLOAD_CAMERA_IR_SPOTMETER_MODE, original, 3000)) {
        GTEST_SKIP() << "Payload did not report IR SpotMeter Mode ("
                     << PAYLOAD_CAMERA_IR_SPOTMETER_MODE << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_IR_SPOTMETER_MODE,
                                     param.value, actual))
        << "No readback after setting IR SpotMeter Mode to " << param
        << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "IR SpotMeter Mode readback mismatch for " << param << ".";

    // Restore original value
    std::cout << "Restoring IR SpotMeter Mode to original value "
              << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_IR_SPOTMETER_MODE,
                                     static_cast<uint32_t>(original), actual))
        << "No readback after restoring IR SpotMeter Mode to "
        << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraIrSpotMeterModeSettingValueTest,
    ::testing::Values(IrSpotMeterModeCase{"PAYLOAD_CAMERA_IR_SPOTMETER_MODE_DISABLE", PAYLOAD_CAMERA_IR_SPOTMETER_MODE_DISABLE},
                       IrSpotMeterModeCase{"PAYLOAD_CAMERA_IR_SPOTMETER_MODE_ENABLE", PAYLOAD_CAMERA_IR_SPOTMETER_MODE_ENABLE}),
    [](const ::testing::TestParamInfo<IrSpotMeterModeCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_IR_SPOTMETER_MODE

// =============================================================================
// 14. IR SpotMeter Size PAYLOAD_CAMERA_IR_SPOTMETER_SIZE
// =============================================================================
// C_T_SPOT_S	-	-	x	-	x	x
#if defined(PAYLOAD_CAMERA_IR_SPOTMETER_SIZE) && defined(MB1)

using IrSpotMeterSizeCase = CameraIrParamValue;   // was CameraEoParamValue

class CameraIrSpotMeterSizeSettingValueTest
    : public CameraIrParamSettingTest,               // was CameraEoParamSettingTest
      public ::testing::WithParamInterface<IrSpotMeterSizeCase> {};

TEST_P(CameraIrSpotMeterSizeSettingValueTest, SetAndReadBack) {
    const IrSpotMeterSizeCase param = GetParam();
    // Set and read back IR SpotMeter Mode first, because the SpotMeter Size param is only valid when the SpotMeter Mode is enabled.
    cit::setCameraParam(PAYLOAD_CAMERA_IR_ISOTHERM_MODE, PAYLOAD_CAMERA_IR_ISOTHERM_MODE_ENABLE, 3000);

    double original = -1;

    if (!cit::readCameraParam(PAYLOAD_CAMERA_IR_SPOTMETER_SIZE, original, 3000)) {
        GTEST_SKIP() << "Payload did not report IR SpotMeter Size ("
                     << PAYLOAD_CAMERA_IR_SPOTMETER_SIZE << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_IR_SPOTMETER_SIZE,
                                     param.value, actual))
        << "No readback after setting IR SpotMeter Size to " << param
        << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "IR SpotMeter Size readback mismatch for " << param << ".";

    // Restore original value
    std::cout << "Restoring IR SpotMeter Size to original value "
              << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_IR_SPOTMETER_SIZE,
                                     static_cast<uint32_t>(original), actual))
        << "No readback after restoring IR SpotMeter Size to "
        << original << ".";

    cit::setCameraParam(PAYLOAD_CAMERA_IR_ISOTHERM_MODE, PAYLOAD_CAMERA_IR_ISOTHERM_MODE_DISABLE, 3000);
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraIrSpotMeterSizeSettingValueTest,
    ::testing::Values(IrSpotMeterSizeCase{"PAYLOAD_CAMERA_IR_SPOTMETER_SIZE_0", 0},
                        IrSpotMeterSizeCase{"PAYLOAD_CAMERA_IR_SPOTMETER_SIZE_1", 1},
                        IrSpotMeterSizeCase{"PAYLOAD_CAMERA_IR_SPOTMETER_SIZE_16", 16},
                        IrSpotMeterSizeCase{"PAYLOAD_CAMERA_IR_SPOTMETER_SIZE_24", 24},
                        IrSpotMeterSizeCase{"PAYLOAD_CAMERA_IR_SPOTMETER_SIZE_36", 36},
                        IrSpotMeterSizeCase{"PAYLOAD_CAMERA_IR_SPOTMETER_SIZE_64", 64},
                        IrSpotMeterSizeCase{"PAYLOAD_CAMERA_IR_SPOTMETER_SIZE_96", 96},
                        IrSpotMeterSizeCase{"PAYLOAD_CAMERA_IR_SPOTMETER_SIZE_100", 100},
                        IrSpotMeterSizeCase{"PAYLOAD_CAMERA_IR_SPOTMETER_SIZE_124", 124},
                        IrSpotMeterSizeCase{"PAYLOAD_CAMERA_IR_SPOTMETER_SIZE_128", 128},
                       IrSpotMeterSizeCase{"PAYLOAD_CAMERA_IR_SPOTMETER_SIZE_132", 132}),
    [](const ::testing::TestParamInfo<IrSpotMeterSizeCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_IR_SPOTMETER_SIZE

// =============================================================================
// 15. IR SpotMeter Units PAYLOAD_CAMERA_IR_SPOTMETER_UNITS
// =============================================================================
// C_T_SPOT_U	-	-	x	-	x	x
#if defined(PAYLOAD_CAMERA_IR_SPOTMETER_UNITS) && defined(MB1)

using IrSpotMeterUnitsCase = CameraIrParamValue;   // was CameraEoParamValue

class CameraIrSpotMeterUnitsSettingValueTest
    : public CameraIrParamSettingTest,               // was CameraEoParamSettingTest
      public ::testing::WithParamInterface<IrSpotMeterUnitsCase> {};

TEST_P(CameraIrSpotMeterUnitsSettingValueTest, SetAndReadBack) {
    const IrSpotMeterUnitsCase param = GetParam();
    double original = -1;
    if (!cit::readCameraParam(PAYLOAD_CAMERA_IR_SPOTMETER_UNITS, original, 3000)) {
        GTEST_SKIP() << "Payload did not report IR SpotMeter Units ("
                     << PAYLOAD_CAMERA_IR_SPOTMETER_UNITS << ").";
    }
    double actual = -1;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_IR_SPOTMETER_UNITS,
                                     param.value, actual))
        << "No readback after setting IR SpotMeter Units to " << param
        << ".";
    EXPECT_EQ(static_cast<uint32_t>(actual), param.value)
        << "IR SpotMeter Units readback mismatch for " << param << ".";

    // Restore original value
    std::cout << "Restoring IR SpotMeter Units to original value "
              << original << "." << std::endl;
    ASSERT_TRUE(SetParamAndReadBack(PAYLOAD_CAMERA_IR_SPOTMETER_UNITS,
                                     static_cast<uint32_t>(original), actual))
        << "No readback after restoring IR SpotMeter Units to "
        << original << ".";
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues, CameraIrSpotMeterUnitsSettingValueTest,
    ::testing::Values(IrSpotMeterUnitsCase{"PAYLOAD_CAMERA_IR_SPOTMETER_UNITS_CELSIUS", PAYLOAD_CAMERA_IR_SPOTMETER_UNITS_CELSIUS},
                        IrSpotMeterUnitsCase{"PAYLOAD_CAMERA_IR_SPOTMETER_UNITS_FAHRENHEIT", PAYLOAD_CAMERA_IR_SPOTMETER_UNITS_FAHRENHEIT},
                       IrSpotMeterUnitsCase{"PAYLOAD_CAMERA_IR_SPOTMETEPAYLOAD_CAMERA_IR_SPOTMETER_UNITS_KELVINR_UNITS_132", PAYLOAD_CAMERA_IR_SPOTMETER_UNITS_KELVIN}),
    [](const ::testing::TestParamInfo<IrSpotMeterUnitsCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_IR_SPOTMETER_UNITS
// =============================================================================
// 16. Ir Isotherms
// =============================================================================
// Ir Isotherms	ISOTHERMS_EN	-	-	?	-	?	?

// =============================================================================
// 17. Ir Isotherms Gain
// =============================================================================
// Ir Isotherms Gain	ISOTHERMS_GAIN	-	-	?	-	?	?