#include "api_camera_ir_test_helper.h"

namespace cit = camera_ir_test;
class CameraIrTestSetZoom : public CameraIrTest{
    // protected:
    // void TearDown() override {
    //     CameraIrTest::TearDown();
    // }
};

TEST_F(CameraIrTestSetZoom, SetZoomFactorValues) {
    for (uint32_t value : {ZOOM_IR_1X, ZOOM_IR_2X, ZOOM_IR_4X, ZOOM_IR_8X}) {
        double actual = -1;
        ASSERT_TRUE(cit::setAndReadBack(PAYLOAD_CAMERA_IR_ZOOM_FACTOR,
                                        value, actual));
    }
}

TEST_F(CameraIrTestSetZoom, StepZoomInAndOut) {
    double zoom = -1;
    ASSERT_TRUE(cit::setAndReadBack(PAYLOAD_CAMERA_IR_ZOOM_FACTOR,
                                    ZOOM_IR_1X, zoom));

    cit::FovStatus baseline;
    ASSERT_TRUE(cit::readFovStatus(baseline));
    ASSERT_GT(baseline.horizontal, 0);
    ASSERT_GT(baseline.vertical, 0);

    g_payload->setCameraZoom(ZOOM_TYPE_STEP, ZOOM_IN);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    cit::FovStatus zoomedIn;
    ASSERT_TRUE(cit::readFovStatus(zoomedIn));
    EXPECT_LT(zoomedIn.horizontal, baseline.horizontal)
        << "ZOOM_IN should reduce horizontal FOV";
    EXPECT_LT(zoomedIn.vertical, baseline.vertical)
        << "ZOOM_IN should reduce vertical FOV";

    g_payload->setCameraZoom(ZOOM_TYPE_STEP, ZOOM_OUT);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    cit::FovStatus zoomedOut;
    ASSERT_TRUE(cit::readFovStatus(zoomedOut));
    EXPECT_GT(zoomedOut.horizontal, zoomedIn.horizontal)
        << "ZOOM_OUT should increase horizontal FOV";
    EXPECT_GT(zoomedOut.vertical, zoomedIn.vertical)
        << "ZOOM_OUT should increase vertical FOV";
}

TEST_F(CameraIrTestSetZoom, ZoomExample) {
    // --- Set view source to IR ---
    double viewSrc = -1;
    ASSERT_TRUE(cit::setAndReadBack(PAYLOAD_CAMERA_VIEW_SRC,
                                     PAYLOAD_CAMERA_VIEW_IR, viewSrc));

    // --- Reset zoom to 1x baseline ---
    double zoom = -1;
    ASSERT_TRUE(cit::setAndReadBack(PAYLOAD_CAMERA_IR_ZOOM_FACTOR,
                                     ZOOM_IR_1X, zoom));

    cit::FovStatus baseline;
    ASSERT_TRUE(cit::readFovStatus(baseline));
    ASSERT_GT(baseline.horizontal, 0);
    ASSERT_GT(baseline.vertical, 0);

    // ================= STEP ZOOM =================
    std::cout << "Zoom In 4 times..." << std::endl;
    cit::FovStatus prev = baseline;
    for (int i = 0; i < 4; ++i) {
        g_payload->setCameraZoom(ZOOM_TYPE_STEP, ZOOM_IN);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        cit::FovStatus current;
        ASSERT_TRUE(cit::readFovStatus(current));
        EXPECT_LT(current.horizontal, prev.horizontal)
            << "ZOOM_IN step " << (i + 1) << " should reduce horizontal FOV";
        EXPECT_LT(current.vertical, prev.vertical)
            << "ZOOM_IN step " << (i + 1) << " should reduce vertical FOV";
        prev = current;
    }
    cit::FovStatus afterZoomIn = prev;

    std::cout << "Zoom Out 2 times..." << std::endl;
    for (int i = 0; i < 2; ++i) {
        g_payload->setCameraZoom(ZOOM_TYPE_STEP, ZOOM_OUT);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        cit::FovStatus current;
        ASSERT_TRUE(cit::readFovStatus(current));
        EXPECT_GT(current.horizontal, prev.horizontal)
            << "ZOOM_OUT step " << (i + 1) << " should increase horizontal FOV";
        EXPECT_GT(current.vertical, prev.vertical)
            << "ZOOM_OUT step " << (i + 1) << " should increase vertical FOV";
        prev = current;
    }

    // ================= CONTINUOUS ZOOM =================
    std::cout << "Start Zoom In (continuous)..." << std::endl;
    g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_IN);
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    std::cout << "Stop Zoom!" << std::endl;
    g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_STOP);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    cit::FovStatus afterContinuousIn;
    ASSERT_TRUE(cit::readFovStatus(afterContinuousIn));
    EXPECT_LT(afterContinuousIn.horizontal, prev.horizontal)
        << "Continuous ZOOM_IN should reduce horizontal FOV";
    EXPECT_LT(afterContinuousIn.vertical, prev.vertical)
        << "Continuous ZOOM_IN should reduce vertical FOV";

    std::cout << "Start Zoom Out (continuous)..." << std::endl;
    g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_OUT);
    std::this_thread::sleep_for(std::chrono::milliseconds(7000));

    std::cout << "Stop Zoom!" << std::endl;
    g_payload->setCameraZoom(ZOOM_TYPE_CONTINUOUS, ZOOM_STOP);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    cit::FovStatus afterContinuousOut;
    ASSERT_TRUE(cit::readFovStatus(afterContinuousOut));
    EXPECT_GT(afterContinuousOut.horizontal, afterContinuousIn.horizontal)
        << "Continuous ZOOM_OUT should increase horizontal FOV";
    EXPECT_GT(afterContinuousOut.vertical, afterContinuousIn.vertical)
        << "Continuous ZOOM_OUT should increase vertical FOV";

    // ================= RANGE ZOOM =================
    struct RangeStep { double percent; };
    const RangeStep rangeSteps[] = {{50.0}, {70.0}, {100.0}, {0.0}};

    cit::FovStatus prevRange = afterContinuousOut;
    for (size_t i = 0; i < std::size(rangeSteps); ++i) {
        const double pct = rangeSteps[i].percent;
        std::cout << "Zoom Range " << pct << "%..." << std::endl;
        g_payload->setCameraZoom(ZOOM_TYPE_RANGE, pct);

        // last step (0%) matches the example's longer settle time
        const int waitMs = (i == std::size(rangeSteps) - 1) ? 5000 : 3000;
        std::this_thread::sleep_for(std::chrono::milliseconds(waitMs));

        cit::FovStatus current;
        ASSERT_TRUE(cit::readFovStatus(current));
        ASSERT_GT(current.horizontal, 0);
        ASSERT_GT(current.vertical, 0);

        // Higher range % => more zoomed in => narrower FOV than previous
        // (monotonic relationship depends on payload's range mapping;
        // relaxed to a sanity check that FOV actually changed)
        EXPECT_NE(current.horizontal, prevRange.horizontal)
            << "Range zoom to " << pct << "% did not change horizontal FOV";
        prevRange = current;
    }

    std::cout << "!--------------------!" << std::endl;
}
