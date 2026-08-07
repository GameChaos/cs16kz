#!/bin/bash
set -euo pipefail

#TODO: ReleaseFast crashes, ReleaseSafe temporary
zig build -Dtarget=x86-linux-gnu -Doptimize=ReleaseSafe "$@"
