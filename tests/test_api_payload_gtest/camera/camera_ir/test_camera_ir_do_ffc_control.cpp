/**
 * @file test_camera_ir_do_ffc_control.cpp
 * @brief Example-faithful IR FFC Auto -> Manual -> Trigger flow.
 */

#include "camera_ir_test_helpers.h"

namespace cit = camera_ir_test;

class CameraIrFfcControlTest : public cit::CameraIrTest {};

TEST_F(CameraIrFfcControlTest, AutoThenManualThenTrigger_RemainsResponsive) {
    const bool autoAck = cit::sendUser4Accepted([] {
        g_payload->setPayloadCameraFFCMode(FFC_MODE_AUTO);
    });
    std::this_thread::sleep_for(std::chrono::seconds(1));

    const bool manualAck = cit::sendUser4Accepted([] {
        g_payload->setPayloadCameraFFCMode(FFC_MODE_MANUAL);
    });
    std::this_thread::sleep_for(std::chrono::seconds(1));

    const bool triggerAck = cit::sendUser4Accepted([] {
        g_payload->setPayloadCameraFFCTrigg();
    });

    EXPECT_TRUE(autoAck || manualAck || triggerAck || cit::irStreamIsResponsive())
        << "FFC sequence produced no ACK and IR stream stopped responding.";
    EXPECT_TRUE(cit::irStreamIsResponsive());
}

TEST_F(CameraIrFfcControlTest, FiveManualTriggers_KeepIRResponsive) {
    g_payload->setPayloadCameraFFCMode(FFC_MODE_MANUAL);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    for (int trigger = 1; trigger <= 5; ++trigger) {
        std::cout << "[TEST] IR FFC trigger " << trigger << "/5" << std::endl;
        g_payload->setPayloadCameraFFCTrigg();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        ASSERT_TRUE(cit::irStreamIsResponsive())
            << "IR stream stopped after FFC trigger " << trigger << ".";
    }
}
