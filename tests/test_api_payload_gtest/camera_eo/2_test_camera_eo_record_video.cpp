#include "api_camera_eo_test_helper.h"

namespace cet = camera_eo_test;

class CameraEoRecordVideoTest : public cet::CameraEoTest {};
// class ManualCameraEoRecordVideoTest : public cet::CameraEoTest {};

TEST_F(CameraEoRecordVideoTest,
       SetPayloadCameraRecordVideoStartAndStop_ChangesVideoStatus) {
    ASSERT_TRUE(cet::setCameraMode(CAMERA_MODE_VIDEO));
#ifndef ZIO
    ASSERT_TRUE(cet::setCameraParam(PAYLOAD_CAMERA_RECORD_SRC,
                                     PAYLOAD_CAMERA_RECORD_BOTH));
#endif

    const uint64_t startAckSeq = getCommandAckSeq(MAV_CMD_VIDEO_START_CAPTURE);
    g_payload->setPayloadCameraRecordVideoStart();

    cet::CaptureStatus started;
    ASSERT_TRUE(cet::waitForVideoState(true, 8000, &started))
        << "video_status did not become active.";

    std::this_thread::sleep_for(std::chrono::seconds(2));
    cet::CaptureStatus later;
    ASSERT_TRUE(cet::readCaptureStatus(later));
    EXPECT_GT(later.recordingMs, started.recordingMs);

    AckInfo startAck;
    if (waitForCommandAck(MAV_CMD_VIDEO_START_CAPTURE, startAckSeq,
                          startAck, 1500)) {
        EXPECT_TRUE(startAck.result == MAV_RESULT_ACCEPTED ||
                    startAck.result == MAV_RESULT_IN_PROGRESS);
    }

    const uint64_t stopAckSeq = getCommandAckSeq(MAV_CMD_VIDEO_STOP_CAPTURE);
    g_payload->setPayloadCameraRecordVideoStop();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ASSERT_TRUE(cet::waitForVideoState(false))
        << "video_status did not return to idle.";

    AckInfo stopAck;
    if (waitForCommandAck(MAV_CMD_VIDEO_STOP_CAPTURE, stopAckSeq,
                          stopAck, 1500)) {
        EXPECT_TRUE(stopAck.result == MAV_RESULT_ACCEPTED ||
                    stopAck.result == MAV_RESULT_IN_PROGRESS);
    }
}


// Manual test for camera record video start and stop
// TEST_F(ManualCameraEoRecordVideoTest,
//        Manual_camera_record_video_status) {
//     cet::CaptureStatus status;
//     ASSERT_TRUE(cet::readCaptureStatus(status, 3000));
//     std::cout << "readCaptureStatus: image=" << status.image
//               << ", video=" << status.video
//               << ", count=" << status.count
//               << ", recordingMs=" << status.recordingMs
//               << std::endl;
// }

// TEST_F(ManualCameraEoRecordVideoTest, Manual_camera_record_video_start) {
// #ifndef ZIO
//     ASSERT_TRUE(cet::setCameraParam(PAYLOAD_CAMERA_RECORD_SRC,
//                                     PAYLOAD_CAMERA_RECORD_BOTH));
// #endif
//     std::this_thread::sleep_for(std::chrono::milliseconds(200));
//     g_payload->setPayloadCameraMode(CAMERA_MODE_VIDEO);
//     std::this_thread::sleep_for(std::chrono::milliseconds(200));
//     g_payload->setPayloadCameraRecordVideoStart();
//     // std::this_thread::sleep_for(std::chrono::milliseconds(200));
// }
// TEST_F(ManualCameraEoRecordVideoTest, Manual_camera_record_video_stop) {
//     g_payload->setPayloadCameraRecordVideoStop();
//     std::this_thread::sleep_for(std::chrono::milliseconds(200));
// }
