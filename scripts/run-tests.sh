#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ZIG="${ZIG:-zig}"
OUT="${ROOT}/zig-out/kz_global_api_tests"

mkdir -p "${ROOT}/zig-out"

"${ZIG}" c++ -std=c++17 -I"${ROOT}/src/kz_global_api/include" \
  "${ROOT}/src/kz_global_api/kz_path_validate.cpp" \
  "${ROOT}/src/kz_global_api/krp_header_validate.cpp" \
  "${ROOT}/src/kz_global_api/kz_replay_uid.cpp" \
  "${ROOT}/src/kz_global_api/test/test_main.cpp" \
  "${ROOT}/src/kz_global_api/test/path_validate_test.cpp" \
  "${ROOT}/src/kz_global_api/test/krp_validate_test.cpp" \
  "${ROOT}/src/kz_global_api/test/replay_uid_test.cpp" \
  -o "${OUT}"

"${OUT}"
