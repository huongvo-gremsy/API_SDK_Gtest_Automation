# Gimbal API integration tests

The example-oriented cases cover:

- `examples/gimbal_change_settings.cpp`: read, set to 50, verify, and restore
  `STIFF_TILT`.
- `examples/gimbal_move_angle.cpp`: MAVLink-v1 and v2 pitch/yaw targets verified
  through passive `MOUNT_ORIENTATION` telemetry.
- `examples/gimbal_move_speed.cpp`: positive, negative, and stop speed commands
  verified through direction and stability measurements.
- `examples/gimbal_set_mode.cpp`: LOCK, FOLLOW, and MAPPING parameter readback.

Every motion test stops the gimbal and attempts to restore its initial attitude,
RC mode, and gimbal mode. OFF and RESET are disabled by default because they can
remove stabilization or cause a large physical movement. Run those only on a
cleared bench with an operator ready to intervene.
