#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
EXECUTABLE="${BUILD_DIR}/autonomous_car_v3"

if [[ ! -x "${EXECUTABLE}" ]]; then
  echo "Executável não encontrado. Execute ./build.sh primeiro." >&2
  exit 1
fi

"${EXECUTABLE}"
