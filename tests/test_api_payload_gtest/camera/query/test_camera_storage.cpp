/**
 * @file test_camera_storage.cpp
 * @brief Read-only getPayloadStorage() tests.
 */

#include "camera_query_test_helpers.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace {

struct StorageInfo {
    double total = 0;
    double used = 0;
    double available = 0;
    double status = -1;
};

bool readStorage(StorageInfo& info) {
    return getStorageInfo(info.total, info.used, info.available,
                          info.status, 3000);
}

}  // namespace

class CameraStorageTest : public PayloadTest {};

TEST_F(CameraStorageTest, InformationEventArrives) {
    StorageInfo info;
    ASSERT_TRUE(readStorage(info))
        << "No new PAYLOAD_CAM_STORAGE_INFO callback within 3000 ms.";
    std::cout << "[INFO] Storage total=" << info.total
              << " used=" << info.used
              << " available=" << info.available
              << " status=" << info.status << std::endl;
}

TEST_F(CameraStorageTest, Capacities_AreFiniteAndNonnegative) {
    StorageInfo info;
    ASSERT_TRUE(readStorage(info));
    EXPECT_TRUE(std::isfinite(info.total));
    EXPECT_TRUE(std::isfinite(info.used));
    EXPECT_TRUE(std::isfinite(info.available));
    EXPECT_GE(info.total, 0.0);
    EXPECT_GE(info.used, 0.0);
    EXPECT_GE(info.available, 0.0);
}

TEST_F(CameraStorageTest, TotalUsedAvailable_AreConsistent) {
    StorageInfo info;
    ASSERT_TRUE(readStorage(info));
    if (static_cast<int>(info.status) == STORAGE_STATUS_NOT_SUPPORTED) {
        GTEST_SKIP() << "Payload does not provide capacity information.";
    }
    ASSERT_GT(info.total, 0.0);
    const double tolerance = std::max(10.0, info.total * 0.02);
    EXPECT_NEAR(info.used + info.available, info.total, tolerance)
        << "Used plus available storage differs materially from total.";
}

TEST_F(CameraStorageTest, Status_IsKnown) {
    StorageInfo info;
    ASSERT_TRUE(readStorage(info));
    EXPECT_GE(info.status, static_cast<double>(STORAGE_STATUS_EMPTY));
    EXPECT_LT(info.status, static_cast<double>(STORAGE_STATUS_ENUM_END));
    EXPECT_EQ(info.status, std::floor(info.status));
}

TEST_F(CameraStorageTest, AvailableCapacity_ExceedsMediaThreshold) {
    StorageInfo info;
    ASSERT_TRUE(readStorage(info));
    if (static_cast<int>(info.status) != STORAGE_STATUS_READY) {
        GTEST_SKIP() << "Storage is not READY; status=" << info.status << ".";
    }
    EXPECT_GE(info.available, 10.0)
        << "Less than the 10 MB threshold used by SDK media examples.";
}
