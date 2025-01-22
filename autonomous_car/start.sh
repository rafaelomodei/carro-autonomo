#!/bin/bash

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

# Iniciar o programa
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
if [ -f "./autonomous_car" ]; then
  log_info " Iniciando o programa..."
  ./autonomous_car
  log_success " Programa encerrado."
else
  log_error " Executável 'autonomous_car' não encontrado. Certifique-se de que o build foi concluído com sucesso."
  exit 1
fi
