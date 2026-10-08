#include "api_camera_ir_test_helper.h"

namespace cit = camera_ir_test;
#if defined(VIO) || defined(ORUSL)
class CameraIrFfcControlTest : public CameraIrTest {};

namespace {

constexpr int kFfcSettleSeconds = 5;

void expectFfcCommandAndIrResponse(std::function<void()> command,
                                   const char* description) {
    expectAckedCommand(command, MAV_CMD_USER_4);

    cit::FovStatus fov;
    ASSERT_TRUE(cit::readFovStatus(fov))
        << description << " was acknowledged, but the IR camera did not "
           "return a FOV response.";
    EXPECT_GT(fov.horizontal, 0.0)
        << description << " returned an invalid horizontal FOV.";
    EXPECT_GT(fov.vertical, 0.0)
        << description << " returned an invalid vertical FOV.";
}

}  // namespace

TEST_F(CameraIrFfcControlTest, AutoThenManualThenTrigger_RemainsResponsive) {
    expectFfcCommandAndIrResponse(
        [] { g_payload->setPayloadCameraFFCMode(FFC_MODE_AUTO); },
        "FFC AUTO mode");
    std::this_thread::sleep_for(std::chrono::seconds(kFfcSettleSeconds));

    expectFfcCommandAndIrResponse(
        [] { g_payload->setPayloadCameraFFCMode(FFC_MODE_MANUAL); },
        "FFC MANUAL mode");
    std::this_thread::sleep_for(std::chrono::seconds(kFfcSettleSeconds));

    expectFfcCommandAndIrResponse(
        [] { g_payload->setPayloadCameraFFCTrigg(); }, "FFC trigger");
    //Restore to AUTO mode
}

TEST_F(CameraIrFfcControlTest, FiveManualTriggers_KeepIRResponsive) {
    expectFfcCommandAndIrResponse(
        [] { g_payload->setPayloadCameraFFCMode(FFC_MODE_MANUAL); },
        "FFC MANUAL mode");
    std::this_thread::sleep_for(std::chrono::seconds(kFfcSettleSeconds));

    for (int trigger = 1; trigger <= 5; ++trigger) {
        SCOPED_TRACE("FFC trigger " + std::to_string(trigger));
        std::cout << "[TEST] IR FFC trigger " << trigger << "/5" << std::endl;
        expectFfcCommandAndIrResponse(
            [] { g_payload->setPayloadCameraFFCTrigg(); }, "FFC trigger");
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}
#endif
