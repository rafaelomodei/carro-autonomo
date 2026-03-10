#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

TARGET="${1:-}"
if [[ "${TARGET}" == "--vision-debug" || "${TARGET}" == "--hardware" ]]; then
  shift
fi

if [[ "${TARGET}" == "--vision-debug" ]]; then
  EXECUTABLE="${BUILD_DIR}/autonomous_car_v3_vision_debug"
elif [[ -x "${BUILD_DIR}/autonomous_car_v3" ]]; then
  EXECUTABLE="${BUILD_DIR}/autonomous_car_v3"
elif [[ -x "${BUILD_DIR}/autonomous_car_v3_vision_debug" ]]; then
  EXECUTABLE="${BUILD_DIR}/autonomous_car_v3_vision_debug"
  echo "autonomous_car_v3 indisponivel; iniciando fallback em autonomous_car_v3_vision_debug."
else
  echo "Nenhum executavel encontrado. Execute ./build.sh primeiro." >&2
  exit 1
fi

exec "${EXECUTABLE}" "$@"
