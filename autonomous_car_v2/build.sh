#!/usr/bin/env bash
set -euo pipefail

# cores para mensagens
GREEN="\033[1;32m"; BLUE="\033[1;34m"; RED="\033[1;31m"; RESET="\033[0m"
info(){ echo -e "${BLUE}ℹ️ $1${RESET}"; }
ok(){ echo -e "${GREEN}✅ $1${RESET}"; }
fail(){ echo -e "${RED}❌ $1${RESET}"; exit 1; }

command -v ninja >/dev/null || fail "Ninja não encontrado. Instale com: sudo apt install ninja-build"

info "Limpando build antigo…"
rm -rf build
mkdir build && cd build
ok "Diretório build/ pronto"

info "Configurando CMake (Ninja)…"
cmake -G Ninja .. || fail "Falha no CMake"
ok "CMake OK"

info "Compilando (Ninja)…"
ninja -j$(nproc) || fail "Falha na compilação"
ok "Build concluído com sucesso"
