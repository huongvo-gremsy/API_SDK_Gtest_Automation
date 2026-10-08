/**
 * Sheet row 6: Dieu khien focus - setCameraFocus()
 * Support: VIO x | ORUSL x | MB1 ? | ZIO x
 * Example: examples/camera_eo_set_zoom_focus.cpp
 */

#include "camera_eo_test_helpers.h"

using namespace eo;

// Several firmwares always report focus level 0, so pass/fail is based on the ACK.
class EO_Focus : public EoCameraTest {
protected:
    void SetUp() override {
        EO_SKIP_UNLESS_SUPPORTED("xx?x");

        EoCameraTest::SetUp();

        if (HasFatalFailure()) {
            return;
        }

        // Manual focus so the lens obeys the focus commands. TearDown() restores it.
#ifndef MB1
        if (restoreLater(PAYLOAD_CAMERA_VIDEO_FOCUS_MODE)) {
            setParam(PAYLOAD_CAMERA_VIDEO_FOCUS_MODE, PAYLOAD_CAMERA_VIDEO_FOCUS_MODE_MANUAL);

            std::cout << "[  INFO  ] SetUp: focus mode = MANUAL\n";
        }
#endif
    }

    // Focus level from CAMERA_SETTINGS (only printed, not asserted).
    double readFocusLevel() {
        double mode = 0;
        double zoom = 0;
        double focus = -1;

        readCameraSettings(mode, zoom, focus, 1500);

        return focus;
    }

    // Prints both focus readings for reference, nothing is asserted:
    //   CAMERA_SETTINGS.focusLevel  - constant on several firmwares
    //   C_V_FV                      - real lens position on VIO / ORUSL, absent on MB1
    void printFocusInfo(const char* label) {
        double focusLevel = readFocusLevel();

        double focusValue = -1;
        bool hasFocusValue = readParam("C_V_FV", focusValue, 1500);   // raw id: MB1 header has no macro for it

        std::cout << "[  INFO  ] " << label << ": CAMERA_SETTINGS.focusLevel=" << focusLevel;

        if (hasFocusValue) {
            std::cout << " C_V_FV=" << focusValue;
        } else {
            std::cout << " C_V_FV=(not readable on this payload)";
        }

        std::cout << "\n";
    }
};

// Check that setCameraFocus(FOCUS_TYPE_CONTINUOUS) is accepted for in, stop, out, stop.
TEST_F(EO_Focus, SetCameraFocusContinuous) {
    double focusBefore = readFocusLevel();

    printFocusInfo("before");

    // Focus in for 1.5 seconds, then stop.
    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_SET_CAMERA_FOCUS,
            [] {
                g_payload->setCameraFocus(FOCUS_TYPE_CONTINUOUS, FOCUS_IN);
            },
            "focus in"
        )
    ) << "setCameraFocus(FOCUS_TYPE_CONTINUOUS, FOCUS_IN) was not accepted";

    std::cout << "[  INFO  ] setCameraFocus(CONTINUOUS, FOCUS_IN): ACK accepted, focusing for 1.5 s\n";

    sleepMs(1500);

    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_SET_CAMERA_FOCUS,
            [] {
                g_payload->setCameraFocus(FOCUS_TYPE_CONTINUOUS, FOCUS_STOP);
            },
            "focus stop"
        )
    ) << "setCameraFocus(FOCUS_TYPE_CONTINUOUS, FOCUS_STOP) was not accepted";

    std::cout << "[  INFO  ] setCameraFocus(CONTINUOUS, FOCUS_STOP): ACK accepted\n";

    double focusAfterIn = readFocusLevel();

    printFocusInfo("after focus in");

    // Focus out for 1.5 seconds, then stop.
    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_SET_CAMERA_FOCUS,
            [] {
                g_payload->setCameraFocus(FOCUS_TYPE_CONTINUOUS, FOCUS_OUT);
            },
            "focus out"
        )
    ) << "setCameraFocus(FOCUS_TYPE_CONTINUOUS, FOCUS_OUT) was not accepted";

    std::cout << "[  INFO  ] setCameraFocus(CONTINUOUS, FOCUS_OUT): ACK accepted, focusing for 1.5 s\n";

    sleepMs(1500);

    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_SET_CAMERA_FOCUS,
            [] {
                g_payload->setCameraFocus(FOCUS_TYPE_CONTINUOUS, FOCUS_STOP);
            },
            "focus stop"
        )
    ) << "setCameraFocus(FOCUS_TYPE_CONTINUOUS, FOCUS_STOP) was not accepted";

    std::cout << "[  INFO  ] setCameraFocus(CONTINUOUS, FOCUS_STOP): ACK accepted\n";

    double focusAfterOut = readFocusLevel();

    printFocusInfo("after focus out");

    std::cout << "[  INFO  ] focus levels: "
              << focusBefore << " -> " << focusAfterIn << " -> " << focusAfterOut;

    if (focusBefore == 0 && focusAfterIn == 0 && focusAfterOut == 0) {
        std::cout << " (payload does not report focus level)";
    }

    std::cout << "\n";
}

// Check that setCameraFocus(FOCUS_TYPE_STEP) is accepted for in and out.
TEST_F(EO_Focus, SetCameraFocusStep) {
    // Focus in 2 steps.
    for (int i = 0; i < 2; i++) {
        ASSERT_TRUE(
            sendAndWaitAck(
                MAV_CMD_SET_CAMERA_FOCUS,
                [] {
                    g_payload->setCameraFocus(FOCUS_TYPE_STEP, FOCUS_IN);
                },
                "step focus in"
            )
        ) << "setCameraFocus(FOCUS_TYPE_STEP, FOCUS_IN) was not accepted";

        std::cout << "[  INFO  ] setCameraFocus(STEP, FOCUS_IN) #" << i + 1 << ": ACK accepted\n";

        sleepMs(500);
    }

    // Focus out 2 steps.
    for (int i = 0; i < 2; i++) {
        ASSERT_TRUE(
            sendAndWaitAck(
                MAV_CMD_SET_CAMERA_FOCUS,
                [] {
                    g_payload->setCameraFocus(FOCUS_TYPE_STEP, FOCUS_OUT);
                },
                "step focus out"
            )
        ) << "setCameraFocus(FOCUS_TYPE_STEP, FOCUS_OUT) was not accepted";

        std::cout << "[  INFO  ] setCameraFocus(STEP, FOCUS_OUT) #" << i + 1 << ": ACK accepted\n";

        sleepMs(500);
    }
}

// Check that setCameraFocus(FOCUS_TYPE_AUTO) is accepted.
TEST_F(EO_Focus, SetCameraFocusAuto) {
    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_SET_CAMERA_FOCUS,
            [] {
                g_payload->setCameraFocus(FOCUS_TYPE_AUTO, 0);
            },
            "auto focus"
        )
    ) << "setCameraFocus(FOCUS_TYPE_AUTO, 0) was not accepted";

    std::cout << "[  INFO  ] setCameraFocus(AUTO, 0): ACK accepted\n";
}
