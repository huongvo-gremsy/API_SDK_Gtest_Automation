# Camera IR tests

These tests require a connected payload and an IR camera. Run them from the
build directory after rebuilding the test target:

```bash
cmake --build build --target test_api_payload_gtest -j"$(nproc)"
./build/tests/test_api_payload_gtest/test_api_payload_gtest \
  --gtest_filter='*CameraIr*'
```

The parameter tests set each documented value and request the same parameter by
ID to verify the callback readback. Supported values can differ between G1 and
F1 sensors; a failed readback identifies a firmware or sensor capability
difference, not merely a missing SDK macro.

`getPayloadCameraFFCMode()` is intentionally not tested for readback because
the SDK implementation is empty. The FFC tests verify that the set and trigger
commands can be sent. TIFF temperature extraction is an offline file-processing
test and should be added separately with a valid thermal TIFF fixture.
