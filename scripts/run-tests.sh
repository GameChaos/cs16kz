#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ZIG="${ZIG:-zig}"
OUT="${ROOT}/zig-out/kz_global_api_tests"

mkdir -p "${ROOT}/zig-out"

"${ZIG}" c++ -std=c++17 -I"${ROOT}/src/include" \
  "${ROOT}/src/kz_path_validate.cpp" \
  "${ROOT}/src/krp_header_validate.cpp" \
  "${ROOT}/src/kz_replay_uid.cpp" \
  "${ROOT}/src/test/test_main.cpp" \
  "${ROOT}/src/test/path_validate_test.cpp" \
  "${ROOT}/src/test/krp_validate_test.cpp" \
  "${ROOT}/src/test/replay_uid_test.cpp" \
  -o "${OUT}"

"${OUT}"
