/**
 * @file test_camera_information.cpp
 * @brief Read-only CAMERA_INFORMATION capability tests.
 *
 * PayloadSdkInterface currently exposes only the CAMERA_INFORMATION flags
 * field. Vendor, model, firmware version, focal length, sensor size, and
 * definition URI are decoded internally but are not forwarded by the public
 * callback, so they cannot be truthfully verified through the SDK API yet.
 */

#include "camera_query_test_helpers.h"

#include <cstdint>
#include <iomanip>
#include <iostream>

namespace {

void printCapabilities(uint32_t flags) {
    std::cout << "[INFO] CAMERA_INFORMATION flags=0x"
              << std::hex << std::uppercase << flags << std::dec
              << " image=" << !!(flags & CAMERA_CAP_FLAGS_CAPTURE_IMAGE)
              << " video=" << !!(flags & CAMERA_CAP_FLAGS_CAPTURE_VIDEO)
              << " modes=" << !!(flags & CAMERA_CAP_FLAGS_HAS_MODES)
              << " zoom=" << !!(flags & CAMERA_CAP_FLAGS_HAS_BASIC_ZOOM)
              << " focus=" << !!(flags & CAMERA_CAP_FLAGS_HAS_BASIC_FOCUS)
              << " stream=" << !!(flags & CAMERA_CAP_FLAGS_HAS_VIDEO_STREAM)
              << std::endl;
}

uint32_t requestCameraInformation() {
    uint32_t flags = 0;
    EXPECT_TRUE(getCameraInformation(flags, 3000))
        << "No new PAYLOAD_CAM_INFO response within 3000 ms.";
    printCapabilities(flags);
    return flags;
}

struct CapabilityCase {
    const char* name;
    uint32_t flag;
};

std::ostream& operator<<(std::ostream& os, const CapabilityCase& capability) {
    return os << capability.name << " (flag=0x" << std::hex
              << capability.flag << std::dec << ")";
}

}  // namespace

class CameraInformationTest : public PayloadTest {};

TEST_F(CameraInformationTest, Request_ReturnsInformation) {
    // CameraInformationTest.Request_ReturnsInformation
    uint32_t flags = 0;
    ASSERT_TRUE(getCameraInformation(flags, 3000))
        << "The payload did not respond to MAV_CMD_REQUEST_CAMERA_INFORMATION.";
    printCapabilities(flags);
}

TEST_F(CameraInformationTest, CapabilityBitmap_IsNotEmpty) {
    // CameraInformationTest.CapabilityBitmap_IsNotEmpty
    const uint32_t flags = requestCameraInformation();
    EXPECT_NE(flags, 0u)
        << "The camera responded but advertised no MAVLink capabilities.";
}

class CameraInformationCapabilityTest
    : public CameraInformationTest,
      public ::testing::WithParamInterface<CapabilityCase> {};

TEST_P(CameraInformationCapabilityTest, CoreFeature_IsAdvertised) {
    // CameraInformationCapabilityTest.CoreFeature_IsAdvertised
    const CapabilityCase capability = GetParam();
    const uint32_t flags = requestCameraInformation();
    EXPECT_NE(flags & capability.flag, 0u)
        << "CAMERA_INFORMATION does not advertise the " << capability.name
        << " capability even though this PayloadSDK test suite uses it.";
}

INSTANTIATE_TEST_SUITE_P(
    CoreCapabilities,
    CameraInformationCapabilityTest,
    ::testing::Values(
        CapabilityCase{"CaptureImage", CAMERA_CAP_FLAGS_CAPTURE_IMAGE},
        CapabilityCase{"CaptureVideo", CAMERA_CAP_FLAGS_CAPTURE_VIDEO},
        CapabilityCase{"CameraModes", CAMERA_CAP_FLAGS_HAS_MODES},
        CapabilityCase{"BasicZoom", CAMERA_CAP_FLAGS_HAS_BASIC_ZOOM},
        CapabilityCase{"BasicFocus", CAMERA_CAP_FLAGS_HAS_BASIC_FOCUS},
        CapabilityCase{"VideoStream", CAMERA_CAP_FLAGS_HAS_VIDEO_STREAM}),
    [](const ::testing::TestParamInfo<CapabilityCase>& info) {
        return info.param.name;
    });

TEST_F(CameraInformationTest, RepeatedRequests_ReturnStableCapabilities) {
    // CameraInformationTest.RepeatedRequests_ReturnStableCapabilities
    uint32_t firstFlags = 0;
    ASSERT_TRUE(getCameraInformation(firstFlags, 3000))
        << "No response to the first camera-information request.";

    for (int request = 2; request <= 3; ++request) {
        uint32_t currentFlags = 0;
        ASSERT_TRUE(getCameraInformation(currentFlags, 3000))
            << "No response to camera-information request #" << request << ".";
        EXPECT_EQ(currentFlags, firstFlags)
            << "Capability flags changed between read-only requests.";
    }

    printCapabilities(firstFlags);
}
