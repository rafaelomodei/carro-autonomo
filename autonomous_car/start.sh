#!/bin/bash
set -euo pipefail

# Definir cores
GREEN="\033[1;32m"
RED="\033[1;31m"
BLUE="\033[1;34m"
RESET="\033[0m"

# Funções auxiliares para logs
log_info() {
  echo -e "${BLUE}ℹ️ $1${RESET}"
}
log_success() {
  echo -e "${GREEN}✅ $1${RESET}"
}
log_error() {
  echo -e "${RED}❌ $1${RESET}"
}

log_info " Ligando o veículo..."

# Verificar se o diretório de build existe
if [ -d "build" ]; then
  cd build
  log_success " Diretório de build acessado."
else
  log_error " Diretório de build não encontrado. Certifique-se de que o build foi concluído."
  exit 1
fi

# Verificar se o executável existe
if [ -f ./autonomous_car ]; then
  log_info " Iniciando o programa..."

  ./autonomous_car &
  CAR_PID=$!

  # Função para encerrar o processo de forma graciosa
  cleanup() {
    if kill -0 "$CAR_PID" 2>/dev/null; then
      log_info " Encerrando programa..."
      kill "$CAR_PID" 2>/dev/null
      wait "$CAR_PID" 2>/dev/null || true
    fi
  }
  trap cleanup SIGINT SIGTERM

  # Aguarda término do executável
  wait "$CAR_PID"
  STATUS=$?
  if [ $STATUS -ne 0 ]; then
    log_error " Programa finalizado com erro ($STATUS)."
  else
    log_success " Programa encerrado."
  fi
else
  log_error " Executável 'autonomous_car' não encontrado. Certifique-se de que o build foi concluído com sucesso."
  exit 1
fi
