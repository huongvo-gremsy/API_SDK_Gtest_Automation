// #include "api_feature_test_helpers.h"
 
// using namespace feature_test;
 
// class Feature_Basic : public FeatureTest {};
 
// // ---------------------------------------------------------------------------
// // Camera mode: read, then set to the other mode and back.
// // ---------------------------------------------------------------------------
// TEST_F(Feature_Basic, CameraMode_SetAndReadBack) {
//     double mode = -1;
//     ASSERT_TRUE(readCameraMode(mode)) << "could not read current camera mode";
 
//     std::cout << "[  INFO  ] current mode: " << mode << "\n";
 
//     // Toggle to the other of IMAGE/VIDEO and confirm, then set back.
//     const CAMERA_MODE target = (static_cast<int>(mode) == CAMERA_MODE_IMAGE)
//                                     ? CAMERA_MODE_VIDEO
//                                     : CAMERA_MODE_IMAGE;
 
//     ASSERT_TRUE(setCameraMode(target)) << "could not set camera mode to " << target;
 
//     double afterSet = -1;
//     ASSERT_TRUE(readCameraMode(afterSet)) << "could not read camera mode after set";
//     EXPECT_EQ(static_cast<int>(afterSet), static_cast<int>(target))
//         << "camera mode did not change to " << target << " (reads back as " << afterSet << ")";
 
//     ASSERT_TRUE(setCameraMode(static_cast<CAMERA_MODE>(static_cast<int>(mode))))
//         << "could not restore camera mode to " << mode;
// }