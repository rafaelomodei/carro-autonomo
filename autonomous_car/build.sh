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

# Iniciar o processo de build
log_info " Iniciando o build..."

# Limpar diretório anterior
if rm -rf build/*; then
  log_success "Diretório de build limpo."
else
  log_error "Falha ao limpar o diretório de build."
  exit 1
fi

# Criar e acessar o diretório de build
if mkdir -p build && cd build; then
  log_success "Diretório de build criado e acessado."
else
  log_error "Falha ao criar ou acessar o diretório de build."
  exit 1
fi

# Executar o CMake
log_info " Executando o CMake..."
if cmake ..; then
  log_success "CMake configurado com sucesso."
else
  log_error "Falha na configuração do CMake."
  exit 1
fi

# Executar o Make
log_info " Compilando o projeto..."
if make; then
  log_success "Build concluído com sucesso."
else
  log_error " Falha durante o build."
  exit 1
fi
