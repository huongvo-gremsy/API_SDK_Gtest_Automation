/**
 * Sheet rows 8..9: Control Flags / Gimbal Mode - setPayloadCameraParam(GB_FW_FLAG / GB_MODE)
 * Support: per row, see the `support` column of kGimbalParams (VIO ORUSL MB1 ZIO)
 * Example: examples/gimbal_set_mode.cpp (the old GB_MODE path, now disabled in the example)
 *
 * Same table format as camera_eo/test_eo_09_params.cpp: each row writes EVERY
 * value in `values`, in order, and reads each one back.
 */

#include "gb_test_helpers.h"

using namespace gb;

// One row of the spreadsheet.
struct GimbalParamRow {
    const char* id;                 // parameter ID sent on the wire
    const char* name;               // name in the spreadsheet
    const char* support;            // VIO ORUSL MB1 ZIO: 'x' supported, '?' not confirmed, '-' not available
    std::vector<uint32_t> values;   // every value to write, in this order
    const char* note;
};

// gtest prints this instead of raw bytes in --gtest_list_tests and failures.
inline void PrintTo(const GimbalParamRow& row, std::ostream* os) {
    *os << row.id;
}

// To add a row: append a line here, in spreadsheet order.
static const std::vector<GimbalParamRow> kGimbalParams = {
    {"GB_FW_FLAG", "Control Flags", "--x-", {1, 0}, "0 overwrite, 1 forward"},
    {"GB_MODE",    "Gimbal Mode",   "????", {1, 2}, "0 off, 1 lock, 2 follow, 3 mapping, 4 reset. Macro exists on every product, ID missing in XML; the example now uses setGimbalMode() flags instead"},
};

class GB_ControlParam : public GimbalTest, public ::testing::WithParamInterface<GimbalParamRow> {};

// Check that setPayloadCameraParam() accepts every listed value and reads each one back.
TEST_P(GB_ControlParam, SetPayloadCameraParam) {
    const GimbalParamRow& row = GetParam();

    std::cout << "[  INFO  ] " << row.name << " (" << row.id << "): " << row.note << "\n";

    // Skip rows the spreadsheet marks '-' (or '?' without PAYLOAD_TEST_UNVERIFIED=1).
    char support = supportOnThisProduct(row.support);

    if (support == '-') {
        GTEST_SKIP() << "not available on this product (sheet: " << row.support << ")";
    }

    if (support == '?' && !runUnverified()) {
        GTEST_SKIP() << "not confirmed on this product (sheet: " << row.support
                     << "); set PAYLOAD_TEST_UNVERIFIED=1 to run it";
    }

    // Remember the current value so TearDown() puts it back.
    ASSERT_TRUE(restoreLater(row.id))
        << row.id << ": the payload did not answer getPayloadCameraSettingByID(). "
        << "The parameter is probably missing from this firmware's setting list "
        << "(check with ./build/examples/camera_load_settings).";

    double original = 0;

    ASSERT_TRUE(readParam(row.id, original))
        << row.id << ": could not read the current value";

    std::cout << "[  INFO  ] " << row.id << ": current value = " << original
              << ", writing " << row.values.size() << " values\n";

    // Write every value in order and read each one back.
    for (size_t i = 0; i < row.values.size(); i++) {
        uint32_t value = row.values[i];

        bool valueWritten = setParam(row.id, value);

        EXPECT_TRUE(valueWritten)
            << row.id << ": wrote " << value << " but the read-back did not match";

        if (valueWritten) {
            std::cout << "[  INFO  ] " << row.id << " = " << value << ", read back OK\n";
        } else {
            // Show what the payload reports instead, to tell "ignored" from "clamped".
            double actual = -1;
            readParam(row.id, actual);
            std::cout << "[  INFO  ] " << row.id << " = " << value << ", read back did not match, payload reports "
                      << actual << "\n";
        }

        sleepMs(300);
    }
}

// One test per spreadsheet row, named by the parameter ID (empty first argument = no prefix).
INSTANTIATE_TEST_SUITE_P(
    ,
    GB_ControlParam,
    ::testing::ValuesIn(kGimbalParams),
    [](const ::testing::TestParamInfo<GimbalParamRow>& info) {
        return std::string(info.param.id);
    }
);

// =============================================================================
// 8. Control Flags  PAYLOAD_CAMERA_GIMBAL_FW_FLAG
// =============================================================================
// GB_FW_FLAG	-	-	x	-
#if defined(PAYLOAD_CAMERA_GIMBAL_FW_FLAG) && defined(MB1)

struct GimbalControlFlagCase {
    const char* name;
    uint32_t value;
};

inline void PrintTo(const GimbalControlFlagCase& param, std::ostream* os) {
    *os << param.name << "(" << param.value << ")";
}

class GimbalControlFlagSettingValueTest
    : public GimbalTest,
      public ::testing::WithParamInterface<GimbalControlFlagCase> {
};

TEST_P(GimbalControlFlagSettingValueTest, SetAndReadBack) {
    const GimbalControlFlagCase param = GetParam();

    std::cout << "[  INFO  ] Gimbal Control Flag ("
              << PAYLOAD_CAMERA_GIMBAL_FW_FLAG << "): "
              << param.name << " = " << param.value << "\n";

    // -----------------------------------------------------------------------
    // 1. Read and save the original value
    // -----------------------------------------------------------------------
    double original = -1.0;

    if (!gb::readParam(
            PAYLOAD_CAMERA_GIMBAL_FW_FLAG,
            original,
            3000)) {

        GTEST_SKIP()
            << "Payload did not report Gimbal Control Flag ("
            << PAYLOAD_CAMERA_GIMBAL_FW_FLAG << ").";
    }

    std::cout << "[  INFO  ] Original value = "
              << original << "\n";

    // -----------------------------------------------------------------------
    // 2. Set the documented value
    // -----------------------------------------------------------------------
    const bool written = gb::setParam(
        PAYLOAD_CAMERA_GIMBAL_FW_FLAG,
        param.value,
        5000);

    EXPECT_TRUE(written)
        << PAYLOAD_CAMERA_GIMBAL_FW_FLAG
        << ": failed to set value "
        << param.value
        << " (" << param.name << ")";

    // -----------------------------------------------------------------------
    // 3. Read back and verify
    // -----------------------------------------------------------------------
    double actual = -1.0;

    const bool readBack = gb::readParam(
        PAYLOAD_CAMERA_GIMBAL_FW_FLAG,
        actual,
        3000);

    EXPECT_TRUE(readBack)
        << PAYLOAD_CAMERA_GIMBAL_FW_FLAG
        << ": no readback after setting value "
        << param.value;

    if (readBack) {
        std::cout << "[  INFO  ] "
                  << PAYLOAD_CAMERA_GIMBAL_FW_FLAG
                  << " = " << actual
                  << ", expected = " << param.value
                  << "\n";

        EXPECT_EQ(
            static_cast<uint32_t>(actual),
            param.value)
            << "Gimbal Control Flag readback mismatch for "
            << param.name;
    }

    // -----------------------------------------------------------------------
    // 4. Restore original value
    // -----------------------------------------------------------------------
    std::cout << "[  INFO  ] Restoring "
              << PAYLOAD_CAMERA_GIMBAL_FW_FLAG
              << " to original value "
              << original << "\n";

    const bool restored = gb::setParam(
        PAYLOAD_CAMERA_GIMBAL_FW_FLAG,
        static_cast<uint32_t>(original),
        5000);

    EXPECT_TRUE(restored)
        << PAYLOAD_CAMERA_GIMBAL_FW_FLAG
        << ": failed to restore original value "
        << original;
}

INSTANTIATE_TEST_SUITE_P(
    DocumentedValues,
    GimbalControlFlagSettingValueTest,
    ::testing::Values(GimbalControlFlagCase{"GB_FW_FLAG_Autopilot", 0},
                        GimbalControlFlagCase{"GB_FW_FLAG_OnboardComputer", 1},
                       GimbalControlFlagCase{"GB_FW_FLAG_SBUSConvert", 2}),
    [](const ::testing::TestParamInfo<GimbalControlFlagCase>& info) {
        return info.param.name;
    });

#endif  // PAYLOAD_CAMERA_GIMBAL_FW_FLAG
