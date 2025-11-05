# Autonomous Car v3

Esta é a terceira versão do projeto do carro autônomo, focada no controle dos motores utilizando a biblioteca [WiringPi](https://github.com/WiringPi/WiringPi) e em um servidor WebSocket leve para receber comandos remotos. O objetivo é permitir o controle básico de movimentação (frente, ré, esquerda, direita e parada), além do ajuste da direção via servo motor.

## Tecnologias utilizadas

- **C++17**
- **WiringPi** para controle das GPIOs
- Servidor WebSocket próprio baseado na especificação RFC 6455
- Padrão de projeto **Command** para desacoplar os comandos das ações sobre o hardware

## Estrutura de pastas

```
autonomous_car_v3/
├── src/
│   ├── commands/          # comandos do padrão Command (headers e fontes)
│   ├── controllers/       # controladores de motor e direção
│   ├── services/          # servidor WebSocket
│   └── main.cpp
├── build.sh
├── start.sh
└── CMakeLists.txt
```

## GPIO utilizadas

| Pino GPIO | Função                           |
|-----------|----------------------------------|
| `26`      | Motor esquerdo – sentido frente  |
| `20`      | Motor esquerdo – sentido ré      |
| `19`      | Motor direito – sentido frente   |
| `16`      | Motor direito – sentido ré       |
| `13`      | Servo da direção (PWM)           |

> ⚠️ Os pinos utilizam a numeração BCM por meio de `wiringPiSetupGpio()`. Ajuste-os conforme o seu hardware.

## Dependências

- `cmake`
- `g++` (compatível com C++17)
- Biblioteca `wiringPi`

## Como compilar

```bash
./build.sh
```

O script cria uma pasta `build/`, executa o `cmake` e, em seguida, compila o projeto.

## Como executar

```bash
./start.sh
```

O servidor WebSocket será iniciado em `ws://0.0.0.0:8080`. Envie mensagens de texto para os canais abaixo:

- `command:<acao>` – controla o carro. Exemplos:
  - `command:forward`
  - `command:backward`
  - `command:stop`
  - `command:left`
  - `command:right`
  - `command:center`
  - `command:throttle=75` (valor normalizado: `-100` a `100` ou `-1.0` a `1.0`)
  - `command:steering=-35` (valor normalizado: `-100` a `100` ou `-1.0` a `1.0`)
- `config:<chave>=<valor>` – ajusta parâmetros em tempo de execução (por exemplo `config:motor.pid.kp=3.1`).

Os comandos podem ser disparados rapidamente em sequência; o controlador PID interno cuida da suavização das transições, evitando travamentos ao alternar entre frente/ré ou esquerda/direita.

### Ajuste fino via PID

Os arquivos `.env` (e o canal `config`) aceitam os parâmetros abaixo para calibrar a dinâmica do carro:

| Chave | Descrição |
|-------|-----------|
| `MOTOR_PID_KP`, `MOTOR_PID_KI`, `MOTOR_PID_KD` | Ganhos do PID do motor (aceleração/frenagem) |
| `MOTOR_PID_OUTPUT_LIMIT` | Delta máximo aplicado a cada iteração para limitar a rampa |
| `MOTOR_PID_INTERVAL_MS` | Intervalo de atualização do PID do motor em ms |
| `MOTOR_MIN_ACTIVE_THROTTLE` | Fator mínimo (0–1) necessário para acionar o H-bridge sem travar |
| `STEERING_PID_KP`, `STEERING_PID_KI`, `STEERING_PID_KD` | Ganhos do PID de direção |
| `STEERING_PID_OUTPUT_LIMIT` | Velocidade máxima de variação do ângulo |
| `STEERING_PID_INTERVAL_MS` | Intervalo de atualização do PID de direção |
| `STEERING_CENTER_ANGLE` | Define a posição neutra (em graus) aplicada no boot |
| `STEERING_LEFT_LIMIT_DEGREES`, `STEERING_RIGHT_LIMIT_DEGREES` | Limite (em graus) para cada lado em relação ao centro |
| `MOTOR_LEFT_INVERTED`, `MOTOR_RIGHT_INVERTED` | Ajustam a polaridade física de cada motor |

Os valores padrão vivem em `config/autonomous_car.env` (e em um arquivo `.env` de conveniência na raiz do projeto). Copie-os para outro local ou edite-os diretamente conforme a sua necessidade. Valores podem ser ajustados em tempo real via WebSocket para facilitar a calibração em pista.

> **Dica:** os limites de direção vêm configurados para manter o servo entre 70° e 110° (centro em 90°). Ajuste `STEERING_LEFT_LIMIT_DEGREES` e `STEERING_RIGHT_LIMIT_DEGREES` caso a montagem permita um curso diferente em cada lado.

## Próximos passos sugeridos

- Integrar sensores de feedback (encoders, IMU) para fechar o loop de velocidade real
- Adicionar monitoramento de telemetria via WebSocket/broker MQTT
- Criar interface web com sliders para ajuste dinâmico dos ganhos PID
