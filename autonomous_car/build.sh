#!/bin/bash

# Criar o diretório de build e compilar
echo "Iniciando o build..."
mkdir -p build && cd build
cmake ..
make
