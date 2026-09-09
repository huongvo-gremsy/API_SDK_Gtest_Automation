# Camera media tests

This folder owns APIs that create image or video files on payload storage.

| File | APIs | Verification |
|---|---|---|
| `test_camera_capture.cpp` | `setPayloadCameraCaptureImage()`, `setPayloadCameraStopImage()` | Storage readiness and increasing `image_count`; stop uses command-specific best-effort ACK when no independent state exists |
| `test_camera_record.cpp` | `setPayloadCameraRecordVideoStart()`, `setPayloadCameraRecordVideoStop()` | `video_status` becomes active/idle and `recording_time_ms` advances |

Both fixtures:

- require at least 10 MB available storage;
- stop recording left active by an interrupted test;
- cover EO, IR, PiP, and Both where supported;
- restore original mode, view source, record source, and MB1 storage target;
- stop media operations from `TearDown()` after assertion failures.

Run from `PayloadSdk/build`:

```bash
./tests/test_api_payload_gtest/test_api_payload_gtest \
  --gtest_filter='CameraCaptureTest.*:CaptureModes/CameraCaptureSourceTest.*:CameraRecordTest.*'
```

These tests create real media files and should only run when payload storage is
available.
