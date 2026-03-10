#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
EXECUTABLE="${BUILD_DIR}/road_segmentation_lab"
CONFIG_FILE="${SCRIPT_DIR}/config/road_segmentation_lab.env"

if [[ ! -x "${EXECUTABLE}" ]]; then
  echo "Executavel nao encontrado. Execute ./build.sh primeiro." >&2
  exit 1
fi

HAS_CONFIG=false
for arg in "$@"; do
  if [[ "${arg}" == "--config" ]]; then
    HAS_CONFIG=true
    break
  fi
done

ARGS=("$@")
if [[ "${HAS_CONFIG}" == "false" ]]; then
  ARGS+=(--config "${CONFIG_FILE}")
fi

if [[ "${#ARGS[@]}" -eq 2 ]] && [[ "${ARGS[0]}" == "--config" ]]; then
  ARGS+=(--camera-index 0)
fi

exec "${EXECUTABLE}" "${ARGS[@]}"
