#!/bin/bash
set -euo pipefail

GREEN="\033[1;32m"; RED="\033[1;31m"; BLUE="\033[1;34m"; RESET="\033[0m"
info(){ echo -e "${BLUE}ℹ️ $1${RESET}"; }
ok(){ echo -e "${GREEN}✅ $1${RESET}"; }
error(){ echo -e "${RED}❌ $1${RESET}"; }

info "Iniciando servidor WebSocket..."

if [ -d "build" ]; then
  cd build
  ok "Diretório de build acessado."
else
  error "Diretório de build não encontrado. Compile o projeto antes."
  exit 1
fi

if [ -f ./autonomous_car_v2 ]; then
  ./autonomous_car_v2 &
  PID=$!

  cleanup(){
    if kill -0 "$PID" 2>/dev/null; then
      info "Encerrando programa..."
      kill "$PID" 2>/dev/null
      wait "$PID" 2>/dev/null || true
    fi
  }
  trap cleanup SIGINT SIGTERM

  wait "$PID"
  STATUS=$?
  if [ $STATUS -ne 0 ]; then
    error "Programa finalizado com erro ($STATUS)."
  else
    ok "Programa encerrado."
  fi
else
  error "Executável não encontrado."
  exit 1
fi
