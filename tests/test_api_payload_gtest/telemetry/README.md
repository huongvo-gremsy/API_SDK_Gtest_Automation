# Telemetry injection tests

These tests verify the actual outbound MAVLink packets produced by
`PayloadSdkInterface`. Each test creates a temporary SDK connection to an
ephemeral loopback UDP listener, captures the queued datagram, decodes it, and
compares every message field. No GPS or time test packet is sent to the physical
payload.

Covered messages:

- `GLOBAL_POSITION_INT` through `sendPayloadGPSPosition()`
- `GPS_RAW_INT` through `sendPayloadGPSRawInt()`
- `SYSTEM_TIME` through `sendPayloadSystemTime()`
