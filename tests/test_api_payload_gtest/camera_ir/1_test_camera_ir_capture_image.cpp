#include "api_camera_ir_test_helper.h"

namespace cit = camera_ir_test;

TEST_F(CameraIrTest, CaptureImageIncreasesImageCount) {
    ASSERT_TRUE(cit::setCameraMode(CAMERA_MODE_IMAGE));
    cit::CaptureStatus before;
    ASSERT_TRUE(cit::readCaptureStatus(before));
    ASSERT_EQ(before.image, 0);

    g_payload->setPayloadCameraParam(PAYLOAD_CAMERA_VIEW_SRC, PAYLOAD_CAMERA_VIEW_IR, PARAM_TYPE_UINT32);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    g_payload->setPayloadCameraParam(PAYLOAD_CAMERA_RECORD_SRC, PAYLOAD_CAMERA_RECORD_IR, PARAM_TYPE_UINT32);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    g_payload->setPayloadCameraCaptureImage();
    
    cit::CaptureStatus after = before;
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::seconds(20);
    while (std::chrono::steady_clock::now() < deadline &&
           after.count <= before.count) {
        ASSERT_TRUE(cit::readCaptureStatus(after, 1500));
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    // g_payload->setPayloadCameraStopImage();
    EXPECT_GT(after.count, before.count);
}
TEST_F(CameraIrTest, Manual_SetCameraViewToIR) {
    g_payload->setPayloadCameraParam(PAYLOAD_CAMERA_VIEW_SRC, PAYLOAD_CAMERA_VIEW_IR, PARAM_TYPE_UINT32);

}