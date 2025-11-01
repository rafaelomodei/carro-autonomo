#!/usr/bin/env bash
set -euo pipefail

HOST=${HOST:-192.168.15.164}
PORT=${PORT:-8080}
INTERVAL=${INTERVAL:-0.1}
WS_URL="ws://$HOST:$PORT"

log() {
  local message="$1"
  printf '[%s] %s\n' "$(date '+%Y-%m-%d %H:%M:%S')" "$message"
}

# abre (ou reabre) a sessão wscat em background e guarda os FDs no coproc WS
open_ws() {
  if [[ -n "${WS_PID:-}" ]] && kill -0 "$WS_PID" 2>/dev/null; then
    return 0
  fi

  # Fecha coproc anterior se ainda tiver descritores
  if [[ -n "${WS_1:-}" ]]; then
    exec {WS_1}>&-
  fi
  if [[ -n "${WS_0:-}" ]]; then
    exec {WS_0}<&-
  fi

  # Inicia wscat; --no-color evita códigos ANSI no buffer
  coproc WS { wscat -c "$WS_URL" --no-color 2>/dev/null; }
  WS_PID=$!
  # Guardar os descritores do coproc pra uso explícito
  WS_0=${WS[0]} # stdout do wscat (se quiser ler respostas)
  WS_1=${WS[1]} # stdin do wscat  (é aqui que a gente escreve)
  log "Sessão WebSocket aberta em $WS_URL (PID $WS_PID)"
  sleep 0.1
}

send_command() {
  local command="$1"
  open_ws
  # tenta enviar; se der EPIPE, reabre e tenta mais uma vez
  if ! printf '%s\n' "$command" >&"$WS_1" 2>/dev/null; then
    log "Conexão caiu. Reabrindo..."
    open_ws
    printf '%s\n' "$command" >&"$WS_1"
  fi
  log "Comando enviado: $command"
}

show_header() {
  cat <<MSG
============================================
 Controle remoto - Teste Autonomous Car V3
============================================
Host: $HOST  |  Porta: $PORT
Pressione W, A, S ou D para enviar comandos.
Pressione Q para sair.
MSG
}

cleanup() {
  stty echo || true
  printf '\n'
  # fecha descritores e encerra o wscat
  if [[ -n "${WS_1:-}" ]]; then exec {WS_1}>&-; fi
  if [[ -n "${WS_0:-}" ]]; then exec {WS_0}<&-; fi
  if [[ -n "${WS_PID:-}" ]] && kill -0 "$WS_PID" 2>/dev/null; then
    kill "$WS_PID" 2>/dev/null || true
    wait "$WS_PID" 2>/dev/null || true
  fi
  log "Programa finalizado."
}

trap cleanup EXIT INT TERM

show_header
stty -echo

open_ws

while true; do
  if read -rsn1 -t "$INTERVAL" key; then
    key_lower=${key,,}
    case "$key_lower" in
      w) send_command "forward" ;;
      a) send_command "left" ;;
      s) send_command "backward" ;;
      d) send_command "right" ;;
      q) exit 0 ;;
      *) ;; # ignora outras teclas
    esac
  fi
done
