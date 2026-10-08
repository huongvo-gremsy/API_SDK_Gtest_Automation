# IR camera example integration tests

These tests mirror the SDK examples for IR capture, recording, FFC, isotherm
profiles, palette selection, and zoom. The shared fixture requires IR stream ID
2, selects IR view/record sources, stops leftover recording, and restores the
original camera source and mode after each test.

The isotherm tests require VIO F1 firmware v3.0.3 or newer. Tests skip when the
IR stream or required isotherm parameters are unavailable.
