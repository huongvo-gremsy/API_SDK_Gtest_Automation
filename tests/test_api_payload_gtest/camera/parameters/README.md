# Camera parameter tests

This folder owns the camera PARAM_EXT APIs.

| File | API | Coverage |
|---|---|---|
| `test_camera_param_list.cpp` | `getPayloadCameraSettingList()` | Responses, unique/bounded IDs and indices, finite values, stable repeated list |
| `test_camera_param_by_id.cpp` | `getPayloadCameraSettingByID()` | Exact ID routing, list consistency, repeated/unknown/16-byte IDs |
| `test_camera_param_by_index.cpp` | `getPayloadCameraSettingByIndex()` | First/middle/last, all listed indices, invalid index |
| `test_camera_param_set.cpp` | `setPayloadCameraParam()` | Readback, restoration, binary zero bytes, boundaries, invalid ID/value |
| `test_camera_param_catalog.cpp` | all product parameter IDs | Catalog validity/readability and representative source, exposure, focus, zoom, IR, and control values |

All setter tests read and restore the original value. Run from
`PayloadSdk/build`:

```bash
./tests/test_api_payload_gtest/test_api_payload_gtest \
  --gtest_filter='CameraParam*'
```

Read-only tests:

```bash
./tests/test_api_payload_gtest/test_api_payload_gtest \
  --gtest_filter='CameraParamListTest.*:CameraParamByIdTest.*:CameraParamByIndexTest.*'
```
