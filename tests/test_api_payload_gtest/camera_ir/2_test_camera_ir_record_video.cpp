#include "api_camera_ir_test_helper.h"

namespace cit = camera_ir_test;
class CameraIrTestRecordVideo : public CameraIrTest{
    protected:
    void TearDown() override {
        // std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // gap before next case
        g_payload->setPayloadCameraRecordVideoStop();
        CameraIrTest::TearDown();
    }
};
class ManualCameraIrRecordVideoTest : public testing :: Test {
};

TEST_F(CameraIrTestRecordVideo, RecordVideoStartAndStopChangesVideoState) {
    ASSERT_TRUE(cit::setCameraMode(CAMERA_MODE_VIDEO));
    ASSERT_TRUE(cit::setCameraParam(PAYLOAD_CAMERA_RECORD_SRC,
                                     PAYLOAD_CAMERA_RECORD_IR));

    g_payload->setPayloadCameraRecordVideoStart();
    cit::CaptureStatus started;
    ASSERT_TRUE(cit::waitForVideoState(true, 3000, &started));

    // Record for 30 seconds
    constexpr int kRecordDurationMs = 10000;
    constexpr int kPollIntervalMs = 500;
    int elapsed = 0;
    while (elapsed < kRecordDurationMs) {
        std::this_thread::sleep_for(std::chrono::milliseconds(kPollIntervalMs));
        elapsed += kPollIntervalMs;
        cit::CaptureStatus status;
        if (cit::readCaptureStatus(status, 1200)) {
            ASSERT_EQ(status.video, 1) << "Recording stopped unexpectedly at " << elapsed << "ms";
        }
    }

    g_payload->setPayloadCameraRecordVideoStop();
    cit::CaptureStatus stopped;
    ASSERT_TRUE(cit::waitForVideoState(false, 8000, &stopped))
        << "Video did not stop within timeout.";
    EXPECT_EQ(stopped.video, 0);
}
TEST_F(CameraIrTestRecordVideo, RecordVideoStartAndStopChangesVideoState_30s) {
    ASSERT_TRUE(cit::setCameraMode(CAMERA_MODE_VIDEO));
    ASSERT_TRUE(cit::setCameraParam(PAYLOAD_CAMERA_RECORD_SRC,
                                     PAYLOAD_CAMERA_RECORD_IR));

    g_payload->setPayloadCameraRecordVideoStart();
    cit::CaptureStatus started;
    ASSERT_TRUE(cit::waitForVideoState(true, 3000, &started));

    // Record for 30 seconds
    constexpr int kRecordDurationMs = 30000;
    constexpr int kPollIntervalMs = 1000;
    int elapsed = 0;
    while (elapsed < kRecordDurationMs) {
        std::this_thread::sleep_for(std::chrono::milliseconds(kPollIntervalMs));
        elapsed += kPollIntervalMs;
        cit::CaptureStatus status;
        if (cit::readCaptureStatus(status, 1200)) {
            ASSERT_EQ(status.video, 1) << "Recording stopped unexpectedly at " << elapsed << "ms";
        }
    }

    g_payload->setPayloadCameraRecordVideoStop();
    cit::CaptureStatus stopped;
    ASSERT_TRUE(cit::waitForVideoState(false, 8000, &stopped))
        << "Video did not stop within timeout.";
    EXPECT_EQ(stopped.video, 0);
}


//---------------------------------------------------------------------------
// Manual test for camera record video start and stop
// TEST_F(ManualCameraIrRecordVideoTest,
//        Manual_camera_record_video_status) {
//      cit::CaptureStatus status;
//     ASSERT_TRUE(cit::readCaptureStatus(status, 3000))
//         << "No capture-status response received from the IR camera.";
//     std::cout << "readCaptureStatus: image=" << status.image
//               << ", video=" << status.video
//               << ", count=" << status.count
//               << ", recordingMs=" << status.recordingMs
//               << std::endl;
// }

// TEST_F(ManualCameraIrRecordVideoTest, Manual_camera_record_video_start) {
// // #ifndef ZIO
// //     ASSERT_TRUE(cet::setCameraParam(PAYLOAD_CAMERA_RECORD_SRC,
// //                                     PAYLOAD_CAMERA_RECORD_BOTH));
// // #endif
//     // std::this_thread::sleep_for(std::chrono::milliseconds(200));
//     g_payload->setPayloadCameraMode(CAMERA_MODE_VIDEO);
//     std::this_thread::sleep_for(std::chrono::milliseconds(200));
//     g_payload->setPayloadCameraRecordVideoStart();
//     std::this_thread::sleep_for(std::chrono::milliseconds(200));
// }
// TEST_F(ManualCameraIrRecordVideoTest, Manual_camera_record_video_stop) {
//     g_payload->setPayloadCameraRecordVideoStop();
//     std::this_thread::sleep_for(std::chrono::milliseconds(200));
// }

// TEST_F(ManualCameraIrRecordVideoTest, Manual_set_camera_param_record_source) {
//     g_payload->setPayloadCameraParam(PAYLOAD_CAMERA_RECORD_SRC,
//                                      PAYLOAD_CAMERA_RECORD_IR, PARAM_TYPE_UINT32);
//     std::this_thread::sleep_for(std::chrono::milliseconds(200));
// }