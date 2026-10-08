/**
 * Sheet row 4: Doc tham so gimbal: tat ca / ID / index - getPayloadGimbalSettingList() / ByID() / ByIndex()
 * Support: VIO x | ORUSL x | MB1 x | ZIO x
 * Example: examples/gimbal_load_settings.cpp
 */

#include "gb_test_helpers.h"

#include <set>

using namespace gb;

class GB_ParamRead : public GimbalTest {
protected:
    void SetUp() override {
        GB_SKIP_UNLESS_SUPPORTED("xxxx");

        GimbalTest::SetUp();
    }
};

// Check that getPayloadGimbalSettingList() returns the parameter list.
TEST_F(GB_ParamRead, GetPayloadGimbalSettingList) {
    std::vector<GimbalParamSample> list = readGimbalParamList(3000);

    ASSERT_FALSE(list.empty())
        << "getPayloadGimbalSettingList(): no PARAM_VALUE reply within 3 s";

    // Count unique ids (VERSION_X is streamed by the gimbal and can repeat).
    std::set<std::string> ids;

    for (size_t i = 0; i < list.size(); i++) {
        ids.insert(list[i].id);
    }

    std::cout << "[  INFO  ] received " << list.size() << " PARAM_VALUE, " << ids.size() << " unique ids\n";

    for (size_t i = 0; i < list.size() && i < 8; i++) {
        std::cout << "[  INFO  ]   index " << list[i].index << ": " << list[i].id << " = " << list[i].value << "\n";
    }

    EXPECT_GE(ids.size(), 10u)
        << "the gimbal reported fewer than 10 different parameters";

    EXPECT_TRUE(ids.count("VERSION_X") == 1)
        << "VERSION_X is missing from the list";

    // One PARAM_VALUE can be lost on UDP. A missing id is confirmed with a read by id
    // before calling it missing.
    if (ids.count("STIFF_TILT") == 0) {
        std::cout << "[  INFO  ] STIFF_TILT was not in the burst, reading it by id\n";

        double value = 0;

        EXPECT_TRUE(readGimbalParam("STIFF_TILT", value))
            << "STIFF_TILT is missing from the list and cannot be read by id";
    }
}

// Check that getPayloadGimbalSettingByID() answers for known ids.
TEST_F(GB_ParamRead, GetPayloadGimbalSettingByID) {
    double version = 0;

    ASSERT_TRUE(readGimbalParam("VERSION_X", version))
        << "getPayloadGimbalSettingByID(VERSION_X): no reply";

    std::cout << "[  INFO  ] VERSION_X = " << version << "\n";

    double stiffTilt = 0;

    ASSERT_TRUE(readGimbalParam("STIFF_TILT", stiffTilt))
        << "getPayloadGimbalSettingByID(STIFF_TILT): no reply";

    std::cout << "[  INFO  ] STIFF_TILT = " << stiffTilt << "\n";

    // Reading again must give the same value.
    double stiffTiltAgain = 0;

    ASSERT_TRUE(readGimbalParam("STIFF_TILT", stiffTiltAgain));

    EXPECT_EQ(stiffTiltAgain, stiffTilt)
        << "STIFF_TILT changed between two reads";
}

// Check that getPayloadGimbalSettingByIndex() answers and matches the read by id.
TEST_F(GB_ParamRead, GetPayloadGimbalSettingByIndex) {
    GimbalParamSample first;

    ASSERT_TRUE(readGimbalParamByIndex(0, first))
        << "getPayloadGimbalSettingByIndex(0): no reply";

    std::cout << "[  INFO  ] index 0: " << first.id << " = " << first.value << "\n";

    EXPECT_EQ(first.index, 0);

    EXPECT_FALSE(first.id.empty())
        << "index 0 came back without an id";

    // The same parameter read by id must have the same value.
    double byId = 0;

    ASSERT_TRUE(readGimbalParam(first.id.c_str(), byId))
        << "getPayloadGimbalSettingByID(" << first.id << "): no reply";

    EXPECT_EQ(byId, first.value)
        << first.id << ": by index = " << first.value << ", by id = " << byId;

    std::cout << "[  INFO  ] " << first.id << " by id = " << byId << "\n";
}
