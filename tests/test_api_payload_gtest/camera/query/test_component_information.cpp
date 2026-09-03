/**
 * @file test_component_information.cpp
 * @brief Tests getPayloadComponentBasicInformation().
 */

#include "../../common/payload_test_fixture.h"

#include <iostream>
#include <vector>

namespace {

std::vector<std::string> requestComponentInformation() {
    std::vector<std::string> info;
    EXPECT_TRUE(getComponentInformation(info, 3000))
        << "No PAYLOAD_COMP_INFO callback within 3000 ms.";
    return info;
}

}  // namespace

class ComponentInformationTest : public PayloadTest {};

TEST_F(ComponentInformationTest, InfoCallbackArrives) {
    std::vector<std::string> info;
    ASSERT_TRUE(getComponentInformation(info, 3000))
        << "The payload did not respond with COMPONENT_INFORMATION_BASIC.";
    EXPECT_EQ(info.size(), 3u)
        << "SDK contract is [model_name, software_version, serial_number].";
}

TEST_F(ComponentInformationTest, ModelName_IsNonempty) {
    const auto info = requestComponentInformation();
    ASSERT_EQ(info.size(), 3u);
    EXPECT_FALSE(info[0].empty());
}

TEST_F(ComponentInformationTest, SoftwareVersion_IsNonempty) {
    const auto info = requestComponentInformation();
    ASSERT_EQ(info.size(), 3u);
    EXPECT_FALSE(info[1].empty());
}

TEST_F(ComponentInformationTest, SerialNumber_IsNonempty) {
    const auto info = requestComponentInformation();
    ASSERT_EQ(info.size(), 3u);
    EXPECT_FALSE(info[2].empty());
}

TEST_F(ComponentInformationTest, RepeatedRequests_ReturnStableIdentity) {
    const auto first = requestComponentInformation();
    const auto second = requestComponentInformation();
    ASSERT_EQ(first.size(), 3u);
    ASSERT_EQ(second.size(), 3u);
    EXPECT_EQ(second, first);

    std::cout << "[INFO] Component model=" << first[0]
              << " software=" << first[1]
              << " serial=" << first[2] << std::endl;
}
