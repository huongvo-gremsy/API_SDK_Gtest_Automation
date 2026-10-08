# Stream API tests

| File | SDK APIs covered |
|---|---|
| `test_stream_information.cpp` | `regPayloadStreamChanged()`, `getPayloadCameraStreamingInformation()` |
| `test_stream_bitrate.cpp` | `setPayloadStreamBitrate()`, `getPayloadStreamBitrate()` |
| `test_stream_resolution.cpp` | `setPayloadStreamResolution()` |
| `test_stream_profile.cpp` | `setPayloadStreamProfile()` |
| `test_stream_rate.cpp` | `sendPayloadRequestStreamRate()` |

Stream IDs are `0 = default`, `1 = EO`, and `2 = IR`. Tests that require IR
skip when stream 2 is unavailable. Mutating bitrate and resolution tests should
restore their baseline value. The public stream callback currently exposes
type, height, width, bitrate, and stream ID; it does not expose encoder profile,
so profile verification uses a command-specific ACK and a stream health query.
