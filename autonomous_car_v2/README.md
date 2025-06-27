# Autonomous Car v2

Esta pasta contém a segunda versão simplificada do projeto do carro autônomo. Além do servidor WebSocket, esta versão já possui captura de vídeo para transmissão aos clientes e controle dos motores via GPIO.

## Como compilar

```bash
./build.sh
```

## Como executar

```bash
./start.sh
```

O servidor WebSocket será iniciado na porta **8080**. A captura de vídeo utiliza a câmera de índice **4**, correspondente ao módulo conectado no *HaspBarrier*.

## Pinagem GPIO

As portas abaixo são usadas pelo `pigpio` para controlar os motores e o servo de direção:

| Pino GPIO | Função | Comando |
|-----------|---------------------------------------|----------------|
| `17`      | Motor A1 – avançar | `accelerate` (velocidade > 0) |
| `27`      | Motor A2 – ré | `accelerate` (velocidade < 0) |
| `23`      | Motor B1 – avançar | `accelerate` (velocidade > 0) |
| `22`      | Motor B2 – ré | `accelerate` (velocidade < 0) |
| `18`      | Servo de direção | `turn` |

Esses valores correspondem ao código fonte em `include/commands/handlers`. Ajuste conforme a sua fiação.
