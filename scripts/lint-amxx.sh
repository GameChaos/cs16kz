#!/usr/bin/env bash
# AMXX Metamod string API guard.
#
# MF_GetAmxString / MF_GetAmxStringNull return pointers into AMXX-managed buffers.
# The third argument is a buffer slot id (0, 1, …), not a char[] — never pass sizeof().
# Wrong (HLSDK-style): MF_GetAmxString(amx, params[1], mapname, sizeof(mapname))
# Right:              char* mapname = MF_GetAmxString(amx, params[1], 0, &len)
#
# See kz_api_get_map_details in src/kz_global_api/kz_natives.cpp.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PATTERN='MF_GetAmxString(Null)?.*sizeof'

if rg --pcre2 -q "${PATTERN}" "${ROOT}/src/"; then
  echo "::error::MF_GetAmxString misuse — returns AMXX-managed char*; do not pass a local buffer or sizeof (see kz_api_get_map_details in src/kz_global_api/kz_natives.cpp)"
  rg --pcre2 -n "${PATTERN}" "${ROOT}/src/"
  exit 1
fi

echo "AMXX string API lint: ok"
