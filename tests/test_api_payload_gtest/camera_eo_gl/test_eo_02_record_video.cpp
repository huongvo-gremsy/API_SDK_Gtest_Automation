/**
 * Sheet row 2: Quay / dung video EO - setPayloadCameraRecordVideoStart() / Stop()
 * Support: VIO x | ORUSL x | MB1 x | ZIO x
 * Example: examples/camera_eo_record_video.cpp
 */

#include "camera_eo_test_helpers.h"

using namespace eo;

// Recording needs storage; without it the whole suite is skipped.
class EO_RecordVideo : public EoCameraTest {
protected:
    void SetUp() override {
        EO_SKIP_WITHOUT_STORAGE();

        EoCameraTest::SetUp();
    }
};

// Check that setPayloadCameraRecordVideoStart() / Stop() really start and stop a recording.
TEST_F(EO_RecordVideo, SetPayloadCameraRecordVideoStartAndStop) {
    // Best effort: MB1 never reports the mode back, so the result is only printed.
    bool modeConfirmed = setCameraMode(CAMERA_MODE_VIDEO);

    if (modeConfirmed) {
        std::cout << "[  INFO  ] setPayloadCameraMode(VIDEO): confirmed by the payload\n";
    } else {
        std::cout << "[  INFO  ] setPayloadCameraMode(VIDEO): sent, not confirmed (best effort)\n";
    }

    CaptureStatus before;

    ASSERT_TRUE(readCaptureStatus(before))
        << "getPayloadCaptureStatus(): no reply";

    ASSERT_EQ(before.video, 0)
        << "video recording already in progress";

    std::cout << "[  INFO  ] before record: video_status=" << before.video
              << " recording_time_ms=" << before.recordMs << "\n";

    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_VIDEO_START_CAPTURE,
            [] {
                g_payload->setPayloadCameraRecordVideoStart();
            },
            "record start"
        )
    ) << "setPayloadCameraRecordVideoStart() was not accepted";

    std::cout << "[  INFO  ] setPayloadCameraRecordVideoStart(): ACK accepted\n";

    // Check that the camera is recording.
    CaptureStatus last;

    bool videoIsRecording = waitCaptureStatus(
        [](const CaptureStatus& status) {
            return status.video == 1;
        },
        20000,
        &last
    );

    EXPECT_TRUE(videoIsRecording)
        << "video_status did not become 1 after record start";

    std::cout << "[  INFO  ] video_status after start: " << last.video << " (1 = recording)\n";

    // Check that the recorder really uses the EO source. REC_SRC is the live
    // value the recorder applies; C_V_REC is only the stored setting. The file
    // on the card follows REC_SRC, so a mismatch means the wrong video is recorded.
    double liveSource = -1;

    if (readLiveRecordSource(liveSource)) {
        std::cout << "[  INFO  ] REC_SRC while recording: " << liveSource << " (0 = both, 1 = EO, 2 = IR)\n";

        EXPECT_EQ(liveSource, PAYLOAD_CAMERA_RECORD_EO)
            << "payload records source " << liveSource << " instead of 1 (EO): C_V_REC was set but the firmware did not apply it";
    } else {
        std::cout << "[  INFO  ] REC_SRC not answered, record source not verified\n";
    }

    // Check that the recording time is advancing.
    if (videoIsRecording) {
        sleepMs(3000);

        CaptureStatus during;

        if (readCaptureStatus(during)) {
            std::cout << "[  INFO  ] recording_time_ms = "
                      << during.recordMs << "\n";

            EXPECT_GT(during.recordMs, 0.0)
                << "recording_time_ms is not advancing while recording";
        }
    }

    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_VIDEO_STOP_CAPTURE,
            [] {
                g_payload->setPayloadCameraRecordVideoStop();
            },
            "record stop"
        )
    ) << "setPayloadCameraRecordVideoStop() was not accepted";

    std::cout << "[  INFO  ] setPayloadCameraRecordVideoStop(): ACK accepted\n";

    // Check that the camera stopped recording.
    bool videoIsIdle = waitCaptureStatus(
        [](const CaptureStatus& status) {
            return status.video == 0;
        },
        20000,
        &last
    );

    EXPECT_TRUE(videoIsIdle)
        << "video_status did not return to 0 after record stop";

    std::cout << "[  INFO  ] video_status after stop: " << last.video << " (0 = idle)\n";

    double recordSrc = 0;

    ASSERT_TRUE(readRecordSource(recordSrc))
        << "C_V_REC: no reply";

    std::cout << "[  INFO  ] C_V_REC = " << recordSrc << " (0 = both, 1 = EO, 2 = IR)\n";

}

// No "stop while idle" test: like the Gremsy examples, the tests never send
// setPayloadCameraRecordVideoStop() to an idle camera. On VIO that command
// freezes the record source (REC_SRC stops following C_V_REC until a real
// recording is made), so it is a firmware issue to report, not a test to run.
