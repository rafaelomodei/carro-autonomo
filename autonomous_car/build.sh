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
log_info "Iniciando o build..."

# Criar o diretório de build, caso ainda não exista
if [ ! -d "build" ]; then
  mkdir -p build
  log_success "Diretório de build criado."
else
  log_info "Diretório de build já existe. Utilizando cache."
fi

# Acessar o diretório de build
if cd build; then
  log_success "Diretório de build acessado."
else
  log_error "Falha ao acessar o diretório de build."
  exit 1
fi

# Executar o CMake
log_info "Executando o CMake..."
if cmake ..; then
  log_success "CMake configurado com sucesso."
else
  log_error "Falha na configuração do CMake."
  exit 1
fi

# Compilar o projeto utilizando cache
log_info "Compilando o projeto..."
if make -j$(nproc); then
  log_success "Build concluído com sucesso."
else
  log_error "Falha durante o build."
  exit 1
fi
