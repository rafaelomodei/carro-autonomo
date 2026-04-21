#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
CONFIG_FILE="${SCRIPT_DIR}/config/service.env"
CONFIG_DIR="$(cd "$(dirname "${CONFIG_FILE}")" && pwd)"

resolve_path() {
  local base_dir="$1"
  local raw_path="$2"

  if [[ "${raw_path}" = /* ]]; then
    printf '%s\n' "${raw_path}"
    return
  fi

  (
    cd "${base_dir}"
    realpath "${raw_path}"
  )
}

if [[ -f "${CONFIG_FILE}" ]]; then
  set -a
  # shellcheck disable=SC1090
  source "${CONFIG_FILE}"
  set +a
fi

MODEL_ZIP="${EDGE_IMPULSE_MODEL_ZIP:-${SCRIPT_DIR}/../edgeImpulse/tcc-pare-direita-esquerda-cam-raspberry-cpp-linux-v20-impulse.zip}"
MODEL_ZIP="$(resolve_path "${CONFIG_DIR}" "${MODEL_ZIP}")"

cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" \
  -DTRAFFIC_SIGN_SERVICE_MODEL_ZIP="${MODEL_ZIP}"

cmake --build "${BUILD_DIR}" --parallel

echo "Binario disponivel em ${BUILD_DIR}/traffic_sign_service"
echo "Testes disponiveis em ${BUILD_DIR}/traffic_sign_service_tests"
