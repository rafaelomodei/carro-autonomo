#!/usr/bin/env bash
set -euo pipefail

# ——— cores p/ logs ———
GREEN="\033[1;32m"; RED="\033[1;31m"; BLUE="\033[1;34m"; RESET="\033[0m"
info(){ echo -e "${BLUE}ℹ️ $1${RESET}"; }
ok  (){ echo -e "${GREEN}✅ $1${RESET}"; }
fail(){ echo -e "${RED}❌ $1${RESET}"; exit 1; }

# -------- Ninja? --------
command -v ninja >/dev/null || fail "Ninja não encontrado. Instale com: sudo apt install ninja-build"

# -------- lite/full -----
CMAKE_ARGS=""
if [[ "${1:-}" == "-DLITE" ]]; then
  info "Modo lite: Edge Impulse desativado"
  CMAKE_ARGS="-DENABLE_EI_INFERENCE=OFF"
else
  info "Modo full: Edge Impulse ativado"
fi

# -------- limpar build ---
info "Limpando build antigo…"
rm -rf build
mkdir build && cd build
ok "Diretório build/ pronto"

# -------- configurar -----
info "Configurando CMake (Ninja)…"
cmake -G Ninja ${CMAKE_ARGS} .. || fail "Falha no CMake"
ok "CMake OK"

# -------- compilar -------
info "Compilando (Ninja)…"
ninja -j$(nproc) || fail "Falha na compilação"
ok "Build concluído com sucesso"
