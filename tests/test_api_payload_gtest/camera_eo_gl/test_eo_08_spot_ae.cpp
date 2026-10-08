/**
 * Sheet row 8: Spot AE hien thi / che do / vung do sang - setCameraExtSettings_SpotAE_Display() / _Mode() / _Position()
 * Support: VIO - | ORUSL x | MB1 - | ZIO -
 * Example: examples/camera_set_ext_settings.cpp
 */

#include "camera_eo_test_helpers.h"

using namespace eo;

// Values (see payloadSdkInterface.cpp): Display 1 = on, 0 = off. Mode 2 = on, 3 = off.
class EO_SpotAE : public EoCameraTest {
protected:
    void SetUp() override {
        EO_SKIP_UNLESS_SUPPORTED("-x--");

        EoCameraTest::SetUp();
    }
};

// Check that setCameraExtSettings_SpotAE_Display() is accepted for on and off.
TEST_F(EO_SpotAE, SetCameraExtSettings_SpotAE_Display) {
    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_USER_4,
            [] {
                g_payload->setCameraExtSettings_SpotAE_Display(1);
            },
            "SpotAE display on"
        )
    ) << "setCameraExtSettings_SpotAE_Display(1) was not accepted";

    std::cout << "[  INFO  ] setCameraExtSettings_SpotAE_Display(1): ACK accepted (display on)\n";

    sleepMs(1000);

    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_USER_4,
            [] {
                g_payload->setCameraExtSettings_SpotAE_Display(0);
            },
            "SpotAE display off"
        )
    ) << "setCameraExtSettings_SpotAE_Display(0) was not accepted";

    std::cout << "[  INFO  ] setCameraExtSettings_SpotAE_Display(0): ACK accepted (display off)\n";
}

// Check that setCameraExtSettings_SpotAE_Mode() is accepted for on and off.
TEST_F(EO_SpotAE, SetCameraExtSettings_SpotAE_Mode) {
    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_USER_4,
            [] {
                g_payload->setCameraExtSettings_SpotAE_Mode(2);
            },
            "SpotAE mode on"
        )
    ) << "setCameraExtSettings_SpotAE_Mode(2) was not accepted";

    std::cout << "[  INFO  ] setCameraExtSettings_SpotAE_Mode(2): ACK accepted (mode on)\n";

    sleepMs(1000);

    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_USER_4,
            [] {
                g_payload->setCameraExtSettings_SpotAE_Mode(3);
            },
            "SpotAE mode off"
        )
    ) << "setCameraExtSettings_SpotAE_Mode(3) was not accepted";

    std::cout << "[  INFO  ] setCameraExtSettings_SpotAE_Mode(3): ACK accepted (mode off)\n";
}

// Check that setCameraExtSettings_SpotAE_Position() is accepted while the mode is on.
TEST_F(EO_SpotAE, SetCameraExtSettings_SpotAE_Position) {
    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_USER_4,
            [] {
                g_payload->setCameraExtSettings_SpotAE_Mode(2);
            },
            "SpotAE mode on"
        )
    ) << "setCameraExtSettings_SpotAE_Mode(2) was not accepted";

    std::cout << "[  INFO  ] setCameraExtSettings_SpotAE_Mode(2): ACK accepted (mode on)\n";

    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_USER_4,
            [] {
                g_payload->setCameraExtSettings_SpotAE_Position(6, 6, 2, 2);
            },
            "SpotAE position"
        )
    ) << "setCameraExtSettings_SpotAE_Position(6, 6, 2, 2) was not accepted";

    std::cout << "[  INFO  ] setCameraExtSettings_SpotAE_Position(6, 6, 2, 2): ACK accepted\n";

    sleepMs(1000);

    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_USER_4,
            [] {
                g_payload->setCameraExtSettings_SpotAE_Mode(3);
            },
            "SpotAE mode off"
        )
    ) << "setCameraExtSettings_SpotAE_Mode(3) was not accepted";

    std::cout << "[  INFO  ] setCameraExtSettings_SpotAE_Mode(3): ACK accepted (mode off)\n";
}
