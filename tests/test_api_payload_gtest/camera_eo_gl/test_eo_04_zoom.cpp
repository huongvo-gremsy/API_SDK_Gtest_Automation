/**
 * Sheet row 4: Dieu khien zoom - setCameraZoom()
 * Support: VIO x | ORUSL x | MB1 ? | ZIO x
 * Example: examples/camera_eo_set_zoom_focus.cpp
 */

#include "camera_eo_test_helpers.h"

using namespace eo;

// Zoom is checked through the EO zoom level (payload param EO_ZOOM).
class EO_Zoom : public EoCameraTest {
protected:
    void SetUp() override {
        EO_SKIP_UNLESS_SUPPORTED("xx?x");

        EoCameraTest::SetUp();

        if (HasFatalFailure()) {
            return;
        }

        // Digital zoom back to 1x. TearDown() restores the old value.
#ifdef MB1
        if (restoreLater(PAYLOAD_CAMERA_VIDEO_ZOOM_FACTOR)) {
            setParam(PAYLOAD_CAMERA_VIDEO_ZOOM_FACTOR, ZOOM_EO_1X);
        }
#else
        if (restoreLater(PAYLOAD_CAMERA_VIDEO_ZOOM_SUPER_RESOLUTION_FACTOR)) {
            setParam(PAYLOAD_CAMERA_VIDEO_ZOOM_SUPER_RESOLUTION_FACTOR, ZOOM_SUPER_RESOLUTION_1X);
        }

        if (restoreLater(PAYLOAD_CAMERA_VIDEO_ZOOM_COMBINE_FACTOR)) {
            setParam(PAYLOAD_CAMERA_VIDEO_ZOOM_COMBINE_FACTOR, ZOOM_COMBINE_1X);
        }
#endif

        // Optical zoom back to wide.
        g_payload->setCameraZoom(ZOOM_TYPE_RANGE, 0);

        sleepMs(1500);

        std::cout << "[  INFO  ] SetUp: digital zoom 1x, optical zoom range 0%\n";
    }
};

// Check that setCameraZoom(ZOOM_TYPE_STEP) zooms in and out.
TEST_F(EO_Zoom, SetCameraZoomStep) {
    double zoomBefore = 0;

    ASSERT_TRUE(readZoomLevel(CAMERA_EO, zoomBefore))
        << "EO zoom level is not readable";

    std::cout << "[  INFO  ] zoom level before: " << zoomBefore << "\n";

    // Zoom in 2 steps.
    for (int i = 0; i < 2; i++) {
        ASSERT_TRUE(
            sendAndWaitAck(
                MAV_CMD_SET_CAMERA_ZOOM,
                [] {
                    g_payload->setCameraZoom(ZOOM_TYPE_STEP, ZOOM_IN);
                },
                "step zoom in"
            )
        ) << "setCameraZoom(ZOOM_TYPE_STEP, ZOOM_IN) was not accepted";

        std::cout << "[  INFO  ] setCameraZoom(STEP, ZOOM_IN) #" << i + 1 << ": ACK accepted\n";

        sleepMs(800);
    }

    // Check that the zoom level increased.
    double zoomAfterIn = 0;

    bool zoomIncreased = waitZoomIncreased(CAMERA_EO, zoomBefore, 10000, &zoomAfterIn);

    EXPECT_TRUE(zoomIncreased)
        << "zoom level did not increase after 2 step in: "
        << zoomBefore << " -> " << zoomAfterIn;

    std::cout << "[  INFO  ] zoom level after 2 step in: " << zoomAfterIn << "\n";

    // Zoom out 2 steps.
    for (int i = 0; i < 2; i++) {
        ASSERT_TRUE(
            sendAndWaitAck(
                MAV_CMD_SET_CAMERA_ZOOM,
                [] {
                    g_payload->setCameraZoom(ZOOM_TYPE_STEP, ZOOM_OUT);
                },
                "step zoom out"
            )
        ) << "setCameraZoom(ZOOM_TYPE_STEP, ZOOM_OUT) was not accepted";

        std::cout << "[  INFO  ] setCameraZoom(STEP, ZOOM_OUT) #" << i + 1 << ": ACK accepted\n";

        sleepMs(800);
    }

    // Check that the zoom level decreased.
    double zoomAfterOut = 0;

    bool zoomDecreased = waitZoomDecreased(CAMERA_EO, zoomAfterIn, 10000, &zoomAfterOut);

    EXPECT_TRUE(zoomDecreased)
        << "zoom level did not decrease after 2 step out: "
        << zoomAfterIn << " -> " << zoomAfterOut;

    std::cout << "[  INFO  ] zoom level after 2 step out: " << zoomAfterOut << "\n";

    std::cout << "[  INFO  ] zoom levels: "
              << zoomBefore << " -> " << zoomAfterIn << " -> " << zoomAfterOut << "\n";
}

// Check that setCameraZoom(ZOOM_TYPE_CONTINUOUS) zooms in, stops, zooms out, stops.
TEST_F(EO_Zoom, SetCameraZoomContinuous) {
    double zoomBefore = 0;

    ASSERT_TRUE(readZoomLevel(CAMERA_EO, zoomBefore))
        << "EO zoom level is not readable";

    std::cout << "[  INFO  ] zoom level before: " << zoomBefore << "\n";

    // Zoom in for 2.5 seconds, then stop.
    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_SET_CAMERA_ZOOM,
            [] {
                g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_IN);
            },
            "continuous zoom in"
        )
    ) << "setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_IN) was not accepted";

    std::cout << "[  INFO  ] setCameraZoom(CONTINUOUS, ZOOM_IN): ACK accepted, zooming for 2.5 s\n";

    sleepMs(2500);

    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_SET_CAMERA_ZOOM,
            [] {
                g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_STOP);
            },
            "zoom stop"
        )
    ) << "setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_STOP) was not accepted";

    std::cout << "[  INFO  ] setCameraZoom(CONTINUOUS, ZOOM_STOP): ACK accepted\n";

    // Let the lens settle, then check that the zoom level increased.
    sleepMs(1500);

    double zoomAfterIn = 0;

    ASSERT_TRUE(readZoomLevel(CAMERA_EO, zoomAfterIn))
        << "EO zoom level is not readable";

    EXPECT_GT(zoomAfterIn, zoomBefore + 0.1)
        << "zoom level did not increase during zoom in: "
        << zoomBefore << " -> " << zoomAfterIn;

    std::cout << "[  INFO  ] zoom level after zoom in: " << zoomAfterIn << "\n";

    // Check that the zoom does not keep moving after stop.
    sleepMs(1500);

    double zoomAfterStop = 0;

    if (readZoomLevel(CAMERA_EO, zoomAfterStop)) {
        EXPECT_NEAR(zoomAfterStop, zoomAfterIn, 0.3)
            << "zoom kept moving after ZOOM_STOP";

        std::cout << "[  INFO  ] zoom level 1.5 s after stop: " << zoomAfterStop << "\n";
    }

    // Zoom out for 2.5 seconds, then stop.
    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_SET_CAMERA_ZOOM,
            [] {
                g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_OUT);
            },
            "continuous zoom out"
        )
    ) << "setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_OUT) was not accepted";

    std::cout << "[  INFO  ] setCameraZoom(CONTINUOUS, ZOOM_OUT): ACK accepted, zooming for 2.5 s\n";

    sleepMs(2500);

    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_SET_CAMERA_ZOOM,
            [] {
                g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_STOP);
            },
            "zoom stop"
        )
    ) << "setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_STOP) was not accepted";

    std::cout << "[  INFO  ] setCameraZoom(CONTINUOUS, ZOOM_STOP): ACK accepted\n";

    // Let the lens settle, then check that the zoom level decreased.
    sleepMs(1500);

    double zoomAfterOut = 0;

    ASSERT_TRUE(readZoomLevel(CAMERA_EO, zoomAfterOut))
        << "EO zoom level is not readable";

    EXPECT_LT(zoomAfterOut, zoomAfterIn - 0.1)
        << "zoom level did not decrease during zoom out: "
        << zoomAfterIn << " -> " << zoomAfterOut;

    std::cout << "[  INFO  ] zoom level after zoom out: " << zoomAfterOut << "\n";

    std::cout << "[  INFO  ] zoom levels: "
              << zoomBefore << " -> " << zoomAfterIn << " -> " << zoomAfterOut << "\n";
}

// Check that setCameraZoom(ZOOM_TYPE_RANGE) zooms to a percent of the range.
TEST_F(EO_Zoom, SetCameraZoomRange) {
    // Go to 0% first.
    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_SET_CAMERA_ZOOM,
            [] {
                g_payload->setCameraZoom(ZOOM_TYPE_RANGE, 0);
            },
            "zoom range 0%"
        )
    ) << "setCameraZoom(ZOOM_TYPE_RANGE, 0) was not accepted";

    std::cout << "[  INFO  ] setCameraZoom(RANGE, 0): ACK accepted\n";

    sleepMs(1500);

    double zoomAt0 = 0;

    ASSERT_TRUE(readZoomLevel(CAMERA_EO, zoomAt0))
        << "EO zoom level is not readable";

    std::cout << "[  INFO  ] zoom level at 0%: " << zoomAt0 << "\n";

    // Go to 50% and check that the zoom level increased.
    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_SET_CAMERA_ZOOM,
            [] {
                g_payload->setCameraZoom(ZOOM_TYPE_RANGE, 50);
            },
            "zoom range 50%"
        )
    ) << "setCameraZoom(ZOOM_TYPE_RANGE, 50) was not accepted";

    std::cout << "[  INFO  ] setCameraZoom(RANGE, 50): ACK accepted\n";

    double zoomAt50 = 0;

    bool zoomIncreased = waitZoomIncreased(CAMERA_EO, zoomAt0, 10000, &zoomAt50);

    EXPECT_TRUE(zoomIncreased)
        << "zoom level did not increase after range 50%: "
        << zoomAt0 << " -> " << zoomAt50;

    std::cout << "[  INFO  ] zoom level at 50%: " << zoomAt50 << "\n";

    // Back to 0%.
    ASSERT_TRUE(
        sendAndWaitAck(
            MAV_CMD_SET_CAMERA_ZOOM,
            [] {
                g_payload->setCameraZoom(ZOOM_TYPE_RANGE, 0);
            },
            "zoom range 0%"
        )
    ) << "setCameraZoom(ZOOM_TYPE_RANGE, 0) was not accepted";

    std::cout << "[  INFO  ] setCameraZoom(RANGE, 0): ACK accepted, back to wide\n";
}
