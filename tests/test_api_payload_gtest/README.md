# Payload SDK GoogleTest suite

This directory contains integration and packet-level tests for
`PayloadSdkInterface`. The suite covers camera, stream, gimbal, tracking,
telemetry, and system-control APIs.

Most tests communicate with a real Gremsy payload. Run motion, capture,
recording, tracking, standby, calibration, and restart tests only on a secured
bench.

## Test structure

```text
test_api_payload_gtest/
├── common/                       Shared fixture, callbacks, and wait helpers
├── camera/
│   ├── query/                    Information, mode, storage, status, and FOV
│   ├── parameters/               Catalog, list, ID/index lookup, and set/get
│   ├── media/                    Image capture and video recording
│   └── control/                  Zoom, focus, IR FFC, and white balance
├── stream/                       Information, bitrate, resolution, profile, rate
├── gimbal/                       Parameters, motion, calibration, home, autotune
├── tracking/                     Runtime mode and tracking position/ROI
├── telemetry/                    GPS position, GPS raw, and system time packets
├── system/                       Standby and application restart
├── CMakeLists.txt
└── run_with_report.sh
```

The detailed architecture and planned coverage are documented in
[`API_TEST_DESIGN.md`](API_TEST_DESIGN.md) and
[`TEST_COVERAGE_PLAN.md`](TEST_COVERAGE_PLAN.md).

## Prerequisites

- A C++ compiler and CMake
- GoogleTest development files discoverable by CMake
- The Payload SDK dependencies required by the selected product build
- A payload reachable through the connection configured in
  `libs/payloadsdk.h`
- For the current UDP build, the default target is `192.168.16.211:14566`

The global test environment creates one `PayloadSdkInterface`, registers all
callbacks, waits for a camera or gimbal connection, and probes camera
information before running tests. Consequently, even host-side loopback tests
currently start after the real-payload connection step.

## Build

From the `PayloadSdk` directory:

```bash
cmake -S . -B build
cmake --build build --target test_api_payload_gtest -j"$(nproc)"
```

Test executable:

```text
build/tests/test_api_payload_gtest/test_api_payload_gtest
```

## List tests

From `PayloadSdk/build`:

```bash
./tests/test_api_payload_gtest/test_api_payload_gtest --gtest_list_tests
```

## Run tests

Run every enabled test:

```bash
./tests/test_api_payload_gtest/test_api_payload_gtest
```

Run one test:

```bash
./tests/test_api_payload_gtest/test_api_payload_gtest \
  --gtest_filter=CameraFocusTest.VisualObserve_InStopOutStop
```

Use quotes around filters containing `*` so the shell does not expand them.

### Common filters

```bash
# Camera
./tests/test_api_payload_gtest/test_api_payload_gtest \
  --gtest_filter='Camera*:*SupportedModes*'

# Stream
./tests/test_api_payload_gtest/test_api_payload_gtest \
  --gtest_filter='Stream*'

# Gimbal parameters only (no motion)
./tests/test_api_payload_gtest/test_api_payload_gtest \
  --gtest_filter='GimbalParam*'

# Gimbal, including enabled motion tests
./tests/test_api_payload_gtest/test_api_payload_gtest \
  --gtest_filter='Gimbal*'

# Tracking
./tests/test_api_payload_gtest/test_api_payload_gtest \
  --gtest_filter='Tracking*'

# Telemetry packet encoding
./tests/test_api_payload_gtest/test_api_payload_gtest \
  --gtest_filter='GpsPositionTest.*:GpsRawTest.*:SystemTimeTest.*'

# Standby
./tests/test_api_payload_gtest/test_api_payload_gtest \
  --gtest_filter='Standby*'

# Safe restart-command encoding; does not restart physical applications
./tests/test_api_payload_gtest/test_api_payload_gtest \
  --gtest_filter='SupportedApps/RestartAppEncodingTest.*'
```

GoogleTest continues with later test cases after an ordinary assertion failure.
Use `--gtest_fail_fast` only when the run should stop after the first failed
test.

## Reports

Use the report wrapper from the test directory:

```bash
./tests/test_api_payload_gtest/run_with_report.sh
```

Pass normal GoogleTest arguments through the wrapper:

```bash
./tests/test_api_payload_gtest/run_with_report.sh \
  --gtest_filter='TrackingModeTest.*'
```

Reports are written under:

```text
PayloadSdk/reports/gtest/YYYY-MM-DD_HH-MM-SS/
├── console.log
└── test_api_payload_gtest.xml
```

Set `GTEST_REPORT_FORMAT=json` to request JSON instead of XML:

```bash
GTEST_REPORT_FORMAT=json \
  ./tests/test_api_payload_gtest/run_with_report.sh
```

## Disabled and disruptive tests

Tests prefixed with `DISABLED_` do not run normally. These include operations
that recalibrate hardware or restart payload services:

- Gyroscope, accelerometer, and motor calibration
- Gimbal home search
- Gimbal autotune
- Payload, streaming, tracking, web UI, and gimbal application restart

Run only one disruptive test at a time with a narrow filter. Example:

```bash
./tests/test_api_payload_gtest/test_api_payload_gtest \
  --gtest_also_run_disabled_tests \
  --gtest_filter='SupportedApps/RestartAppHardwareTest.DISABLED_Restart_RecoversWithinTimeout/Streaming'
```

Before running a disruptive test:

1. Secure the payload and provide unobstructed gimbal movement.
2. Stop aircraft or vehicle operation.
3. Confirm the network connection and power supply are stable.
4. Run only the intended filtered test.
5. Verify payload recovery before starting another test.

## State restoration

Tests that change configuration use cleanup where the SDK permits it. Examples
include restoring camera parameters, zoom, focus, stream bitrate/resolution,
gimbal parameters, and tracking mode. Motion tests send a stop command and try
to return to their initial pose. Standby tests always send standby-disable in
cleanup.

Cleanup is best effort. If communication is lost or firmware stops responding,
inspect the physical payload state before running another test.

## Test verification model

The suite prefers observable state over a generic command ACK:

- Camera and stream setters are checked through settings, FOV, capture status,
  stream information, or parameter readback.
- Command-specific ACK tracking prevents an unrelated ACK from satisfying a
  waiter.
- Telemetry and safe system-encoding tests capture the actual SDK-generated
  MAVLink datagram through an ephemeral loopback UDP listener.
- Optional IR tests skip when stream or IR-specific feedback is unavailable.

Some SDK APIs have no paired getter. For example, stream profile verification
uses a command-specific ACK followed by a stream-health query. The current
`getPayloadStreamBitrate()` implementation dispatches an asynchronous request;
the actual bitrate arrives through `regPayloadStreamChanged()`.

## Troubleshooting

### Payload is physically stable but a test reports no response

Physical stability does not prove that the expected MAVLink response was sent.
Confirm that the test requested the correct message and that the relevant
callback sequence advanced. Common causes include:

- The firmware applies a command but does not send `COMMAND_ACK`.
- The camera does not periodically publish `CAMERA_SETTINGS`.
- Autofocus keeps the reported manual focus value unchanged.
- An IR stream or feature is not installed on the current payload.
- A response arrived for a different stream, parameter ID, or command.

### Camera is connected but camera-targeted tests fail

`checkPayloadConnection()` returns when either a camera or a gimbal component is
discovered. The global environment separately probes camera information and
prints a warning if the camera component cannot be confirmed. Check the payload
IP, UDP port, product firmware, and camera component before continuing.

### Test changed hardware state before failing

Stop the affected feature manually before rerunning:

- Stop recording or image sequences.
- Send focus/zoom STOP.
- Send zero gimbal speed.
- Stop object tracking.
- Disable standby.

Then run the smallest relevant test filter rather than the entire suite.

## Adding tests

Place new `.cpp` files in the matching feature directory. The test target uses
`file(GLOB_RECURSE ... CONFIGURE_DEPENDS)`, so rerun CMake after adding files:

```bash
cmake -S . -B build
cmake --build build --target test_api_payload_gtest -j"$(nproc)"
```

New hardware-changing tests should:

- Capture original state when a getter exists.
- Restore it in `TearDown()` or an RAII guard.
- Use command-specific ACK tracking rather than `lastAck`.
- Prefer state/readback verification over ACK-only verification.
- Use `GTEST_SKIP()` for optional unsupported hardware.
- Prefix destructive or service-interrupting cases with `DISABLED_`.
