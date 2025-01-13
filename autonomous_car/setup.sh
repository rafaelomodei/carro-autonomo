#!/bin/bash

# Atualizar o sistema
echo "Atualizando o sistema..."
sudo apt update && sudo apt upgrade -y

# Instalar dependências básicas
echo "Instalando dependências básicas..."
sudo apt install -y build-essential cmake libboost-all-dev git
sudo apt install rapidjson-dev

echo "Configuração concluída!"
