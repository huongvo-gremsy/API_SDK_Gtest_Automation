/**
 * @file test_stream_information.cpp
 * @brief Tests getPayloadCameraStreamingInformation() and its callback data.
 */

#include "stream_test_helpers.h"

#include <iostream>

namespace st = stream_test;

class StreamInformationTest : public PayloadTest {};

TEST_F(StreamInformationTest, DefaultStream_EventArrivesAndFieldsAreValid) {
    st::Snapshot info;
    ASSERT_TRUE(st::getSnapshot(st::kDefaultStreamId, info))
        << "No VIDEO_STREAM_INFORMATION response for the default stream.";
    EXPECT_TRUE(st::isValid(info));
    EXPECT_FALSE(info.uri.empty());
    std::cout << "[INFO] default stream: id=" << info.streamId
              << " type=" << info.type << " " << info.width << "x"
              << info.height << " bitrate=" << info.bitrate
              << " uri=" << info.uri << std::endl;
}

TEST_F(StreamInformationTest, EOStream_IdAndFieldsMatchRequest) {
    st::Snapshot info;
    ASSERT_TRUE(st::getSnapshot(st::kEoStreamId, info))
        << "No VIDEO_STREAM_INFORMATION response for EO stream 1.";
    EXPECT_EQ(info.streamId, st::kEoStreamId);
    EXPECT_TRUE(st::isValid(info));
    EXPECT_FALSE(info.uri.empty());
}

TEST_F(StreamInformationTest, IRStream_ValidWhenAvailable) {
    st::Snapshot info;
    if (!st::getSnapshot(st::kIrStreamId, info, 2500)) {
        GTEST_SKIP() << "No IR stream 2 is available on this payload.";
    }
    EXPECT_EQ(info.streamId, st::kIrStreamId);
    EXPECT_TRUE(st::isValid(info));
    EXPECT_FALSE(info.uri.empty());
}

TEST_F(StreamInformationTest, RepeatedEOQuery_IsStable) {
    st::Snapshot first;
    st::Snapshot second;
    ASSERT_TRUE(st::getSnapshot(st::kEoStreamId, first));
    ASSERT_TRUE(st::getSnapshot(st::kEoStreamId, second));
    EXPECT_EQ(second.streamId, first.streamId);
    EXPECT_EQ(second.type, first.type);
    EXPECT_EQ(second.width, first.width);
    EXPECT_EQ(second.height, first.height);
    EXPECT_EQ(second.uri, first.uri);
    EXPECT_GT(second.bitrate, 0u);
}

TEST_F(StreamInformationTest, CallbackRegistration_IsActive) {
    ASSERT_TRUE(static_cast<bool>(g_payload->__notifyPayloadStreamChanged));
    const uint64_t seq = g_cb.streamSeq.load();
    g_payload->getPayloadCameraStreamingInformation(st::kEoStreamId);
    EXPECT_TRUE(waitForSeq(g_cb.streamSeq, seq, 5000));
}
