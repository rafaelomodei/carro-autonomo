#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
CONFIG_FILE="${1:-${SCRIPT_DIR}/config/service.env}"
CONFIG_DIR="$(cd "$(dirname "${CONFIG_FILE}")" && pwd)"

resolve_path() {
  local base_dir="$1"
  local raw_path="$2"

  if [[ -z "${raw_path}" ]]; then
    printf '\n'
    return
  fi

  if [[ "${raw_path}" = /* ]]; then
    printf '%s\n' "${raw_path}"
    return
  fi

  (
    cd "${base_dir}"
    realpath "${raw_path}"
  )
}

if [[ ! -x "${BUILD_DIR}/traffic_sign_service" ]]; then
  "${SCRIPT_DIR}/build.sh"
fi

if [[ -f "${CONFIG_FILE}" ]]; then
  set -a
  # shellcheck disable=SC1090
  source "${CONFIG_FILE}"
  set +a

  if [[ "${EDGE_IMPULSE_BACKEND:-}" == "eim" ]]; then
    EIM_PATH="$(resolve_path "${CONFIG_DIR}" "${EDGE_IMPULSE_EIM_PATH:-}")"
    if [[ -z "${EIM_PATH}" || ! -x "${EIM_PATH}" ]]; then
      "${SCRIPT_DIR}/build_linux_eim.sh" "${CONFIG_FILE}"
    fi
  fi
fi

cd "${SCRIPT_DIR}"
exec "${BUILD_DIR}/traffic_sign_service" "${CONFIG_FILE}"
