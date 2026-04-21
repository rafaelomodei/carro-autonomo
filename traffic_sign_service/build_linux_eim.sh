#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_ROOT="${SCRIPT_DIR}/build"
SDK_DIR="${BUILD_ROOT}/edge_impulse_linux_sdk"
CONFIG_FILE="${1:-${SCRIPT_DIR}/config/service.env}"
CONFIG_DIR="$(cd "$(dirname "${CONFIG_FILE}")" && pwd)"
DEFAULT_MODEL_ZIP="${SCRIPT_DIR}/../edgeImpulse/tcc-pare-direita-esquerda-cam-raspberry-cpp-linux-v20-impulse.zip"

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

if [[ -f "${CONFIG_FILE}" ]]; then
  set -a
  # shellcheck disable=SC1090
  source "${CONFIG_FILE}"
  set +a
fi

MODEL_ZIP_RAW="${EDGE_IMPULSE_MODEL_ZIP:-${DEFAULT_MODEL_ZIP}}"
MODEL_ZIP="$(resolve_path "${CONFIG_DIR}" "${MODEL_ZIP_RAW}")"
if [[ ! -f "${MODEL_ZIP}" ]]; then
  echo "ZIP do Edge Impulse nao encontrado: ${MODEL_ZIP}" >&2
  exit 1
fi

case "$(uname -m)" in
  x86_64|amd64)
    TARGET_FLAG="TARGET_LINUX_X86=1"
    ;;
  aarch64|arm64)
    TARGET_FLAG="TARGET_LINUX_AARCH64=1"
    ;;
  armv7l)
    TARGET_FLAG="TARGET_LINUX_ARMV7=1"
    ;;
  *)
    echo "Arquitetura Linux nao suportada por este script: $(uname -m)" >&2
    exit 1
    ;;
esac

mkdir -p "${BUILD_ROOT}"

if [[ ! -d "${SDK_DIR}/.git" ]]; then
  git clone --depth 1 https://github.com/edgeimpulse/example-standalone-inferencing-linux \
    "${SDK_DIR}"
fi

git -C "${SDK_DIR}" submodule update --init --recursive

rm -rf "${SDK_DIR}/edge-impulse-sdk" \
       "${SDK_DIR}/model-parameters" \
       "${SDK_DIR}/tflite-model"
rm -f "${SDK_DIR}/README.txt" "${SDK_DIR}/CMakeLists.txt" "${SDK_DIR}/model.zip"

cp "${MODEL_ZIP}" "${SDK_DIR}/model.zip"
unzip -oq "${SDK_DIR}/model.zip" -d "${SDK_DIR}"

MAKE_JOBS="$(nproc)"
(
  cd "${SDK_DIR}"
  make -j"${MAKE_JOBS}" APP_EIM=1 "${TARGET_FLAG}" USE_FULL_TFLITE=1
)

MODEL_EIM="${SDK_DIR}/build/model.eim"
if [[ ! -x "${MODEL_EIM}" ]]; then
  echo "Falha ao gerar o model.eim em ${MODEL_EIM}" >&2
  exit 1
fi

echo "Model EIM disponivel em ${MODEL_EIM}"
echo "[info] No fluxo oficial do Edge Impulse para Linux x86_64, este build usa TFLite Full."
echo "[info] A aceleracao GPU/TensorRT documentada pelo Edge Impulse permanece no caminho Jetson."
