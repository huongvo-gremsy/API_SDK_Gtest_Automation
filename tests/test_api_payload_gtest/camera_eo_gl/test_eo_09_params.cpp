/**
 * Sheet rows 9..64: EO camera parameters - setPayloadCameraParam()
 * Support: per row, see the `support` column of kEoParams (VIO ORUSL MB1 ZIO)
 * Example: examples/camera_change_settings.cpp
 *
 * Each row writes EVERY value in `values`, in order, and reads each one back.
 * Enumerated parameters list all their values; wide ranges list min / middle / max.
 * Changing settings does not need storage, so this file never touches capture.
 */

#include "camera_eo_test_helpers.h"

using namespace eo;

// One row of the spreadsheet.
struct EoParamRow {
    const char* id;                 // parameter ID sent on the wire
    const char* name;               // name in the spreadsheet
    const char* support;            // VIO ORUSL MB1 ZIO: 'x' supported, '?' not confirmed, '-' not available
    std::vector<uint32_t> values;   // every value to write, in this order
    const char* note;
};

// gtest prints this instead of raw bytes in --gtest_list_tests and failures.
inline void PrintTo(const EoParamRow& row, std::ostream* os) {
    *os << row.id;
}

// To add a row: append a line here, in spreadsheet order.
static const std::vector<EoParamRow> kEoParams = {
    {"C_G_ADRC",       "ADRC",                      "----", {0, 1},                                  "MB1: setting exists but the camera driver crashes, never send it"},
    {"C_G_AE_COMP",    "Exposure Compensation",     "--x-", {0, 12, 24},                             "sheet -12..12, payload reports 0..24"},
    {"C_G_AE_MODE",    "Video Auto Exposure",       "--?-", {0, 1},                                  "MB1: macro only, ID missing in XML"},
    {"C_G_CONTRA",     "Contrast",                  "--x-", {1, 5, 10},                              "1..10"},
    {"C_G_CTRL_M",     "Control Mode",              "--x-", {0, 1, 2, 3},                            "0 off, 1 auto, 2 use scene, 3 off keep state"},
    {"C_G_EXPO_L",     "Exposure Lock",             "--x-", {0, 1},                                  "0 off, 1 on"},
    {"C_G_EXPO_M",     "Exposure Mode",             "--x-", {0, 1},                                  "0 manual, 1 auto"},
    {"C_G_EXPO_ME",    "Exposure Metering",         "--x-", {0, 1, 2},                               "0 average, 1 center, 2 spot"},
    {"C_G_EXPO_TIME",  "Exposure Time (us)",        "--x-", {200, 10000, 100000},                    "200..100000 step 100"},
    {"C_G_GAIN",       "Eo Gain Ls",                "--?-", {1, 2},                                  "MB1: macro only, ID missing in XML"},
    {"C_G_IRIS",       "Video Aperture Value",      "--?-", {1, 2},                                  "MB1: macro only, ID missing in XML"},
    {"C_G_ISO",        "ISO",                       "--x-", {0, 1, 2, 3, 4, 5, 6, 7},                "0 auto, 1 deblur, 2 ISO100 .. 7 ISO3200"},
    {"C_G_ISO_M",      "Manual ISO",                "--x-", {100, 800, 3200},                        "100..3200 step 10"},
    {"C_G_NR",         "Noise Reduction",           "--x-", {0, 1, 2},                               "0 off, 1 fast, 2 HQ"},
    {"C_G_N_MODE_FPS", "Night Mode FPS",            "--x-", {0, 1, 2},                               "0 30fps, 1 20fps, 2 10fps"},
    {"C_G_N_MODE_OPT", "Night Mode Options",        "--x-", {0, 1, 2},                               "0 auto, 1 off, 2 on"},
    {"C_G_PROFILE",    "Camera Profile",            "--x-", {0, 1, 2},                               "0 custom, 1 daylight, 2 night"},
    {"C_G_SATU",       "Saturation",                "--x-", {0, 5, 10},                              "0..10"},
    {"C_G_SCENE",      "Camera optimizations",      "--x-", {0, 4, 8, 12, 16},                       "0 disabled, 4 landscape, ... 16 HDR"},
    {"C_G_SHARPNESS",  "Sharpness",                 "--x-", {0, 3, 6},                               "0..6"},
    {"C_G_SHDR",       "Super HDR",                 "--x-", {0, 1},                                  "0 off, 1 on"},
    {"C_G_SHUTTER",    "Video Shutter Speed",       "--?-", {1, 2},                                  "MB1: macro only, ID missing in XML"},
    {"C_G_WB",         "White Balance",             "--x-", {3, 5, 8, 9, 10},                        "3 auto, 8 daylight; payload also reports 5, 9, 10"},
    {"C_V_AE",         "EO Auto Exposure Mode",     "xx-x", {0, 3, 10, 11},                          "0 auto, 3 manual, 10 shutter, 11 iris"},
    {"C_V_BGAIN",      "EO B Gain Value",           "xx-x", {0, 128, 255},                           "0..255, effective in WB manual"},
    {"C_V_BrP_HS",     "Bright Value (HS)",         "??-x", {0, 20, 41},                             "0..41; VIO/ORUSL: macro only, ID missing in XML"},
    {"C_V_BrP_LS",     "Bright Value (LS)",         "??-x", {0, 20, 37},                             "0..37; VIO/ORUSL: macro only, ID missing in XML"},
    {"C_V_DEFOG",      "EO Defog Mode",             "xx-x", {2, 3},                                  "2 on, 3 off"},
    {"C_V_DEFOG_LV",   "EO Defog Level",            "xx-x", {0, 1, 2, 3},                            "0 lowest .. 3 high"},
    {"C_V_EIS",        "EO Image Stabilizer",       "-x--", {0, 2, 3},                               "0 hold, 2 on, 3 off"},
    {"C_V_EIS_LV",     "EO Image Stabilizer Level", "-x--", {2, 3},                                  "2 super, 3 super plus"},
    {"C_V_FLIP",       "EO Flip",                   "xxxx", {2, 3},                                  "2 on, 3 off"},
    {"C_V_FLREDUCT",   "EO Flicker Reduction",      "-x--", {2, 3},                                  "2 on, 3 off"},
    {"C_V_FM",         "EO Focus Mode",             "xx-x", {0, 1, 2},                               "0 manual, 1 auto, 2 one push"},
    {"C_V_FOCUS_SPEED","Eo Focus Speed",            "--?-", {1, 2},                                  "MB1: macro only, ID missing in XML"},
    {"C_V_FREEZE",     "EO Freeze",                 "xx-x", {2, 3},                                  "2 on, 3 off"},
    {"C_V_FV",         "EO Manual Focus Value",     "xx-x", {255, 256, 257, 511, 512, 513, 1024, 4095, 4096, 30720, 61440, 61439}, "0..61440. SDK bug probe: values divisible by 256 are sent as 0 (strcpy on the value bytes)"},
    {"C_V_F_SPD",      "EO Focus Speed",            "xx--", {0, 3, 7},                               "0..7"},
    {"C_V_GAIN_HS",    "Gain Value (HS)",           "?--x", {1, 9, 17},                              "1..17; VIO: macro only, ID missing in XML"},
    {"C_V_GAIN_LS",    "Gain Value (LS)",           "?--x", {1, 7, 13},                              "1..13; VIO: macro only, ID missing in XML"},
    {"C_V_HS",         "EO High Sensitivity",       "xx-x", {2, 3},                                  "2 on, 3 off (ZIO inverted)"},
    {"C_V_ICR",        "EO AutoICR Mode",           "xx-x", {2, 3},                                  "2 auto, 3 manual"},
    {"C_V_ICR_MAN",    "EO ICR Mode",               "xx-x", {2, 3},                                  "2 on, 3 off"},
    {"C_V_ICR_THR",    "EO ICR Threshold",          "xx-x", {0, 128, 255},                           "0..255"},
    {"C_V_IrP",        "EO Aperture Value",         "xx-x", {11, 14},                                "index table differs per product; 11 and 14 exist on all"},
    {"C_V_MinSP",      "EO Min Shutter Limit",      "xx-x", {13, 20, 29},                            "13..29"},
    {"C_V_NSREDUCT",   "EO Noise Reduction",        "-x--", {0, 2, 5, 127},                          "0..5, 127 manual"},
    {"C_V_NSREDUCT2D", "2DNR level",                "-x--", {0, 2, 5},                               "0..5"},
    {"C_V_NSREDUCT3D", "3DNR level",                "-x--", {0, 2, 5},                               "0..5"},
    {"C_V_RGAIN",      "EO R Gain Value",           "xx-x", {0, 128, 255},                           "0..255, effective in WB manual"},
    {"C_V_SP",         "EO Shutter Value",          "xx-x", {13, 20},                                "index table differs per product; 13 and 20 exist on all"},
    {"C_V_SPAVOID",    "EO Spot Light Avoidance",   "-x--", {0, 1},                                  ""},
    {"C_V_STZOOM",     "EO Stable Zoom",            "-x--", {0, 1},                                  ""},
    {"C_V_WB",         "EO WB Mode",                "xx-x", {0, 1, 2, 3, 4, 5},                      "0 auto, 1 indoor, 2 outdoor, 3 one push, 4 ATW, 5 manual"},
    {"C_V_ZM_CB_LV",   "EO Zoom Level (Combine)",   "xx-x", {0, 1},                                  "index into the combine table; 0 = 1x"},
    {"C_V_ZM_MODE",    "EO dZoom Mode",             "xx-x", {0, 2},                                  "0 combine, 2 super resolution"},
    {"C_V_ZM_SR_LV",   "EO Zoom Level (SuperRes)",  "xx-x", {0, 1, 2},                               "index into the super-resolution table; 0 = 1x, 2 = 4x"},
    {"C_V_ZOOM",       "EO Zoom",                   "--x-", {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},  "0 = 1x .. 11 = 12x"},
    {"C_V_ZOOM_SPEED", "Eo Zoom Speed",             "--?-", {3, 5},                                  "MB1: macro only, ID missing in XML"},
    {"C_V_Z_SPD",      "EO Zoom Speed",             "xx--", {0, 3, 7},                               "0..7"},
};

class EO_Param : public EoCameraTest, public ::testing::WithParamInterface<EoParamRow> {};

// Check that setPayloadCameraParam() accepts every listed value and reads each one back.
TEST_P(EO_Param, SetPayloadCameraParam) {
    const EoParamRow& row = GetParam();

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
    EO_Param,
    ::testing::ValuesIn(kEoParams),
    [](const ::testing::TestParamInfo<EoParamRow>& info) {
        return std::string(info.param.id);
    }
);
