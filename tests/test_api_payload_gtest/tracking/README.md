# Tracking API tests

The fixture selects object-tracking mode, enables `TRK_POS_*` and `TRK_STATUS`
telemetry, stops runtime tracking in cleanup, restores the original camera
tracking algorithm, and resets telemetry intervals to their defaults.

The coordinate tests use the SDK's documented 1920x1080 tracking coordinate
space. A real visible target should be present for the strongest ROI readback
verification.
