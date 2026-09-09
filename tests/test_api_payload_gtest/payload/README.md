# Payload example API tests

This suite maps the Payload SDK calls used by the following examples to
GoogleTest coverage:

- object detection and object tracking;
- component information, FOV, general status, record status, and streaming;
- camera zoom target position;
- GPS raw/global position and system time;
- standby mode and application restart;
- stream bitrate, resolution, and encoder profile.

Read-only and reversible operations run as live integration tests. Packet-only
APIs are also verified through a loopback UDP receiver, which checks the real
SDK encode/queue/write path without injecting navigation data or disruptive
commands into the connected payload. Restart and standby hardware tests remain
disabled by default and require an explicit GoogleTest opt-in.

The fixtures restore tracking algorithm, EO view, OSD mode, recording state,
camera mode/source, zoom mode/position, and stream bitrate when changed.
