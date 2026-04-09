#!/usr/bin/env bash
set -euo pipefail

sudo apt-get update
sudo apt-get install -y \
  cmake \
  g++ \
  make \
  libopencv-dev \
  libwebsocketpp-dev \
  libasio-dev \
  nlohmann-json3-dev \
  unzip
