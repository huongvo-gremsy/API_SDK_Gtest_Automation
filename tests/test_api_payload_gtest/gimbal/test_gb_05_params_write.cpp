/**
 * Sheet row 5: Ghi tham so gimbal - setPayloadGimbalParamByID()
 * Support: VIO x | ORUSL x | MB1 ? | ZIO ?
 * Example: examples/gimbal_change_settings.cpp
 */

#include "gb_test_helpers.h"

using namespace gb;

class GB_ParamWrite : public GimbalTest {
protected:
    void SetUp() override {
        GB_SKIP_UNLESS_SUPPORTED("xx??");

        GimbalTest::SetUp();
    }
};
// Ghi tham số gimbal	setPayloadGimbalParamByID()	x	x	?	?
// Check the example flow: read STIFF_TILT, set it to 50, read back, restore, read back.
TEST_F(GB_ParamWrite, SetPayloadGimbalParamByID) {
    // Remember STIFF_TILT so TearDown() puts it back.
    ASSERT_TRUE(restoreGimbalParamLater("STIFF_TILT"))
        << "STIFF_TILT is not readable on this gimbal";

    double original = 0;

    ASSERT_TRUE(readGimbalParam("STIFF_TILT", original))
        << "getPayloadGimbalSettingByID(STIFF_TILT): no reply";

    std::cout << "[  INFO  ] STIFF_TILT current value = " << original << "\n";

    // Write a value that differs from the current one.
    double target = 50;

    if (original == 50) {
        target = 40;
    }

    bool valueWritten = setGimbalParam("STIFF_TILT", target);

    EXPECT_TRUE(valueWritten)
        << "STIFF_TILT: wrote " << target << " but the read-back did not match";

    if (valueWritten) {
        std::cout << "[  INFO  ] STIFF_TILT: " << original << " -> " << target << ", read back OK\n";
    } else {
        std::cout << "[  INFO  ] STIFF_TILT: " << original << " -> " << target << ", read back did not match\n";
    }

    // Put the original value back, like the example does.
    bool restored = setGimbalParam("STIFF_TILT", original);

    EXPECT_TRUE(restored)
        << "STIFF_TILT: could not restore " << original;

    std::cout << "[  INFO  ] STIFF_TILT: " << target << " -> " << original << ", restored\n";
}
