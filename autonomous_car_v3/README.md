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

- `command:<origem>:<acao>` – controla o carro e informa a origem do comando. Exemplos:
  - `command:manual:forward`
  - `command:manual:left`
  - `command:manual:center`
  - `command:manual:throttle=75` (valor normalizado: `-100` a `100` ou `-1.0` a `1.0`)
  - `command:autonomous:steering=-0.4`
- `config:<chave>=<valor>` – ajusta parâmetros em tempo de execução (por exemplo `config:motor.command_timeout_ms=200`).

Se a origem não for informada o servidor assume que o comando veio da pilotagem manual. Quando o carro estiver no modo autônomo, comandos manuais são ignorados e vice-versa – isso evita que origens conflitantes disputem o controle.

Comandos discretos de direção (`left`/`right`) deslocam o alvo apenas um incremento por acionamento (`STEERING_COMMAND_STEP`).
Pressione continuamente para acumular o giro até atingir o limite desejado.

Os comandos podem ser disparados rapidamente em sequência. Os motores funcionam de forma binária (apenas frente, ré ou parado) e interrompem o movimento automaticamente quando novas ordens deixam de chegar por mais tempo que o limite configurado.

### Modos de condução e direção

O veículo pode operar em dois modos:

- **Manual (padrão):** toda a direção é aplicada diretamente conforme os comandos recebidos, utilizando apenas o fator de sensibilidade configurado para limitar o ângulo máximo. O PID permanece carregado, mas fica em _stand-by_.
- **Autonomous:** os comandos são aceitos apenas quando enviados com a origem `autonomous`. Nesse modo o PID de direção volta a atuar para suavizar e corrigir a trajetória com base nos alvos enviados.

O modo ativo pode ser definido no arquivo `.env` e alterado em tempo de execução pelo canal `config` (`config:driving.mode=manual`).

### Parâmetros de configuração

Os arquivos `.env` (e o canal `config`) aceitam os parâmetros abaixo para calibrar a dinâmica do carro:

| Chave | Descrição |
|-------|-----------|
| `MOTOR_COMMAND_TIMEOUT_MS` | Tempo máximo (ms) sem novos comandos antes de parar os motores |
| `STEERING_PID_KP`, `STEERING_PID_KI`, `STEERING_PID_KD` | Ganhos do PID de direção |
| `STEERING_PID_OUTPUT_LIMIT` | Velocidade máxima de variação do ângulo |
| `STEERING_PID_INTERVAL_MS` | Intervalo de atualização do PID de direção |
| `STEERING_COMMAND_STEP` | Incremento aplicado a cada comando `left`/`right` (0–1) |
| `STEERING_CENTER_ANGLE` | Define a posição neutra (em graus) aplicada no boot |
| `STEERING_LEFT_LIMIT_DEGREES`, `STEERING_RIGHT_LIMIT_DEGREES` | Limite (em graus) para cada lado em relação ao centro |
| `MOTOR_LEFT_INVERTED`, `MOTOR_RIGHT_INVERTED` | Ajustam a polaridade física de cada motor |
| `DRIVING_MODE` | Define se o carro inicia em `manual` ou `autonomous` |

Os valores padrão vivem em `config/autonomous_car.env` (e em um arquivo `.env` de conveniência na raiz do projeto). Copie-os para outro local ou edite-os diretamente conforme a sua necessidade. Valores podem ser ajustados em tempo real via WebSocket para facilitar a calibração em pista.

> **Dica:** os limites de direção vêm configurados para manter o servo entre 70° e 110° (centro em 90°). Ajuste `STEERING_LEFT_LIMIT_DEGREES` e `STEERING_RIGHT_LIMIT_DEGREES` caso a montagem permita um curso diferente em cada lado.

## Próximos passos sugeridos

- Integrar sensores de feedback (encoders, IMU) para fechar o loop de velocidade real
- Adicionar monitoramento de telemetria via WebSocket/broker MQTT
- Criar interface web com sliders para ajuste dinâmico dos ganhos PID
