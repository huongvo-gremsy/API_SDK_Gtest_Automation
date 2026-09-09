#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
TEST_BIN="${PROJECT_ROOT}/build/tests/test_api_payload_gtest/test_api_payload_gtest"
REPORT_ROOT="${PROJECT_ROOT}/reports/gtest"
RUN_ID="$(date +%Y-%m-%d_%H-%M-%S)"
REPORT_DIR="${REPORT_ROOT}/${RUN_ID}"
REPORT_FORMAT="${GTEST_REPORT_FORMAT:-xml}"
REPORT_FILE="${REPORT_DIR}/test_api_payload_gtest.${REPORT_FORMAT}"

mkdir -p "${REPORT_DIR}"

if [[ ! -x "${TEST_BIN}" ]]; then
  echo "Test binary not found or not executable: ${TEST_BIN}" >&2
  echo "Build it first from ${PROJECT_ROOT}: cmake --build build --target test_api_payload_gtest" >&2
  exit 127
fi

echo "Running: ${TEST_BIN}"
echo "Report directory: ${REPORT_DIR}"
echo "Report format: ${REPORT_FORMAT}"

set +e
"${TEST_BIN}" \
  --gtest_output="${REPORT_FORMAT}:${REPORT_FILE}" \
  "$@" 2>&1 | tee "${REPORT_DIR}/console.log"
status=${PIPESTATUS[0]}
set -e

echo ""
echo "GTest report: ${REPORT_FILE}"
echo "Console   : ${REPORT_DIR}/console.log"

exit "${status}"
