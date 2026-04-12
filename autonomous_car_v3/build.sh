#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
GENERATOR="${BUILD_GENERATOR:-Ninja}"

detect_jobs() {
  if [[ -n "${BUILD_JOBS:-}" ]]; then
    echo "${BUILD_JOBS}"
    return
  fi

  if command -v nproc >/dev/null 2>&1; then
    nproc
    return
  fi

  getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4
}

detect_ninja_command() {
  if command -v ninja >/dev/null 2>&1; then
    echo "ninja"
    return
  fi

  if command -v ninja-build >/dev/null 2>&1; then
    echo "ninja-build"
    return
  fi

  return 1
}

ensure_generator_cache_is_compatible() {
  local cache_file="${BUILD_DIR}/CMakeCache.txt"

  if [[ ! -f "${cache_file}" ]]; then
    return
  fi

  local cached_generator
  local cached_build_dir
  local cached_source_dir
  cached_generator="$(sed -n 's/^CMAKE_GENERATOR:INTERNAL=//p' "${cache_file}" | head -n 1)"
  cached_build_dir="$(sed -n 's/^CMAKE_CACHEFILE_DIR:INTERNAL=//p' "${cache_file}" | head -n 1)"
  cached_source_dir="$(sed -n 's/^CMAKE_HOME_DIRECTORY:INTERNAL=//p' "${cache_file}" | head -n 1)"

  if [[ -n "${cached_generator}" && "${cached_generator}" != "${GENERATOR}" ]]; then
    echo "Build existente usa '${cached_generator}'. Limpando ${BUILD_DIR} para reconfigurar com '${GENERATOR}'."
    rm -rf "${BUILD_DIR}"
    return
  fi

  if [[ -n "${cached_build_dir}" && "${cached_build_dir}" != "${BUILD_DIR}" ]]; then
    echo "Build existente aponta para outro diretorio (${cached_build_dir}). Limpando ${BUILD_DIR} para evitar cache invalido."
    rm -rf "${BUILD_DIR}"
    return
  fi

  if [[ -n "${cached_source_dir}" && "${cached_source_dir}" != "${SCRIPT_DIR}" ]]; then
    echo "Build existente foi configurado a partir de outro source dir (${cached_source_dir}). Limpando ${BUILD_DIR} para reconfigurar."
    rm -rf "${BUILD_DIR}"
  fi
}

if [[ "${GENERATOR}" == "Ninja" ]]; then
  NINJA_COMMAND="$(detect_ninja_command)" || {
    echo "Ninja nao encontrado. Instale com 'sudo apt install ninja-build' ou defina BUILD_GENERATOR para outro gerador." >&2
    exit 1
  }
fi

BUILD_JOBS_DETECTED="$(detect_jobs)"

ensure_generator_cache_is_compatible
mkdir -p "${BUILD_DIR}"

cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" -G "${GENERATOR}"
cmake --build "${BUILD_DIR}" --parallel "${BUILD_JOBS_DETECTED}"

if [[ "${GENERATOR}" == "Ninja" ]]; then
  echo "Build configurado com Ninja (${NINJA_COMMAND}) usando ${BUILD_JOBS_DETECTED} jobs."
else
  echo "Build configurado com ${GENERATOR} usando ${BUILD_JOBS_DETECTED} jobs."
fi

if [[ -x "${BUILD_DIR}/autonomous_car_v3" ]]; then
  echo "Binario de hardware disponivel: ${BUILD_DIR}/autonomous_car_v3"
else
  echo "Binario de hardware nao foi gerado (wiringPi ausente)."
fi

if [[ -x "${BUILD_DIR}/autonomous_car_v3_vision_debug" ]]; then
  echo "Binario de visao/debug disponivel: ${BUILD_DIR}/autonomous_car_v3_vision_debug"
fi
