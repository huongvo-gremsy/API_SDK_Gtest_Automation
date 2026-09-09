# EO camera example integration tests

This directory tests the public SDK flows demonstrated by:

- `examples/camera_eo_capture_image.cpp`
- `examples/camera_eo_record_video.cpp`
- `examples/camera_eo_set_shutter_speed.cpp`
- `examples/camera_eo_set_zoom_focus.cpp`

It also covers EO Auto, Outdoor, Indoor, Manual, ATW, and One-Push white
balance. Tests use fresh state readback where available, command-specific ACKs
as a fallback, and EO stream health when firmware omits an ACK. Fixtures stop
active media operations and restore every setting they save.

The focus movement case contains countdowns for visual verification on the EO
screen. An accepted command proves transport/control acceptance; the operator
must observe image sharpness to verify the lens' physical response.
