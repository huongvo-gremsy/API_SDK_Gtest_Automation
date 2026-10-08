/**
 * @file test_camera_eo_settings.cpp
 * @brief Tests based on camera_change_settings.cpp and camera_load_settings.cpp.
 */

#include "../../common/payload_test_fixture.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <string>
#include <thread>

namespace {

void clearCameraParamHistory() {
    std::lock_guard<std::mutex> lock(g_cb.m);
    g_cb.cameraParamHistory.clear();
}

bool waitForListedValue(const char* paramId, uint32_t expectedValue,
                        int timeoutMs = 8000) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);

    while (std::chrono::steady_clock::now() < deadline) {
        {
            std::lock_guard<std::mutex> lock(g_cb.m);
            for (const auto& sample : g_cb.cameraParamHistory) {
                if (sample.id == paramId &&
                    std::isfinite(sample.value) &&
                    static_cast<uint32_t>(sample.value) == expectedValue) {
                    return true;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}

bool waitForListedParam(const char* paramId, uint32_t& value,
                        int timeoutMs = 8000) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);

    while (std::chrono::steady_clock::now() < deadline) {
        {
            std::lock_guard<std::mutex> lock(g_cb.m);
            for (const auto& sample : g_cb.cameraParamHistory) {
                if (sample.id == paramId && std::isfinite(sample.value)) {
                    value = static_cast<uint32_t>(sample.value);
                    return true;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}

}  // namespace

class CameraEoSettingsTest : public PayloadTest {
protected:
    void SetUp() override {
        // Save the original state using the same setting-list API exercised by
        // the example. Every parameterized case restores this value.
        clearCameraParamHistory();
        g_payload->getPayloadCameraSettingList();
        ASSERT_TRUE(waitForListedParam(
            PAYLOAD_CAMERA_VIDEO_OSD_MODE, originalOsdMode_))
            << "OSD_MODE was not present in the camera setting list.";
        haveOriginalOsdMode_ = true;
    }

    void TearDown() override {
        if (!haveOriginalOsdMode_) return;

        char osdModeId[] = PAYLOAD_CAMERA_VIDEO_OSD_MODE;
        g_payload->setPayloadCameraParam(
            osdModeId, originalOsdMode_, PARAM_TYPE_UINT32);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        clearCameraParamHistory();
        g_payload->getPayloadCameraSettingList();
        EXPECT_TRUE(waitForListedValue(
            PAYLOAD_CAMERA_VIDEO_OSD_MODE, originalOsdMode_))
            << "Could not restore OSD_MODE to " << originalOsdMode_ << ".";
    }

    bool setOsdModeAndVerify(uint32_t mode) {
        char osdModeId[] = PAYLOAD_CAMERA_VIDEO_OSD_MODE;

        // API under test.
        g_payload->setPayloadCameraParam(
            osdModeId, mode, PARAM_TYPE_UINT32);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Verify through the setting-list API used by the SDK example.
        clearCameraParamHistory();
        g_payload->getPayloadCameraSettingList();
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        return waitForListedValue(PAYLOAD_CAMERA_VIDEO_OSD_MODE, mode);
    }

    uint32_t originalOsdMode_ = 0;
    bool haveOriginalOsdMode_ = false;
};

TEST_F(CameraEoSettingsTest, SetOsdModeDisable_VerifiedBySettingList) {
    EXPECT_TRUE(setOsdModeAndVerify(
        PAYLOAD_CAMERA_VIDEO_OSD_MODE_DISABLE))
        << "Setting list did not report OSD_MODE=DISABLE (0).";
}

TEST_F(CameraEoSettingsTest, SetOsdModeDebug_VerifiedBySettingList) {
    EXPECT_TRUE(setOsdModeAndVerify(
        PAYLOAD_CAMERA_VIDEO_OSD_MODE_DEBUG))
        << "Setting list did not report OSD_MODE=DEBUG (1).";
}

TEST_F(CameraEoSettingsTest, SetOsdModeStatus_VerifiedBySettingList) {
    EXPECT_TRUE(setOsdModeAndVerify(
        PAYLOAD_CAMERA_VIDEO_OSD_MODE_STATUS))
        << "Setting list did not report OSD_MODE=STATUS (2).";
}
