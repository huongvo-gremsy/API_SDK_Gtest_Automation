// #ifndef API_FEATURE_TEST_HELPER_H_
// #define API_FEATURE_TEST_HELPER_H_

// #include "../common/payload_test_fixture.h"

// #include <chrono>
// #include <cstdlib>
// #include <cstring>
// #include <functional>
// #include <iostream>
// #include <mutex>
// #include <string>
// #include <thread>

// namespace feature_test {

// // ---------------------------------------------------------------------------
// // Product support, same convention as gb_test_helpers.h / the QA sheet:
// // columns are VIO ORUSL MB1 ZIO. 'x' supported, '?' unconfirmed, '-' n/a.
// // Duplicated here (rather than shared) to match how camera_eo_test_helper.h
// // and gb_test_helpers.h each keep their own copy.
// // ---------------------------------------------------------------------------
// inline char supportOnThisProduct(const char* columns) {
// #if defined(VIO)
//     return columns[0];
// #elif defined(ORUSL)
//     return columns[1];
// #elif defined(MB1)
//     return columns[2];
// #elif defined(ZIO)
//     return columns[3];
// #else
//     (void)columns;
//     return '-';
// #endif
// }

// inline bool runUnverified() {
//     const char* v = std::getenv("PAYLOAD_TEST_UNVERIFIED");
//     return v && v[0] == '1';
// }

// #define FEATURE_SKIP_UNLESS_SUPPORTED(columns)                                                   \
//     do {                                                                                         \
//         const char s = feature_test::supportOnThisProduct(columns);                              \
//         if (s == '-') GTEST_SKIP() << "not available on this product (sheet: " columns ")";      \
//         if (s == '?' && !feature_test::runUnverified())                                          \
//             GTEST_SKIP() << "not confirmed on this product (sheet: " columns                     \
//                          "); set PAYLOAD_TEST_UNVERIFIED=1 to run it";                           \
//     } while (0)

// // ---------------------------------------------------------------------------
// // Camera mode: getPayloadCameraMode() / setPayloadCameraMode()
// // Reuses the same pattern as camera_eo_test_helper.h's readCameraMode/
// // setCameraMode (g_cb.cameraSettingsSeq / g_cb.cameraSettings[0]).
// // ---------------------------------------------------------------------------
// inline bool readCameraMode(double& mode, int timeoutMs = 3000) {
//     const uint64_t seq = g_cb.cameraSettingsSeq.load();
//     g_payload->getPayloadCameraMode();
//     if (!waitForSeq(g_cb.cameraSettingsSeq, seq, timeoutMs)) return false;

//     std::lock_guard<std::mutex> lock(g_cb.m);
//     mode = g_cb.cameraSettings[0];
//     return true;
// }

// inline bool setCameraMode(CAMERA_MODE mode, int timeoutMs = 4000) {
//     g_payload->setPayloadCameraMode(mode);
//     double actual = -1;
//     const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
//     while (std::chrono::steady_clock::now() < deadline) {
//         if (readCameraMode(actual, 1000) && static_cast<int>(actual) == static_cast<int>(mode)) {
//             return true;
//         }
//     }

//     // Fall back to checking COMMAND_ACK, same as camera_eo_test_helper.h's
//     // setCameraMode(), in case readback is slower than the ack.
//     const uint64_t seq = getCommandAckSeq(MAV_CMD_SET_CAMERA_MODE);
//     g_payload->setPayloadCameraMode(mode);
//     AckInfo ack;
//     return waitForCommandAck(MAV_CMD_SET_CAMERA_MODE, seq, ack, 1500) &&
//            (ack.result == MAV_RESULT_ACCEPTED || ack.result == MAV_RESULT_IN_PROGRESS);
// }

// // ---------------------------------------------------------------------------
// // Capture status / storage: getPayloadCaptureStatus() / getPayloadStorage()
// // ---------------------------------------------------------------------------
// struct CaptureStatus {
//     double image = 0;
//     double video = 0;
//     double count = 0;
//     double recordingMs = 0;
// };

// inline bool readCaptureStatus(CaptureStatus& status, int timeoutMs = 3000) {
//     const uint64_t seq = g_cb.cameraCaptureStatusSeq.load();
//     g_payload->getPayloadCaptureStatus();
//     if (!waitForSeq(g_cb.cameraCaptureStatusSeq, seq, timeoutMs)) return false;

//     std::lock_guard<std::mutex> lock(g_cb.m);
//     status.image = g_cb.cameraCaptureStatus[0];
//     status.video = g_cb.cameraCaptureStatus[1];
//     status.count = g_cb.cameraCaptureStatus[2];
//     status.recordingMs = g_cb.cameraCaptureStatus[3];
//     return true;
// }

// // STORAGE_INFORMATION fields, per MAVLink's common set: total/used/available
// // capacity in MiB and storage status. ASSUMPTION: g_cb exposes these as
// // storageSeq / storage[0..n], matching the cameraCaptureStatus pattern above.
// // Verify against your actual payload_test_fixture.h / g_cb definition --
// // adjust only the three lines inside this function if the field names or
// // array layout differ.
// struct StorageStatus {
//     double totalCapacityMiB = -1;
//     double usedCapacityMiB = -1;
//     double availableCapacityMiB = -1;
//     double status = -1;  // STORAGE_STATUS_*
// };

// inline bool readStorageStatus(StorageStatus& storage, int timeoutMs = 3000) {
//     const uint64_t seq = g_cb.cameraStorageInfoSeq.load();
//     g_payload->getPayloadStorage();
//     if (!waitForSeq(g_cb.cameraStorageInfoSeq, seq, timeoutMs)) return false;

//     std::lock_guard<std::mutex> lock(g_cb.m);
//     storage.totalCapacityMiB = g_cb.cameraStorageInfo[0];
//     storage.usedCapacityMiB = g_cb.cameraStorageInfo[1];
//     storage.availableCapacityMiB = g_cb.cameraStorageInfo[2];
//     storage.status = g_cb.cameraStorageInfo[3];
//     return true;
// }

// inline bool readCameraInformation(CameraInfoStatus& info, int timeoutMs = 3000) {
//     const uint64_t seq = g_cb.cameraInfoSeq.load();
//     g_payload->getPayloadCameraInformation();
//     if (!waitForSeq(g_cb.cameraInfoSeq, seq, timeoutMs)) return false;

//     std::lock_guard<std::mutex> lock(g_cb.m);
//     info.capabilityFlags = g_cb.cameraInfoFlags;  // plain uint32_t, not an array
//     return true;
// }
// inline bool readStreamingInformation(StreamingInfoStatus& info, int timeoutMs = 4000) {
//     const uint64_t seq = g_cb.streamSeq.load();
//     g_payload->getPayloadCameraStreamingInformation();
//     if (!waitForSeq(g_cb.streamSeq, seq, timeoutMs)) return false;

//     std::lock_guard<std::mutex> lock(g_cb.m);
//     info.type = g_cb.streamValues[0];
//     info.resolutionV = g_cb.streamValues[1];
//     info.resolutionH = g_cb.streamValues[2];
//     info.uri = g_cb.lastStreamUri;
//     return true;
// }
// // ---------------------------------------------------------------------------
// // Camera information: getPayloadCameraInformation()
// // Delivered via PAYLOAD_CAM_INFO (see payload_get_video_streaming.cpp's
// // onPayloadStatusChanged): param[0] = capability flags.
// // ASSUMPTION: g_cb exposes this as cameraInfoSeq / cameraInfo[0] (flags).
// // The vendor example only reads flags; extend this struct if you need more
// // of CAMERA_INFORMATION (vendor/model strings, firmware version, etc.) once
// // you confirm how/if g_cb stores those fields.
// // ---------------------------------------------------------------------------
// struct CameraInfoStatus {
//     double capabilityFlags = -1;
// };

// inline bool readCameraInformation(CameraInfoStatus& info, int timeoutMs = 3000) {
//     const uint64_t seq = g_cb.cameraInfoSeq.load();
//     g_payload->getPayloadCameraInformation();
//     if (!waitForSeq(g_cb.cameraInfoSeq, seq, timeoutMs)) return false;

//     std::lock_guard<std::mutex> lock(g_cb.m);
//     info.capabilityFlags = g_cb.cameraInfoSeq[0];
//     return true;
// }

// // ---------------------------------------------------------------------------
// // Streaming information: getPayloadCameraStreamingInformation()
// // Delivered via PAYLOAD_CAM_STREAMINFO (see payload_get_video_streaming.cpp's
// // onPayloadStreamChanged): param_char = uri, param_double[0] = stream type,
// // [1] = resolution_v, [2] = resolution_h.
// // ASSUMPTION: g_cb exposes this as streamInfoSeq / streamInfoUri /
// // streamInfoValues[0..2], matching the char+double callback shape gb's
// // paramValueById/paramSeqById pattern uses for id-keyed data.
// // ---------------------------------------------------------------------------
// struct StreamingInfoStatus {
//     double type = -1;            // VIDEO_STREAM_TYPE_*
//     double resolutionV = -1;
//     double resolutionH = -1;
//     std::string uri;
// };

// inline bool readStreamingInformation(StreamingInfoStatus& info, int timeoutMs = 4000) {
//     const uint64_t seq = g_cb.streamSeq.load();
//     g_payload->getPayloadCameraStreamingInformation();
//     if (!waitForSeq(g_cb.streamSeq, seq, timeoutMs)) return false;

//     std::lock_guard<std::mutex> lock(g_cb.m);
//     info.type = g_cb.streamValues[0];
//     info.resolutionV = g_cb.streamValues[1];
//     info.resolutionH = g_cb.streamValues[2];
//     // info.uri = g_cb.uri; 
//     return true;
// }

// // ---------------------------------------------------------------------------
// // Stream bitrate: setPayloadStreamBitrate(device, bitrate)
// //
// // NOT independently verifiable yet: there is no documented getter for
// // bitrate, and getPayloadCameraStreamingInformation() (above) does not
// // report it (only type/resolution/uri). The vendor example
// // (payload_set_stream_bitrate.cpp) just calls setPayloadStreamBitrate() and
// // sleeps 500ms with no confirmation at all -- the same "fire and trust"
// // pattern that turned out to hide a real failure with the gimbal's OFF mode
// // earlier in this test suite.
// //
// // TODO before trusting this: find out (a) whether setPayloadStreamBitrate()
// // issues a COMMAND_LONG with an ACK, and if so which MAV_CMD id, so this can
// // use commandAcceptedOrInProgress()/sendAndAcceptIfAcked() the way
// // camera_eo_test_helper.h does for zoom; or (b) a parameter ID that reports
// // the active bitrate back, so a set+readback test (like setCameraParam()
// // elsewhere in this suite) is possible. Until then, this just calls the
// // setter and returns true/false based on nothing but "didn't throw" -- it
// // provides no real confidence and is deliberately not used in an ASSERT
// // below.
// inline bool requestStreamBitrate(int device, int bitrateBps) {
//     g_payload->setPayloadStreamBitrate(device, bitrateBps);
//     return true;
// }

// // ---------------------------------------------------------------------------
// // Test fixture: save/restore camera mode (the one piece of state here that's
// // cheap to save and restore reliably).
// // ---------------------------------------------------------------------------
// class FeatureTest : public PayloadTest {
// protected:
//     void SetUp() override {
//         haveMode_ = readCameraMode(originalMode_);
//         std::cout << "[  INFO  ] SetUp: camera mode=" << originalMode_
//                   << " (have=" << haveMode_ << ")\n";
//     }

//     void TearDown() override {
//         if (haveMode_) {
//             EXPECT_TRUE(setCameraMode(static_cast<CAMERA_MODE>(static_cast<int>(originalMode_))))
//                 << "cleanup: could not restore camera mode to " << originalMode_;
//         }
//     }

//     double originalMode_ = 0;
//     bool haveMode_ = false;
// };

// }  // namespace feature_test

// #endif  // API_FEATURE_TEST_HELPER_H_