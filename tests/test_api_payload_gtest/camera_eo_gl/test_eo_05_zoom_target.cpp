/**
 * Sheet row 5: Zoom den muc chi dinh - setCameraZoomTarget()
 * Support: VIO x | ORUSL x | MB1 ? | ZIO ?
 * Example: examples/payload_set_camera_zoom_targetpos.cpp
 */

#include "camera_eo_test_helpers.h"

using namespace eo;

// setCameraZoomTarget() is a MAV_CMD_USER_4 command.
class EO_ZoomTarget : public EoCameraTest {
protected:
    void SetUp() override {
        EO_SKIP_UNLESS_SUPPORTED("xx??");

        EoCameraTest::SetUp();

        if (HasFatalFailure()) {
            return;
        }

        // The example runs in Combine zoom mode, so do the same. TearDown() restores it.
#ifndef MB1
        if (restoreLater(PAYLOAD_CAMERA_VIDEO_ZOOM_MODE)) {
            setParam(PAYLOAD_CAMERA_VIDEO_ZOOM_MODE, PAYLOAD_CAMERA_VIDEO_ZOOM_MODE_COMBINE);
        }
#endif
    }

    // Send setCameraZoomTarget(level) and wait until the zoom level is near it.
    void zoomTo(double level, double tolerance) {
        // One UDP packet (command or ACK) can be lost, so the command is sent
        // again once before calling it a failure, like a real controller does.
        bool accepted = sendAndWaitAck(
            MAV_CMD_USER_4, //Camera duoc define boi gremsy
            [level] {
                g_payload->setCameraZoomTarget(float(level));
            },
            "zoom target"
        );

        if (!accepted) {
            std::cout << "[  INFO  ] setCameraZoomTarget(" << level << "): no ACK, sending once more\n";

            accepted = sendAndWaitAck(
                MAV_CMD_USER_4,
                [level] {
                    g_payload->setCameraZoomTarget(float(level));
                },
                "zoom target (retry)"
            );
        }

        EXPECT_TRUE(accepted)
            << "setCameraZoomTarget(" << level << ") was not accepted, even after one retry";

        if (accepted) {
            std::cout << "[  INFO  ] setCameraZoomTarget(" << level << "): ACK accepted\n";
        }

        double last = -1;

        bool zoomReached = waitZoomNear(CAMERA_EO, level, tolerance, 10000, &last);

        EXPECT_TRUE(zoomReached)
            << "zoom level did not reach " << level
            << " (last reported " << last << ")";

        std::cout << "[  INFO  ] target " << level << " -> level " << last << "\n";

        // Give the payload a moment before the next USER_4 command; two of them
        // back to back can be treated as a duplicate (SDK always sends confirmation = 1).
        sleepMs(500);
    }
};

// Check that setCameraZoomTarget() moves the zoom to the requested level.
TEST_F(EO_ZoomTarget, SetCameraZoomTarget) {
    double zoom = 0;

    ASSERT_TRUE(readZoomLevel(CAMERA_EO, zoom))
        << "EO zoom level is not readable; cannot verify the target";

    std::cout << "[  INFO  ] zoom level before: " << zoom << "\n";

    zoomTo(1.0, 0.3);

    zoomTo(4.0, 0.6);

    zoomTo(1.0, 0.3);
}
