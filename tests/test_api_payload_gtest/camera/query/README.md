# Camera query tests

This folder owns the read/query portion of `PayloadSdkInterface` camera API.

| File | API under test | Main verification |
|---|---|---|
| `test_camera_information.cpp` | `getPayloadCameraInformation()` | Fresh camera-info event and capability flags |
| `test_component_information.cpp` | `getPayloadComponentBasicInformation()` | Model, software version, and serial callback fields |
| `test_camera_storage.cpp` | `getPayloadStorage()` | Storage status and capacity consistency |
| `test_capture_status.cpp` | `getPayloadCaptureStatus()` | Valid image/video state, count, and recording time |
| `test_camera_mode.cpp` | `getPayloadCameraMode()`, paired `setPayloadCameraMode()` | Mode event, reversible state or command-specific ACK |
| `test_camera_fov.cpp` | `getPayloadCameraFOVStatus()` | Valid EO/IR horizontal and vertical FOV |

Run the complete group from `PayloadSdk/build`:

```bash
./tests/test_api_payload_gtest/test_api_payload_gtest \
  --gtest_filter='CameraInformationTest.*:CoreCapabilities/CameraInformationCapabilityTest.*:ComponentInformationTest.*:CameraStorageTest.*:CaptureStatusTest.*:CameraModeTest.*:CameraFovTest.*'
```

To run only read-only cases, exclude the four camera-mode setter cases:

```bash
./tests/test_api_payload_gtest/test_api_payload_gtest \
  --gtest_filter='CameraInformationTest.*:CoreCapabilities/CameraInformationCapabilityTest.*:ComponentInformationTest.*:CameraStorageTest.*:CaptureStatusTest.*:CameraFovTest.*:CameraModeTest.GetMode_EventArrives'
```
