/** @file test_payload_stream_controls.cpp */
#include "payload_example_test_helpers.h"
#include "../telemetry/udp_telemetry_capture.h"

namespace st = stream_test;
namespace tt = telemetry_test;

class PayloadStreamControlEncodingTest : public tt::TelemetryPacketTest {
protected:
    template <typename Send>
    void expectUser4(Send send, float camera, float selector, float value) {
        mavlink_message_t message{};
        ASSERT_TRUE(capture_->sendAndCapture(
            MAVLINK_MSG_ID_COMMAND_LONG, send, message));
        mavlink_command_long_t command{};
        mavlink_msg_command_long_decode(&message, &command);
        EXPECT_EQ(command.command, MAV_CMD_USER_4);
        EXPECT_FLOAT_EQ(command.param1, 4.0f);
        EXPECT_FLOAT_EQ(command.param2, 2.0f);
        EXPECT_FLOAT_EQ(command.param3, camera);
        EXPECT_FLOAT_EQ(command.param4, selector);
        EXPECT_FLOAT_EQ(command.param5, value);
    }
};

TEST_F(PayloadStreamControlEncodingTest, Bitrate_EoAndIrEncode) {
    expectUser4([](PayloadSdkInterface& sdk) {
        sdk.setPayloadStreamBitrate(1, 4000000);
    }, 1, 0, 4000000);
    expectUser4([](PayloadSdkInterface& sdk) {
        sdk.setPayloadStreamBitrate(2, 2000000);
    }, 2, 0, 2000000);
}

TEST_F(PayloadStreamControlEncodingTest, Resolution_AllDocumentedLevelsEncode) {
    for (uint32_t level = 0; level <= 2; ++level) {
        expectUser4([=](PayloadSdkInterface& sdk) {
            sdk.setPayloadStreamResolution(1, level);
        }, 1, 1, static_cast<float>(level));
    }
    for (uint32_t level = 0; level <= 4; ++level) {
        expectUser4([=](PayloadSdkInterface& sdk) {
            sdk.setPayloadStreamResolution(2, level);
        }, 2, 1, static_cast<float>(level));
    }
}

TEST_F(PayloadStreamControlEncodingTest, Profile_AllDocumentedLevelsEncode) {
    for (uint32_t camera : {1u, 2u}) {
        for (uint32_t profile = 0; profile <= 2; ++profile) {
            expectUser4([=](PayloadSdkInterface& sdk) {
                sdk.setPayloadStreamProfile(camera, profile);
            }, static_cast<float>(camera), 2,
               static_cast<float>(profile));
        }
    }
}

class PayloadStreamBitrateHardwareTest : public PayloadTest {
protected:
    void SetUp() override {
        ASSERT_TRUE(st::getSnapshot(st::kEoStreamId, baseline_, 5000));
        ASSERT_TRUE(st::isValid(baseline_));
        haveBaseline_ = true;
    }
    void TearDown() override {
        if (haveBaseline_) {
            g_payload->setPayloadStreamBitrate(
                st::kEoStreamId, baseline_.bitrate);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            st::Snapshot restored;
            if (!st::getSnapshot(st::kEoStreamId, restored, 5000) ||
                restored.bitrate != baseline_.bitrate) {
                ADD_FAILURE() << "Could not restore EO stream bitrate.";
            }
        }
    }
    st::Snapshot baseline_;
    bool haveBaseline_ = false;
};

TEST_F(PayloadStreamBitrateHardwareTest, EoFourMbps_ReadBackAndRestore) {
    g_payload->setPayloadStreamBitrate(st::kEoStreamId, 4000000);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    st::Snapshot changed;
    ASSERT_TRUE(st::getSnapshot(st::kEoStreamId, changed, 5000));
    EXPECT_EQ(changed.bitrate, 4000000u);

    EXPECT_GT(baseline_.bitrate, 0u);
}
