/**
 * @file test_camera_ir_set_palette.cpp
 * @brief Tests all palette values used by camera_ir_set_palette.cpp.
 */

#include "camera_ir_test_helpers.h"

namespace cit = camera_ir_test;

class CameraIrPaletteTest : public cit::CameraIrTest {
protected:
    void SetUp() override {
        CameraIrTest::SetUp();
        if (::testing::Test::IsSkipped()) return;
        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_IR_PALETTE,
                                          originalPalette_, 3000));
        havePalette_ = true;
    }
    void TearDown() override {
        if (havePalette_) {
            EXPECT_TRUE(cit::setUint32Param(
                PAYLOAD_CAMERA_IR_PALETTE,
                static_cast<uint32_t>(originalPalette_)))
                << "Could not restore the original IR palette.";
        }
        CameraIrTest::TearDown();
    }
    double originalPalette_ = 0;
    bool havePalette_ = false;
};

TEST_F(CameraIrPaletteTest, Palettes1Through10_SetAndReadBack) {
    const uint32_t palettes[] = {
        PAYLOAD_CAMERA_IR_PALETTE_1, PAYLOAD_CAMERA_IR_PALETTE_2,
        PAYLOAD_CAMERA_IR_PALETTE_3, PAYLOAD_CAMERA_IR_PALETTE_4,
        PAYLOAD_CAMERA_IR_PALETTE_5, PAYLOAD_CAMERA_IR_PALETTE_6,
        PAYLOAD_CAMERA_IR_PALETTE_7, PAYLOAD_CAMERA_IR_PALETTE_8,
        PAYLOAD_CAMERA_IR_PALETTE_9, PAYLOAD_CAMERA_IR_PALETTE_10,
    };
    for (size_t index = 0; index < sizeof(palettes) / sizeof(palettes[0]); ++index) {
        std::cout << "[TEST] Selecting IR palette " << (index + 1) << std::endl;
        ASSERT_TRUE(cit::setUint32Param(PAYLOAD_CAMERA_IR_PALETTE,
                                        palettes[index]));
        double actual = 0;
        ASSERT_TRUE(getCameraSettingByID(PAYLOAD_CAMERA_IR_PALETTE, actual));
        EXPECT_EQ(static_cast<uint32_t>(actual), palettes[index]);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}
