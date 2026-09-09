#include "../common/payload_test_fixture.h"


#include <cstdint>
#include <iostream>
#include <thread>
#include <chrono>

class StreamBitrateTest : public PayloadTest
{
};


TEST_F(StreamBitrateTest, GetEOStreamBitrate)
{ // StreamBitrateTest.GetEOStreamBitrate
    uint64_t seq = g_cb.streamSeq.load();

    g_payload->getPayloadCameraStreamingInformation(1);

    ASSERT_TRUE(
        waitForSeq(g_cb.streamSeq, seq, 5000)
    ) << "No stream bitrate response received.";

    std::lock_guard<std::mutex> lock(g_cb.m);

    std::cout
        << "[INFO] EO stream URI: "
        << g_cb.lastStreamUri
        << std::endl;

    std::cout
        << "[INFO] EO stream values: "
        << g_cb.streamValues[0] << ", "
        << g_cb.streamValues[1] << ", "
        << g_cb.streamValues[2] << ", "
        << g_cb.streamValues[3] << ", "
        << g_cb.streamValues[4]
        << std::endl;

    SUCCEED();
}

TEST_F(StreamBitrateTest, Getter_TriggersAsynchronousBitrateCallback)
{
    const uint64_t seq = g_cb.streamSeq.load();

    // The current SDK getter dispatches the request and returns immediately;
    // the actual bitrate is delivered by regPayloadStreamChanged().
    const uint32_t immediateResult = g_payload->getPayloadStreamBitrate();
    ASSERT_TRUE(waitForSeq(g_cb.streamSeq, seq, 5000))
        << "getPayloadStreamBitrate() did not produce a stream callback.";

    uint32_t callbackBitrate = 0;
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        callbackBitrate = static_cast<uint32_t>(g_cb.streamValues[3]);
    }
    std::cout << "[INFO] immediate getter result=" << immediateResult
              << ", asynchronous callback bitrate=" << callbackBitrate
              << " bps" << std::endl;
    EXPECT_GT(callbackBitrate, 0u);
}

// ---- Set, no round-trip baseline (upper documented boundary) ----

TEST_F(StreamBitrateTest, SetStreamBitrate)
{ // StreamBitrateTest.SetStreamBitrate
    constexpr uint32_t CAMERA_ID = 1;
    constexpr uint32_t TARGET_BITRATE = 16000000; // upper documented bound

    uint64_t seq = g_cb.streamSeq.load();

    std::cout << "[TEST] Setting bitrate to " << TARGET_BITRATE << " bps" << std::endl;
    g_payload->setPayloadStreamBitrate(CAMERA_ID, TARGET_BITRATE);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    seq = g_cb.streamSeq.load();
    g_payload->getPayloadCameraStreamingInformation(CAMERA_ID);
    ASSERT_TRUE(waitForSeq(g_cb.streamSeq, seq, 3000)) << "No response after setting bitrate.";

    double actualBitrate;
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        actualBitrate = g_cb.streamValues[3];
    }
    std::cout << "[INFO] Actual bitrate: " << actualBitrate << " bps" << std::endl;

    EXPECT_EQ(static_cast<uint32_t>(actualBitrate), TARGET_BITRATE);
}

// ---- Set, lower documented boundary ----

TEST_F(StreamBitrateTest, SetStreamBitrate_MinBoundary_512kbps)
{
    constexpr uint32_t CAMERA_ID = 1;
    constexpr uint32_t TARGET_BITRATE = 512000; // lower documented bound

    std::cout << "[TEST] Setting bitrate to " << TARGET_BITRATE << " bps" << std::endl;
    g_payload->setPayloadStreamBitrate(CAMERA_ID, TARGET_BITRATE);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    uint64_t seq = g_cb.streamSeq.load();
    g_payload->getPayloadCameraStreamingInformation(CAMERA_ID);
    ASSERT_TRUE(waitForSeq(g_cb.streamSeq, seq, 3000)) << "No response after setting bitrate.";

    double actualBitrate;
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        actualBitrate = g_cb.streamValues[3];
    }
    std::cout << "[INFO] Actual bitrate: " << actualBitrate << " bps" << std::endl;

    EXPECT_EQ(static_cast<uint32_t>(actualBitrate), TARGET_BITRATE);

    // Restore to a known-good value so later tests aren't left at the
    // minimum bitrate (learned from the earlier leftover-state incident).
    g_payload->setPayloadStreamBitrate(CAMERA_ID, 4000000);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

// ---- Full set/get/restore round trip, EO ----

TEST_F(StreamBitrateTest, SetAndGetStreamBitrate)
{
    constexpr uint32_t CAMERA_ID = 1;
    constexpr uint32_t TARGET_BITRATE = 14000000;

    // 1. Read original bitrate
    uint64_t seq = g_cb.streamSeq.load();
    g_payload->getPayloadCameraStreamingInformation(CAMERA_ID);
    ASSERT_TRUE(waitForSeq(g_cb.streamSeq, seq, 3000)) << "No response for original bitrate.";

    double originalBitrate;
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        originalBitrate = g_cb.streamValues[3];
    }
    std::cout << "[INFO] Original bitrate: " << originalBitrate << " bps" << std::endl;

    // 2. Set new bitrate
    std::cout << "[TEST] Setting bitrate to " << TARGET_BITRATE << " bps" << std::endl;
    g_payload->setPayloadStreamBitrate(CAMERA_ID, TARGET_BITRATE);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 3. Read bitrate again
    seq = g_cb.streamSeq.load();
    g_payload->getPayloadCameraStreamingInformation(CAMERA_ID);
    ASSERT_TRUE(waitForSeq(g_cb.streamSeq, seq, 3000)) << "No response after setting bitrate.";

    double actualBitrate;
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        actualBitrate = g_cb.streamValues[3];
    }
    std::cout << "[INFO] Actual bitrate: " << actualBitrate << " bps" << std::endl;

    EXPECT_EQ(static_cast<uint32_t>(actualBitrate), TARGET_BITRATE);

    // 4. Restore original bitrate
    std::cout << "[CLEANUP] Restoring bitrate to " << originalBitrate << " bps" << std::endl;
    g_payload->setPayloadStreamBitrate(CAMERA_ID, static_cast<uint32_t>(originalBitrate));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 5. Verify restoration
    seq = g_cb.streamSeq.load();
    g_payload->getPayloadCameraStreamingInformation(CAMERA_ID);
    ASSERT_TRUE(waitForSeq(g_cb.streamSeq, seq, 3000)) << "No response after restoring bitrate.";

    double restoredBitrate;
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        restoredBitrate = g_cb.streamValues[3];
    }
    EXPECT_EQ(static_cast<uint32_t>(restoredBitrate), static_cast<uint32_t>(originalBitrate));
}

// ---- IR device: skipped automatically if this bench has no IR camera ----

TEST_F(StreamBitrateTest, SetAndGetStreamBitrate_IR)
{
    constexpr uint32_t CAMERA_ID = 2;
    constexpr uint32_t TARGET_BITRATE = 2000000;

    // Probe reachability first -- getPayloadCameraStreamingInformation()
    // simply won't get a reply for CAMERA_ID=2 if there's no IR camera.
    uint64_t seq = g_cb.streamSeq.load();
    g_payload->getPayloadCameraStreamingInformation(CAMERA_ID);
    if (!waitForSeq(g_cb.streamSeq, seq, 2000)) {
        GTEST_SKIP() << "No response for IR stream -- this bench likely has no IR camera.";
    }

    double originalBitrate;
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        originalBitrate = g_cb.streamValues[3];
    }
    std::cout << "[INFO] IR original bitrate: " << originalBitrate << " bps" << std::endl;

    std::cout << "[TEST] Setting IR bitrate to " << TARGET_BITRATE << " bps" << std::endl;
    g_payload->setPayloadStreamBitrate(CAMERA_ID, TARGET_BITRATE);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    seq = g_cb.streamSeq.load();
    g_payload->getPayloadCameraStreamingInformation(CAMERA_ID);
    ASSERT_TRUE(waitForSeq(g_cb.streamSeq, seq, 3000)) << "No response after setting IR bitrate.";

    double actualBitrate;
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        actualBitrate = g_cb.streamValues[3];
    }
    std::cout << "[INFO] IR actual bitrate: " << actualBitrate << " bps" << std::endl;

    EXPECT_EQ(static_cast<uint32_t>(actualBitrate), TARGET_BITRATE);

    // Restore
    std::cout << "[CLEANUP] Restoring IR bitrate to " << originalBitrate << " bps" << std::endl;
    g_payload->setPayloadStreamBitrate(CAMERA_ID, static_cast<uint32_t>(originalBitrate));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    seq = g_cb.streamSeq.load();
    g_payload->getPayloadCameraStreamingInformation(CAMERA_ID);
    ASSERT_TRUE(waitForSeq(g_cb.streamSeq, seq, 3000)) << "No response after restoring IR bitrate.";

    double restoredBitrate;
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        restoredBitrate = g_cb.streamValues[3];
    }
    EXPECT_EQ(static_cast<uint32_t>(restoredBitrate), static_cast<uint32_t>(originalBitrate));
}

// ---- Out-of-spec exploration (findings, not assumptions -- see file header) ----

TEST_F(StreamBitrateTest, SetBitrate_BelowDocumentedMinimum_ObserveBehavior)
{
    constexpr uint32_t CAMERA_ID = 1;

    g_payload->setPayloadStreamBitrate(CAMERA_ID, 100000); // below the 512000 documented floor
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    uint64_t seq = g_cb.streamSeq.load();
    g_payload->getPayloadCameraStreamingInformation(CAMERA_ID);
    ASSERT_TRUE(waitForSeq(g_cb.streamSeq, seq, 3000))
        << "No stream info response after sending an out-of-spec low bitrate.";

    double actual;
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        actual = g_cb.streamValues[3];
    }
    std::cout << "[FINDING] Requested 100000 bps (below documented minimum); "
                 "payload reports " << actual << " bps." << std::endl;

    // Restore to a known-good value.
    g_payload->setPayloadStreamBitrate(CAMERA_ID, 4000000);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST_F(StreamBitrateTest, SetBitrate_AboveDocumentedMaximum_ObserveBehavior)
{
    constexpr uint32_t CAMERA_ID = 1;

    g_payload->setPayloadStreamBitrate(CAMERA_ID, 20000000); // above the 16000000 documented ceiling
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    uint64_t seq = g_cb.streamSeq.load();
    g_payload->getPayloadCameraStreamingInformation(CAMERA_ID);
    ASSERT_TRUE(waitForSeq(g_cb.streamSeq, seq, 3000))
        << "No stream info response after sending an out-of-spec high bitrate.";

    double actual;
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        actual = g_cb.streamValues[3];
    }
    std::cout << "[FINDING] Requested 20000000 bps (above documented maximum); "
                 "payload reports " << actual << " bps." << std::endl;

    g_payload->setPayloadStreamBitrate(CAMERA_ID, 4000000);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}
