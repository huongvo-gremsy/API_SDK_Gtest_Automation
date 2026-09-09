# Camera control tests

This folder owns active camera controls that do not create media files.

| File | APIs | Verification |
|---|---|---|
| `test_camera_zoom.cpp` | `setCameraZoom()`, `setCameraZoomTarget()` | Continuous/step command behavior and physical magnification derived from EO FOV |
| `test_camera_focus.cpp` | `setCameraFocus()` | Manual-focus configuration readback and command-specific accepted ACK |
| `test_camera_ir_ffc.cpp` | `setPayloadCameraFFCMode()`, `getPayloadCameraFFCMode()`, `setPayloadCameraFFCTrigg()` | Input guard and command ACK; getter is registered as a known-gap skip because its SDK body is empty |
| `test_camera_white_balance.cpp` | `setPayloadCameraWBOnePushTrigg()` | One-push mode readback, command ACK, and repeated-trigger responsiveness |

The zoom fixture restores the original magnification using its saved FOV. The
focus and white-balance fixtures restore their original view and parameter
values. Every motion fixture sends STOP from `TearDown()`.

Run from `PayloadSdk/build`:

```bash
./tests/test_api_payload_gtest/test_api_payload_gtest \
  --gtest_filter='CameraZoomTest.*:CameraFocusTest.*:CameraIrFfc*:*CameraIRKnownGaps.*:CameraWhiteBalanceTest.*'
```

These are hardware tests. Zoom and focus can move the physical lens.
