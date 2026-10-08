/** @file test_payload_video_record_status.cpp */
#include "payload_example_test_helpers.h"
#include "../camera/camera_eo/camera_eo_test_helpers.h"

namespace pet = payload_example_test;
namespace cet = camera_eo_test;

class PayloadVideoRecordStatusExampleTest : public cet::CameraEoTest {};

TEST_F(PayloadVideoRecordStatusExampleTest,
       StartAndStop_EmitRecordJsonAndCaptureStatus) {
    ASSERT_TRUE(cet::setCameraModeStateOrAck(CAMERA_MODE_VIDEO));
#ifndef ZIO
    ASSERT_TRUE(cet::setUint32Param(PAYLOAD_CAMERA_RECORD_SRC,
                                    PAYLOAD_CAMERA_RECORD_BOTH));
#endif
    ASSERT_TRUE(cet::stopRecording());

    uint64_t recordSeq = g_cb.recordInfoSeq.load();
    g_payload->setPayloadCameraRecordVideoStart();
    ASSERT_TRUE(cet::waitForRecording(true, 8000))
        << "CAMERA_CAPTURE_STATUS did not report active recording.";

    std::string activeText;
    ASSERT_TRUE(pet::waitForRecordText(recordSeq, activeText, 10000))
        << "regPayloadRecordInfoChanged received no recording JSON.";
    EXPECT_NE(activeText.find("rec_status"), std::string::npos);
    EXPECT_NE(activeText.find("rec_time"), std::string::npos);

    recordSeq = g_cb.recordInfoSeq.load();
    g_payload->setPayloadCameraRecordVideoStop();
    ASSERT_TRUE(cet::waitForRecording(false, 10000));

    std::string stoppedText;
    ASSERT_TRUE(pet::waitForRecordText(recordSeq, stoppedText, 12000))
        << "No final record-info JSON after stopping.";
    EXPECT_NE(stoppedText.find("rec_status"), std::string::npos);
    EXPECT_NE(stoppedText.find("url"), std::string::npos)
        << "Final status should expose the recorded media URL.";
}
