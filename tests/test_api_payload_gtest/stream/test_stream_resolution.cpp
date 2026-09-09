#include "../common/payload_test_fixture.h"


#include <cstdint>
#include <iostream>
#include <thread>
#include <chrono>

/**
 * @file test_stream_resolution.cpp
 * @brief Tests for setPayloadStreamResolution().
 *
* EO resolution_lv -> pixel dimensions (cam_id = 1):
 *   0: 1920x1080
 *   1: 1280x720
 *   2: 960x540
 *
 * IR resolution_lv -> pixel dimensions (cam_id = 2):
 *   0: 1280x1024
 *   1: 640x512
 *   2: 480x384
 *   3: 320x256
 *   4: 160x128
 *
 * Uses the same MAV_CMD_USER_4 dispatch family as setPayloadStreamBitrate()
 * (param1=4, param2=2, param4=1 selects "resolution"), verified the same
 * way -- via getPayloadCameraStreamingInformation() -> streamValues[1]/[2]
 * (resolution_v/resolution_h), not a synchronous getter (none exists for
 * resolution level directly).
 */

namespace {
constexpr uint32_t EO_CAMERA_ID = 1;
constexpr uint32_t IR_CAMERA_ID = 2;

struct ResolutionLevel {
    uint32_t level;
    double width;
    double height;
};

// EO
constexpr ResolutionLevel EO_RES_1080P = {0, 1920, 1080};
constexpr ResolutionLevel EO_RES_720P  = {1, 1280, 720};
constexpr ResolutionLevel EO_RES_540P  = {2, 960, 540};

// IR
constexpr ResolutionLevel IR_RES_1280x1024 = {0, 1280, 1024};
constexpr ResolutionLevel IR_RES_640x512   = {1, 640, 512};
constexpr ResolutionLevel IR_RES_480x384   = {2, 480, 384};
constexpr ResolutionLevel IR_RES_320x256   = {3, 320, 256};
constexpr ResolutionLevel IR_RES_160x128   = {4, 160, 128};

// Set resolution level for a given camera and poll streaming info until it
// reads back as the expected width/height, re-sending on each retry.
bool setAndVerifyResolution(uint32_t camId, const ResolutionLevel& target,
                             int timeoutMs = 5000, int retryIntervalMs = 500) {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->setPayloadStreamResolution(camId, target.level);
        std::this_thread::sleep_for(std::chrono::milliseconds(retryIntervalMs));
        // small delay to allow processing
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));


        uint64_t seq = g_cb.streamSeq.load();
        g_payload->getPayloadCameraStreamingInformation(camId);
        if (waitForSeq(g_cb.streamSeq, seq, retryIntervalMs)) {
            std::lock_guard<std::mutex> lock(g_cb.m);
            double resV = g_cb.streamValues[1];
            double resH = g_cb.streamValues[2];
            if (resV == target.height && resH == target.width) {
                return true;
            }
        }
    }
    return false;
}

bool getResolution(uint32_t camId, double& outWidth, double& outHeight, int timeoutMs = 7000) {
    uint64_t seq = g_cb.streamSeq.load();
    g_payload->getPayloadCameraStreamingInformation(camId);
    if (!waitForSeq(g_cb.streamSeq, seq, timeoutMs)) return false;
    std::lock_guard<std::mutex> lock(g_cb.m);
    outHeight = g_cb.streamValues[1];
    outWidth = g_cb.streamValues[2];
    return true;
}
}

// ==================== EO ====================

class StreamResolutionTest : public PayloadTest {};

TEST_F(StreamResolutionTest, SetResolution_1080p_Verified)
{
    EXPECT_TRUE(setAndVerifyResolution(EO_CAMERA_ID, EO_RES_1080P))
        << "Resolution did not read back as 1920x1080 after setting level 0.";
}

TEST_F(StreamResolutionTest, SetResolution_720p_Verified)
{
    EXPECT_TRUE(setAndVerifyResolution(EO_CAMERA_ID, EO_RES_720P))
        << "Resolution did not read back as 1280x720 after setting level 1.";
}

TEST_F(StreamResolutionTest, SetResolution_540p_Verified)
{
    EXPECT_TRUE(setAndVerifyResolution(EO_CAMERA_ID, EO_RES_540P))
        << "Resolution did not read back as 960x540 after setting level 2.";
}

TEST_F(StreamResolutionTest, SetAndGetStreamResolution)
{
    double baselineWidth, baselineHeight;
    ASSERT_TRUE(getResolution(EO_CAMERA_ID, baselineWidth, baselineHeight))
        << "No response for baseline EO resolution.";
    std::cout << "[INFO] EO baseline resolution: " << baselineWidth << "x" << baselineHeight << std::endl;

    const ResolutionLevel* baselineLevel = &EO_RES_1080P;
    for (const auto* lvl : { &EO_RES_1080P, &EO_RES_720P, &EO_RES_540P }) {
        if (lvl->height == baselineHeight && lvl->width == baselineWidth) {
            baselineLevel = lvl;
            break;
        }
    }

    std::cout << "[TEST] Setting EO resolution to 1280x720 (level 1)" << std::endl;
    ASSERT_TRUE(setAndVerifyResolution(EO_CAMERA_ID, EO_RES_720P))
        << "Resolution did not read back as 1280x720 after setting.";

    double actualWidth, actualHeight;
    ASSERT_TRUE(getResolution(EO_CAMERA_ID, actualWidth, actualHeight));
    EXPECT_EQ(actualWidth, EO_RES_720P.width);
    EXPECT_EQ(actualHeight, EO_RES_720P.height);

    std::cout << "[CLEANUP] Restoring EO resolution to " << baselineWidth << "x" << baselineHeight
              << " (level " << baselineLevel->level << ")" << std::endl;
    EXPECT_TRUE(setAndVerifyResolution(EO_CAMERA_ID, *baselineLevel))
        << "Could not confirm EO resolution restored to original value.";
}

// ==================== IR ====================
// Skipped automatically if this bench has no IR camera -- getResolution()
// simply won't get a reply for IR_CAMERA_ID.

class StreamResolutionIRTest : public PayloadTest {
protected:
    void SetUp() override {
        double w, h;
        if (!getResolution(IR_CAMERA_ID, w, h, 2000)) {
            GTEST_SKIP() << "No response for IR stream -- this bench likely has no IR camera.";
        }
    }
};

TEST_F(StreamResolutionIRTest, SetResolution_1280x1024_Verified)
{
    EXPECT_TRUE(setAndVerifyResolution(IR_CAMERA_ID, IR_RES_1280x1024))
        << "IR resolution did not read back as 1280x1024 after setting level 0.";
}

TEST_F(StreamResolutionIRTest, SetResolution_640x512_Verified)
{
    EXPECT_TRUE(setAndVerifyResolution(IR_CAMERA_ID, IR_RES_640x512))
        << "IR resolution did not read back as 640x512 after setting level 1.";
}

TEST_F(StreamResolutionIRTest, SetResolution_480x384_Verified)
{
    EXPECT_TRUE(setAndVerifyResolution(IR_CAMERA_ID, IR_RES_480x384))
        << "IR resolution did not read back as 480x384 after setting level 2.";
}

TEST_F(StreamResolutionIRTest, SetResolution_320x256_Verified)
{
    EXPECT_TRUE(setAndVerifyResolution(IR_CAMERA_ID, IR_RES_320x256))
        << "IR resolution did not read back as 320x256 after setting level 3.";
}

TEST_F(StreamResolutionIRTest, SetResolution_160x128_Verified)
{
    EXPECT_TRUE(setAndVerifyResolution(IR_CAMERA_ID, IR_RES_160x128))
        << "IR resolution did not read back as 160x128 after setting level 4.";
}

TEST_F(StreamResolutionIRTest, SetAndGetStreamResolution)
{
    double baselineWidth, baselineHeight;
    ASSERT_TRUE(getResolution(IR_CAMERA_ID, baselineWidth, baselineHeight))
        << "No response for baseline IR resolution.";
    std::cout << "[INFO] IR baseline resolution: " << baselineWidth << "x" << baselineHeight << std::endl;

    const ResolutionLevel* baselineLevel = &IR_RES_1280x1024;
    for (const auto* lvl : { &IR_RES_1280x1024, &IR_RES_640x512, &IR_RES_480x384,
                              &IR_RES_320x256, &IR_RES_160x128 }) {
        if (lvl->height == baselineHeight && lvl->width == baselineWidth) {
            baselineLevel = lvl;
            break;
        }
    }

    std::cout << "[TEST] Setting IR resolution to 640x512 (level 1)" << std::endl;
    ASSERT_TRUE(setAndVerifyResolution(IR_CAMERA_ID, IR_RES_640x512))
        << "IR resolution did not read back as 640x512 after setting.";

    double actualWidth, actualHeight;
    ASSERT_TRUE(getResolution(IR_CAMERA_ID, actualWidth, actualHeight));
    EXPECT_EQ(actualWidth, IR_RES_640x512.width);
    EXPECT_EQ(actualHeight, IR_RES_640x512.height);

    std::cout << "[CLEANUP] Restoring IR resolution to " << baselineWidth << "x" << baselineHeight
              << " (level " << baselineLevel->level << ")" << std::endl;
    EXPECT_TRUE(setAndVerifyResolution(IR_CAMERA_ID, *baselineLevel))
        << "Could not confirm IR resolution restored to original value.";
}