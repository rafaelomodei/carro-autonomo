#!/bin/bash

# Definir cores
GREEN="\033[1;32m"
RED="\033[1;31m"
BLUE="\033[1;34m"
RESET="\033[0m"

# Arrays para armazenar pacotes instalados com sucesso e com falha
success_packages=()
failed_packages=()

# Função para instalar pacotes e capturar o resultado
install_package() {
  local package_name=$1
  echo -e "${BLUE}🛠️ Instalando $package_name...${RESET}"
  if sudo apt install -y "$package_name"; then
    success_packages+=("$package_name")
  else
    failed_packages+=("$package_name")
  fi
}

# Atualizar o sistema
echo -e "${BLUE}🔧 Atualizando o sistema...${RESET}"
if sudo apt update && sudo apt upgrade -y; then
  echo -e "${GREEN}✅ Sistema atualizado com sucesso.${RESET}"
else
  echo -e "${RED}❌ Falha ao atualizar o sistema.${RESET}"
  exit 1
fi

# Instalar dependências
echo -e "${BLUE}🔧 Instalando dependências básicas...${RESET}"
install_package "build-essential"
install_package "cmake"
install_package "libboost-all-dev"
install_package "git"
install_package "rapidjson-dev"
install_package "pigpio"
install_package "python3-pigpio"
install_package "libopencv-dev"
install_package "v4l-utils"

# Exibir resultados
echo -e "${BLUE}🛠️ Configuração concluída!${RESET}"
echo
echo "### Relatório de Instalação ###"
if [ ${#success_packages[@]} -gt 0 ]; then
  echo -e "${GREEN}✅ Pacotes instalados com sucesso:${RESET}"
  for pkg in "${success_packages[@]}"; do
    echo -e "${GREEN}  🌟 - $pkg${RESET}"
  done
else
  echo -e "${RED}❌ Nenhum pacote foi instalado com sucesso.${RESET}"
fi

if [ ${#failed_packages[@]} -gt 0 ]; then
  echo
  echo -e "${RED}❌ Pacotes que falharam na instalação:${RESET}"
  for pkg in "${failed_packages[@]}"; do
    echo -e "${RED}  💔 - $pkg${RESET}"
  done
else
  echo
  echo -e "${GREEN}✅ Todos os pacotes foram instalados com sucesso!${RESET}"
fi
