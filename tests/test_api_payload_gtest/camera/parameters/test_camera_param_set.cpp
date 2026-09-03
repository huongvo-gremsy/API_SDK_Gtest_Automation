/** @file test_camera_param_set.cpp
 *  @brief State-verified tests for setPayloadCameraParam(). */

#include "camera_param_test_helpers.h"

class CameraParamSetTest : public PayloadTest {
protected:
    void roundTripAndRestore(const char* id, uint32_t testValue) {
        uint32_t original = 0;
        ASSERT_TRUE(config_param::readUint32Param(id, original))
            << "Could not read original " << id << ".";
        ASSERT_TRUE(config_param::setAndVerifyUint32Param(id, testValue))
            << "Could not set/read back " << id << "=" << testValue << ".";
        ASSERT_TRUE(config_param::setAndVerifyUint32Param(id, original))
            << "Could not restore " << id << "=" << original << ".";
    }
};

TEST_F(CameraParamSetTest, Uint32Value_RoundTripsAndRestores) {
    uint32_t original = 0;
    ASSERT_TRUE(config_param::readUint32Param(PAYLOAD_CAMERA_RC_MODE, original));
    const uint32_t alternate = original == PAYLOAD_CAMERA_RC_MODE_STANDARD
        ? PAYLOAD_CAMERA_RC_MODE_GREMSY
        : PAYLOAD_CAMERA_RC_MODE_STANDARD;
    roundTripAndRestore(PAYLOAD_CAMERA_RC_MODE, alternate);
}

TEST_F(CameraParamSetTest, ValueContainingZeroBytes_RoundTrips) {
    // 30720 == 0x00007800 contains embedded/leading zero bytes and protects
    // against treating MAVLink PARAM_EXT_VALUE as a C string.
    roundTripAndRestore(PAYLOAD_CAMERA_VIDEO_FOCUS_VALUE, 30720u);
}

TEST_F(CameraParamSetTest, MinimumAndMaximumKnownValues_RoundTrip) {
    uint32_t original = 0;
    ASSERT_TRUE(config_param::readUint32Param(
        PAYLOAD_CAMERA_EO_ICR_MODE_AUTO_THRESHOLD, original));
    ASSERT_TRUE(config_param::setAndVerifyUint32Param(
        PAYLOAD_CAMERA_EO_ICR_MODE_AUTO_THRESHOLD, 0));
    ASSERT_TRUE(config_param::setAndVerifyUint32Param(
        PAYLOAD_CAMERA_EO_ICR_MODE_AUTO_THRESHOLD, 255));
    ASSERT_TRUE(config_param::setAndVerifyUint32Param(
        PAYLOAD_CAMERA_EO_ICR_MODE_AUTO_THRESHOLD, original));
}

TEST_F(CameraParamSetTest, UnknownId_IsNotCreated) {
    char unknown[] = "NO_SUCH_PARAM";
    g_payload->setPayloadCameraParam(unknown, 1, PARAM_TYPE_UINT32);
    double value = 0;
    EXPECT_FALSE(getCameraSettingByID(unknown, value, 1200));
}

TEST_F(CameraParamSetTest, OutOfRangeEnum_IsRejectedOrClamped) {
    uint32_t original = 0;
    ASSERT_TRUE(config_param::readUint32Param(PAYLOAD_CAMERA_RC_MODE, original));
    char id[] = PAYLOAD_CAMERA_RC_MODE;
    g_payload->setPayloadCameraParam(id, 255, PARAM_TYPE_UINT32);

    double observed = 0;
    ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_RC_MODE, observed, 3000));
    EXPECT_NE(observed, 255.0)
        << "Invalid RC mode was stored without rejection or clamping.";
    ASSERT_TRUE(config_param::setAndVerifyUint32Param(
        PAYLOAD_CAMERA_RC_MODE, original));
}
