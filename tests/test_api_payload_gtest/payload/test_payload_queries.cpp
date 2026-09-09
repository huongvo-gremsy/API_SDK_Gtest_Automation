/** @file test_payload_queries.cpp */
#include "payload_example_test_helpers.h"

#include <cmath>
#include <iostream>

namespace pet = payload_example_test;
namespace st = stream_test;

class PayloadComponentInfoExampleTest : public PayloadTest {};

TEST_F(PayloadComponentInfoExampleTest, ModelVersionSerial_Arrive) {
    std::vector<std::string> info;
    ASSERT_TRUE(getComponentInformation(info, 4000));
    ASSERT_EQ(info.size(), 3u);
    EXPECT_FALSE(info[0].empty());
    EXPECT_FALSE(info[1].empty());
    EXPECT_FALSE(info[2].empty());
}

class PayloadFovStatusExampleTest : public PayloadTest {};

TEST_F(PayloadFovStatusExampleTest, EoAndIr_AreValidWhenAvailable) {
    double cameraId = -1, hfov = 0, vfov = 0;
    ASSERT_TRUE(getCameraFov(CAMERA_EO, cameraId, hfov, vfov, 4000));
    EXPECT_GT(hfov, 0);
    EXPECT_LE(hfov, 180);
    EXPECT_GT(vfov, 0);
    EXPECT_LE(vfov, 180);

    if (getCameraFov(CAMERA_IR, cameraId, hfov, vfov, 2500)) {
        EXPECT_GT(hfov, 0);
        EXPECT_LE(hfov, 180);
        EXPECT_GT(vfov, 0);
        EXPECT_LE(vfov, 180);
    } else {
        std::cout << "[INFO] IR FOV is unavailable on this payload." << std::endl;
    }
}

class PayloadVideoStreamingExampleTest : public PayloadTest {};

TEST_F(PayloadVideoStreamingExampleTest, CameraCapabilityThenStreamInfo) {
    uint32_t flags = 0;
    ASSERT_TRUE(getCameraInformation(flags, 4000));
    ASSERT_NE(flags & CAMERA_CAP_FLAGS_HAS_VIDEO_STREAM, 0u)
        << "Camera does not advertise video streaming.";

    st::Snapshot stream;
    ASSERT_TRUE(st::getSnapshot(st::kDefaultStreamId, stream, 5000));
    EXPECT_TRUE(st::isValid(stream));
    EXPECT_FALSE(stream.uri.empty());
}

class PayloadStatusExampleTest : public PayloadTest {
protected:
    void TearDown() override {
        g_payload->setParamRate(PARAM_EO_ZOOM_LEVEL, 0);
        g_payload->setParamRate(PARAM_IR_ZOOM_LEVEL, 0);
    }
};

TEST_F(PayloadStatusExampleTest, SetRates_ProducesRequestedStatusValues) {
    const uint64_t eoSeq = pet::payloadParamSeq(PARAM_EO_ZOOM_LEVEL);
    const uint64_t irSeq = pet::payloadParamSeq(PARAM_IR_ZOOM_LEVEL);
    g_payload->setParamRate(PARAM_EO_ZOOM_LEVEL, 250);
    g_payload->setParamRate(PARAM_IR_ZOOM_LEVEL, 250);
    double eo = 0;
    ASSERT_TRUE(pet::waitForPayloadParamSince(
        PARAM_EO_ZOOM_LEVEL, eoSeq, eo, 5000));
    EXPECT_TRUE(std::isfinite(eo));

    double ir = 0;
    if (pet::waitForPayloadParamSince(
            PARAM_IR_ZOOM_LEVEL, irSeq, ir, 2500)) {
        EXPECT_TRUE(std::isfinite(ir));
    } else {
        std::cout << "[INFO] No IR zoom status on this payload." << std::endl;
    }
}

TEST_F(PayloadStatusExampleTest, RequestParamValue_ReturnsMatchingIndex) {
    const uint64_t seq = pet::payloadParamSeq(PARAM_EO_ZOOM_LEVEL);
    g_payload->requestParamValue(PARAM_EO_ZOOM_LEVEL);
    double value = 0;
    ASSERT_TRUE(pet::waitForPayloadParamSince(
        PARAM_EO_ZOOM_LEVEL, seq, value, 4000));
    EXPECT_TRUE(std::isfinite(value));
}
