/**
 * @file test_stream_profile.cpp
 * @brief Tests setPayloadStreamProfile() for the documented encoder profiles.
 *
 * The SDK callback does not expose a profile value, so verification uses the
 * command-specific ACK plus a post-command stream-information health check.
 */

#include "stream_test_helpers.h"

#include <chrono>
#include <iostream>
#include <thread>

namespace st = stream_test;

namespace {

struct ProfileCase {
    uint32_t level;
    const char* name;
};

constexpr ProfileCase kProfiles[] = {
    {0, "LowLatency"},
    {1, "Balance"},
    {2, "HighPerformance"},
};

// Calling setPayloadStreamProfile() selects the encoder's Profile mode.
// The public SDK has no separate setEncoderMode() API; setting a bitrate uses
// stream selector 0 and returns the encoder to Standard mode. These defaults
// match the documented SDK examples and the device's normal stream settings.
constexpr uint32_t kEoStandardBitrateBps = 4000000;
constexpr uint32_t kIrStandardBitrateBps = 2000000;

bool setProfileAndCheckHealth(uint32_t streamId, const ProfileCase& profile) {
    std::cout << "[TEST] stream=" << streamId << " profile=" << profile.name
              << " (level " << profile.level << ")" << std::endl;
    if (!st::sendUser4AndWaitForAcceptedAck([=] {
            g_payload->setPayloadStreamProfile(streamId, profile.level);
        })) {
        return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    st::Snapshot info;
    return st::getSnapshot(streamId, info) && st::isValid(info);
}

}  // namespace

class StreamProfileTest : public PayloadTest {
protected:
    bool setProfile(uint32_t streamId, const ProfileCase& profile) {
        if (streamId == st::kEoStreamId) {
            eoProfileChanged_ = true;
        } else if (streamId == st::kIrStreamId) {
            irProfileChanged_ = true;
        }
        return setProfileAndCheckHealth(streamId, profile);
    }

    void TearDown() override {
        // TearDown runs even after ASSERT_* aborts the test body, so the real
        // device is not accidentally left in Profile encoder mode.
        if (eoProfileChanged_) {
            EXPECT_TRUE(restoreStandardEncoderMode(
                st::kEoStreamId, kEoStandardBitrateBps))
                << "Could not restore the EO encoder to Standard mode.";
        }
        if (irProfileChanged_) {
            EXPECT_TRUE(restoreStandardEncoderMode(
                st::kIrStreamId, kIrStandardBitrateBps))
                << "Could not restore the IR encoder to Standard mode.";
        }
    }

private:
    bool restoreStandardEncoderMode(uint32_t streamId,
                                    uint32_t standardBitrateBps) {
        std::cout << "[CLEANUP] Restoring stream " << streamId
                  << " to Standard encoder mode at " << standardBitrateBps
                  << " bps" << std::endl;

        // A bitrate command switches the device from Profile mode back to
        // Standard mode. Verify its exposed bitrate because stream telemetry
        // does not include the encoder-mode field itself.
        for (int attempt = 1; attempt <= 3; ++attempt) {
            g_payload->setPayloadStreamBitrate(streamId, standardBitrateBps);
            std::this_thread::sleep_for(std::chrono::milliseconds(750));

            st::Snapshot info;
            if (st::getSnapshot(streamId, info, 2500) && st::isValid(info) &&
                info.bitrate == standardBitrateBps) {
                return true;
            }
            std::cout << "[CLEANUP] Restore attempt " << attempt
                      << " did not receive valid stream information." << std::endl;
        }
        return false;
    }

    bool eoProfileChanged_ = false;
    bool irProfileChanged_ = false;
};

TEST_F(StreamProfileTest, EO_LowLatencyBalanceHighPerformance_Accepted) {
    for (const auto& profile : kProfiles) {
        ASSERT_TRUE(setProfile(st::kEoStreamId, profile))
            << "EO rejected or stopped responding for profile " << profile.name;
    }
}

TEST_F(StreamProfileTest, IR_LowLatencyBalanceHighPerformance_AcceptedWhenAvailable) {
    st::Snapshot baseline;
    if (!st::getSnapshot(st::kIrStreamId, baseline, 2500)) {
        GTEST_SKIP() << "No IR stream 2 is available on this payload.";
    }
    for (const auto& profile : kProfiles) {
        ASSERT_TRUE(setProfile(st::kIrStreamId, profile))
            << "IR rejected or stopped responding for profile " << profile.name;
    }
}

TEST_F(StreamProfileTest, BalanceProfile_RepeatedCommandRemainsResponsive) {
    ASSERT_TRUE(setProfile(st::kEoStreamId, kProfiles[1]));
    EXPECT_TRUE(setProfile(st::kEoStreamId, kProfiles[1]));
}
