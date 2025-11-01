#!/usr/bin/env bash
set -euo pipefail

HOST=${HOST:-192.168.15.164}
PORT=${PORT:-8080}
INTERVAL=${INTERVAL:-0.1}

log() {
  local message="$1"
  printf '[%s] %s\n' "$(date '+%Y-%m-%d %H:%M:%S')" "$message"
}

send_command() {
  local command="$1"
  local payload="{\"command\":\"$command\"}"

  if exec 3>/dev/tcp/"$HOST"/"$PORT"; then
    printf '%s\n' "$payload" >&3
    exec 3>&-
    log "Comando enviado: $command"
  else
    log "Não foi possível conectar a $HOST:$PORT"
  fi
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
  stty echo
  printf '\n'
  log "Programa finalizado."
}

trap cleanup EXIT

show_header
stty -echo

while true; do
  if read -rsn1 -t "$INTERVAL" key; then
    key_lower=${key,,}
    case "$key_lower" in
      w)
        send_command "forward"
        ;;
      a)
        send_command "left"
        ;;
      s)
        send_command "backward"
        ;;
      d)
        send_command "right"
        ;;
      q)
        exit 0
        ;;
      *)
        ;; # ignora outras teclas
    esac
  fi

done
