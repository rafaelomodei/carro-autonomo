#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

cmake ..
cmake --build .

if [[ -x "${BUILD_DIR}/autonomous_car_v3" ]]; then
  echo "Binario de hardware disponivel: ${BUILD_DIR}/autonomous_car_v3"
else
  echo "Binario de hardware nao foi gerado (wiringPi ausente)."
fi

if [[ -x "${BUILD_DIR}/autonomous_car_v3_vision_debug" ]]; then
  echo "Binario de visao/debug disponivel: ${BUILD_DIR}/autonomous_car_v3_vision_debug"
fi
