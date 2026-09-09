#ifndef CONFIG_PARAM_TEST_HELPERS_H_
#define CONFIG_PARAM_TEST_HELPERS_H_

#include "../../common/payload_test_fixture.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <thread>
#include <vector>

// Camera parameter read/set verification belongs to the parameters module.
// The common fixture only captures PARAM_EXT callbacks and sequence metadata.
inline bool getCameraSettingByID(const char* paramId, double& outValue,
                                 int timeoutMs = -1) {
    if (timeoutMs < 0) timeoutMs = g_timeoutMs;
    uint64_t seq = 0;
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        const auto it = g_cb.paramSeqById.find(paramId);
        seq = it == g_cb.paramSeqById.end() ? 0 : it->second;
    }

    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->getPayloadCameraSettingByID(const_cast<char*>(paramId));
        const auto retryDeadline = std::min(
            deadline,
            std::chrono::steady_clock::now() + std::chrono::milliseconds(500));

        while (std::chrono::steady_clock::now() < retryDeadline) {
            {
                std::lock_guard<std::mutex> lock(g_cb.m);
                const auto seqIt = g_cb.paramSeqById.find(paramId);
                if (seqIt != g_cb.paramSeqById.end() && seqIt->second > seq) {
                    outValue = g_cb.paramValueById.at(paramId);
                    return true;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    return false;
}

inline bool setAndVerifyCameraParam(char* paramId, uint32_t value,
                                    uint8_t paramType, double expectedValue,
                                    int timeoutMs = 5000,
                                    int retryIntervalMs = 500) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->setPayloadCameraParam(paramId, value, paramType);
        double current = 0;
        if (getCameraSettingByID(paramId, current, retryIntervalMs) &&
            current == expectedValue) {
            return true;
        }
    }
    return false;
}

namespace config_param {

struct CameraParamConfig {
    const char* group;
    const char* name;
    const char* id;
};

inline const std::vector<CameraParamConfig>& cameraParamConfigs() {
    static const std::vector<CameraParamConfig> configs = {
        {"tracking", "tracking_mode", PAYLOAD_CAMERA_TRACKING_MODE},
        {"control", "rc_mode", PAYLOAD_CAMERA_RC_MODE},
        {"source", "view_source", PAYLOAD_CAMERA_VIEW_SRC},
        {"source", "record_source", PAYLOAD_CAMERA_RECORD_SRC},
        {"video", "osd_mode", PAYLOAD_CAMERA_VIDEO_OSD_MODE},
        {"video", "image_flip", PAYLOAD_CAMERA_VIDEO_FLIP},

        {"ir", "palette", PAYLOAD_CAMERA_IR_PALETTE},
        {"ir", "zoom_factor", PAYLOAD_CAMERA_IR_ZOOM_FACTOR},

#if defined(VIO) || defined(ORUSL)
        {"zoom", "eo_zoom_mode", PAYLOAD_CAMERA_VIDEO_ZOOM_MODE},
        {"zoom", "eo_zoom_combine_factor", PAYLOAD_CAMERA_VIDEO_ZOOM_COMBINE_FACTOR},
        {"zoom", "eo_zoom_super_resolution_factor",
         PAYLOAD_CAMERA_VIDEO_ZOOM_SUPER_RESOLUTION_FACTOR},
        {"zoom", "eo_zoom_speed", PAYLOAD_CAMERA_EO_ZOOM_SPEED},

        {"image", "eo_freeze", PAYLOAD_CAMERA_EO_FREEZE},
        {"image", "defog", PAYLOAD_CAMERA_VIDEO_DEFOG},
        {"image", "defog_level", PAYLOAD_CAMERA_VIDEO_DEFOG_LEVEL},
        {"image", "eo_high_sensitivity", PAYLOAD_CAMERA_EO_HS},

        {"exposure", "auto_exposure", PAYLOAD_CAMERA_VIDEO_AUTO_EXPOSURE},
        {"exposure", "shutter_speed", PAYLOAD_CAMERA_VIDEO_SHUTTER_SPEED},
        {"exposure", "shutter_min_limit", PAYLOAD_CAMERA_EO_SHUTTER_MIN_LIMIT},
        {"exposure", "aperture_value", PAYLOAD_CAMERA_VIDEO_APERTURE_VALUE},
        {"exposure", "eo_gain_hs", PAYLOAD_CAMERA_EO_GAIN_HS},
        {"exposure", "eo_gain_ls", PAYLOAD_CAMERA_EO_GAIN_LS},
        {"exposure", "bright_value", PAYLOAD_CAMERA_VIDEO_BRIGHT_VALUE},

        {"white_balance", "mode", PAYLOAD_CAMERA_VIDEO_WHITE_BALANCE},
        {"white_balance", "r_gain", PAYLOAD_CAMERA_EO_R_GAIN},
        {"white_balance", "b_gain", PAYLOAD_CAMERA_EO_B_GAIN},

        {"focus", "focus_mode", PAYLOAD_CAMERA_VIDEO_FOCUS_MODE},
        {"focus", "focus_value", PAYLOAD_CAMERA_VIDEO_FOCUS_VALUE},
        {"focus", "focus_speed", PAYLOAD_CAMERA_EO_FOCUS_SPEED},

        {"icr", "icr_mode", PAYLOAD_CAMERA_EO_ICR_MODE},
        {"icr", "icr_auto_threshold", PAYLOAD_CAMERA_EO_ICR_MODE_AUTO_THRESHOLD},
        {"icr", "icr_manual", PAYLOAD_CAMERA_EO_ICR_MANUAL},
#endif

        {"gimbal", "gimbal_mode", PAYLOAD_CAMERA_GIMBAL_MODE},
        {"lrf", "lrf_mode", PAYLOAD_LRF_MODE},
    };
    return configs;
}

inline void printCameraParamConfigHeader(const char* title) {
    std::cout << "\n========== " << title << " ==========\n";
    std::cout << std::left
              << std::setw(16) << "GROUP"
              << std::setw(34) << "NAME"
              << std::setw(18) << "PARAM_ID"
              << "VALUE\n";
    std::cout << std::string(78, '-') << "\n";
}

inline void printCameraParamConfigRow(const CameraParamConfig& config,
                                      const char* valueText) {
    std::cout << std::left
              << std::setw(16) << config.group
              << std::setw(34) << config.name
              << std::setw(18) << config.id
              << valueText << "\n";
}

inline void printCameraParamConfigRow(const CameraParamConfig& config,
                                      double value) {
    std::cout << std::left
              << std::setw(16) << config.group
              << std::setw(34) << config.name
              << std::setw(18) << config.id
              << value << "\n";
}

inline void copyParamId(char* dest, std::size_t destSize, const char* src) {
    std::memset(dest, 0, destSize);
    std::strncpy(dest, src, destSize - 1);
}

inline bool readUint32Param(const char* id, uint32_t& value, int timeoutMs = 3000) {
    double current = 0;
    if (!getCameraSettingByID(id, current, timeoutMs)) {
        return false;
    }
    value = static_cast<uint32_t>(current);
    return true;
}

inline std::vector<CameraParamSample> collectCameraParamList(
        int timeoutMs = 8000, int idleAfterResponseMs = 1200) {
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        g_cb.cameraParamHistory.clear();
    }

    g_payload->getPayloadCameraSettingList();
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    auto lastChange = std::chrono::steady_clock::now();
    std::size_t lastSize = 0;

    while (std::chrono::steady_clock::now() < deadline) {
        std::size_t size = 0;
        {
            std::lock_guard<std::mutex> lock(g_cb.m);
            size = g_cb.cameraParamHistory.size();
        }
        if (size != lastSize) {
            lastSize = size;
            lastChange = std::chrono::steady_clock::now();
        } else if (size > 0 &&
                   std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now() - lastChange).count() >=
                       idleAfterResponseMs) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    std::map<std::string, CameraParamSample> uniqueById;
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        for (const auto& sample : g_cb.cameraParamHistory) {
            if (!sample.id.empty()) uniqueById[sample.id] = sample;
        }
    }

    std::vector<CameraParamSample> result;
    for (const auto& entry : uniqueById) result.push_back(entry.second);
    std::sort(result.begin(), result.end(),
              [](const CameraParamSample& a, const CameraParamSample& b) {
                  return a.index < b.index;
              });
    return result;
}

inline bool readCameraParamByIndex(uint8_t index, CameraParamSample& out,
                                   int timeoutMs = 3000) {
    std::size_t start = 0;
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        start = g_cb.cameraParamHistory.size();
    }
    g_payload->getPayloadCameraSettingByIndex(index);

    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        {
            std::lock_guard<std::mutex> lock(g_cb.m);
            for (std::size_t i = start; i < g_cb.cameraParamHistory.size(); ++i) {
                if (g_cb.cameraParamHistory[i].index == index) {
                    out = g_cb.cameraParamHistory[i];
                    return true;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}

inline bool readCameraParamById(const char* id, CameraParamSample& out,
                                int timeoutMs = 3000) {
    std::size_t start = 0;
    {
        std::lock_guard<std::mutex> lock(g_cb.m);
        start = g_cb.cameraParamHistory.size();
    }

    // Keep one local terminator beyond MAVLink's full 16-byte ID field so a
    // valid 16-character parameter ID is not truncated before the SDK copies it.
    char mutableId[CAM_PARAM_ID_LEN + 1] = {0};
    copyParamId(mutableId, sizeof(mutableId), id);

    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        g_payload->getPayloadCameraSettingByID(mutableId);
        const auto retryDeadline = std::min(
            deadline,
            std::chrono::steady_clock::now() + std::chrono::milliseconds(500));
        while (std::chrono::steady_clock::now() < retryDeadline) {
            {
                std::lock_guard<std::mutex> lock(g_cb.m);
                for (std::size_t i = start;
                     i < g_cb.cameraParamHistory.size(); ++i) {
                    if (g_cb.cameraParamHistory[i].id == id) {
                        out = g_cb.cameraParamHistory[i];
                        return true;
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    return false;
}

inline bool setAndVerifyUint32Param(const char* id, uint32_t value,
                                    int timeoutMs = 6000,
                                    int retryIntervalMs = 500) {
    char mutableId[CAM_PARAM_ID_LEN];
    copyParamId(mutableId, sizeof(mutableId), id);

    std::cout << "[CONFIG] " << id << " -> " << value << std::endl;
    return setAndVerifyCameraParam(mutableId, value, PARAM_TYPE_UINT32,
                                   value, timeoutMs, retryIntervalMs);
}

class ScopedUint32ParamRestore {
public:
    explicit ScopedUint32ParamRestore(const char* id)
        : id_(id), hasOriginal_(readUint32Param(id, original_, 3000)) {}

    ~ScopedUint32ParamRestore() {
        if (hasOriginal_) {
            setAndVerifyUint32Param(id_, original_, 4000, 500);
        }
    }

    bool hasOriginal() const { return hasOriginal_; }
    uint32_t original() const { return original_; }

private:
    const char* id_;
    bool hasOriginal_;
    uint32_t original_ = 0;
};

inline void expectConfigAccepted(const char* id, uint32_t value) {
    ScopedUint32ParamRestore restore(id);
    ASSERT_TRUE(restore.hasOriginal())
        << "Could not read original value for " << id;

    ASSERT_TRUE(setAndVerifyUint32Param(id, value))
        << "Could not verify " << id << " changed to " << value;
}

} // namespace config_param

class SetPayloadCameraParamConfigTest : public PayloadTest {};

#endif
