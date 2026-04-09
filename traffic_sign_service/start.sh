#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
CONFIG_FILE="${1:-${SCRIPT_DIR}/config/service.env}"

if [[ ! -x "${BUILD_DIR}/traffic_sign_service" ]]; then
  "${SCRIPT_DIR}/build.sh"
fi

cd "${SCRIPT_DIR}"
exec "${BUILD_DIR}/traffic_sign_service" "${CONFIG_FILE}"
