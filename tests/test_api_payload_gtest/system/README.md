# System control tests

Standby tests always disable standby during cleanup and require both heartbeat
and camera-information recovery. Restart command encoding is verified safely on
loopback for application IDs 0 through 4.

Real application restart tests are `DISABLED_*` because they intentionally
interrupt payload services. Run one at a time on a secured bench with
`--gtest_also_run_disabled_tests` and a narrow filter.
