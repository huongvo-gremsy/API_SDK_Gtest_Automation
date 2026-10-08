#include "api_camera_eo_test_helper.h"
class ManualCameraEoParamSettingTest : public testing::Test {};

// Manual test for camera record video start and stop
TEST_F(ManualCameraEoParamSettingTest,
       Manual_camera_record_video_status) {
    const uint64_t seq = g_cb.cameraCaptureStatusSeq.load();
    g_payload->getPayloadCaptureStatus();
    if (!waitForSeq(g_cb.cameraCaptureStatusSeq, seq, 3000)) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_cb.m);
    double image = g_cb.cameraCaptureStatus[0];
    double video = g_cb.cameraCaptureStatus[1];
    double count = g_cb.cameraCaptureStatus[2];
    double recordingMs = g_cb.cameraCaptureStatus[3];
    std::cout << "readCaptureStatus: image=" << image
              << ", video=" << video
              << ", count=" << count
              << ", recordingMs=" << recordingMs
              << std::endl;
    return;
}

TEST_F(ManualCameraEoParamSettingTest, Manual_camera_record_video_start) {
// #ifndef ZIO
//     ASSERT_TRUE(cet::setCameraParam(PAYLOAD_CAMERA_RECORD_SRC,
//                                     PAYLOAD_CAMERA_RECORD_BOTH));
// #endif
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    g_payload->setPayloadCameraMode(CAMERA_MODE_VIDEO);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    g_payload->setPayloadCameraRecordVideoStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    // g_payload->setPayloadCameraRecordVideoStop();
    // std::this_thread::sleep_for(std::chrono::milliseconds(200));
}
TEST_F(ManualCameraEoParamSettingTest, Manual_camera_record_video_stop) {
    g_payload->setPayloadCameraRecordVideoStop();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

// TEST_F(ManualCameraEoParamSettingTest, ManualGetSingleParamValuesById) {
// char paramId[] = "C_SOURCE";

//     const uint64_t seq = g_cb.paramSeq.load();
//     g_payload->getPayloadCameraSettingByID(paramId);

//     ASSERT_TRUE(waitForSeq(g_cb.paramSeq, seq, 10000))
//         << "No response received for " << paramId;

//     std::lock_guard<std::mutex> lock(g_cb.m);

//     ASSERT_EQ(g_cb.lastParamId, "C_SOURCE");

//     const double value = g_cb.paramValues[1];

//     std::cout << "--> Param_id: " << g_cb.lastParamId
//               << ", value: " << value << std::endl;
// }
TEST_F(ManualCameraEoParamSettingTest, ManualGetSingleParamValuesById)
{
    char paramId[] = "C_SOURCE";
    // Save sequence before sending request
    const uint64_t startSeq = g_cb.paramSeq.load();
    // Request parameter by ID
    g_payload->getPayloadCameraSettingByID(paramId);
    // Wait for a NEW response with the requested parameter ID
    const auto deadline =
        std::chrono::steady_clock::now() +
        std::chrono::milliseconds(10000);
    bool found = false;
    while (std::chrono::steady_clock::now() < deadline)
    {
        {
            std::lock_guard<std::mutex> lock(g_cb.m);
            if (g_cb.paramSeq.load() > startSeq &&
                g_cb.lastParamId == paramId)
            {
                found = true;
                break;
            }
        }
        std::this_thread::sleep_for(
            std::chrono::milliseconds(10));
    }
    ASSERT_TRUE(found)
        << "No response received for parameter: "
        << paramId;
    // Read the matched response
    std::lock_guard<std::mutex> lock(g_cb.m);
    ASSERT_EQ(g_cb.lastParamId, paramId);
    const double value = g_cb.paramValues[1];
    std::cout << "--> Param_id: "
              << g_cb.lastParamId
              << ", value: "
              << value
              << std::endl;
}
TEST_F(ManualCameraEoParamSettingTest, ManualGetSomeParamValuesById) {

    const std::vector<std::string> paramIds = {
        "C_V_REC",
        // "C_V_FV",
        // "C_V_FV_SPD"
        // Add more parameter IDs here
    };

    for (const auto& paramIdStr : paramIds) {

        char paramId[64];
        std::strncpy(paramId, paramIdStr.c_str(), sizeof(paramId));
        paramId[sizeof(paramId) - 1] = '\0';

        const uint64_t seq = g_cb.paramSeq.load();

        g_payload->getPayloadCameraSettingByID(paramId);

        ASSERT_TRUE(waitForSeq(g_cb.paramSeq, seq, 10000))
            << "No response received for " << paramIdStr;

        std::lock_guard<std::mutex> lock(g_cb.m);

        ASSERT_EQ(g_cb.lastParamId, paramIdStr)
            << "Returned parameter ID does not match requested ID";

        const double value = g_cb.paramValues[1];

        std::cout << "--> Param_id: "
                  << g_cb.lastParamId
                  << ", value: "
                  << value
                  << std::endl;
    }
}
// TEST_F(ManualCameraEoParamSettingTest, ManualSetVideoView) {
//     g_payload->sdkInitConnection();
//     g_payload->checkPayloadConnection();
//     g_payload->setPayloadCameraParam(PAYLOAD_CAMERA_VIEW_SRC, PAYLOAD_CAMERA_VIEW_EO, PARAM_TYPE_UINT32);
//     std::this_thread::sleep_for(std::chrono::milliseconds(150));
// }

TEST_F(ManualCameraEoParamSettingTest, ManualGetParamEoZoomLevel) {
    g_payload->requestParamValue(PARAM_EO_ZOOM_LEVEL);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    std::lock_guard<std::mutex> lock(g_cb.m);
    const double value = g_cb.payloadParamValueByIndex[PARAM_EO_ZOOM_LEVEL];
    std::cout << "--> Param_id: EO_ZOOM_LEVEL"
              << ", value: " << value << std::endl;
}

TEST_F(ManualCameraEoParamSettingTest, Manual_SetCameraViewToIR) {
    g_payload->setPayloadCameraParam(PAYLOAD_CAMERA_VIEW_SRC, PAYLOAD_CAMERA_VIEW_IR, PARAM_TYPE_UINT32);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}


/*
#define PAYLOAD_CAMERA_RECORD_SRC             "C_V_REC"
#define PAYLOAD_CAMERA_RECORD_BOTH              0
#define PAYLOAD_CAMERA_RECORD_EO                1
#define PAYLOAD_CAMERA_RECORD_IR                2
#define PAYLOAD_CAMERA_RECORD_OSD               5
*/
TEST_F(ManualCameraEoParamSettingTest, Manual_CameraRecordSource) {
    g_payload->setPayloadCameraParam(PAYLOAD_CAMERA_RECORD_SRC, PAYLOAD_CAMERA_RECORD_BOTH, PARAM_TYPE_UINT32);
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        g_payload->setPayloadCameraParam(PAYLOAD_CAMERA_RECORD_SRC, PAYLOAD_CAMERA_RECORD_EO, PARAM_TYPE_UINT32);
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        g_payload->setPayloadCameraParam(PAYLOAD_CAMERA_RECORD_SRC, PAYLOAD_CAMERA_RECORD_IR, PARAM_TYPE_UINT32);
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        g_payload->setPayloadCameraParam(PAYLOAD_CAMERA_RECORD_SRC, PAYLOAD_CAMERA_RECORD_OSD, PARAM_TYPE_UINT32);
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
}