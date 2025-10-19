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
| `17`      | Motor esquerdo – sentido frente  |
| `27`      | Motor esquerdo – sentido ré      |
| `23`      | Motor direito – sentido frente   |
| `22`      | Motor direito – sentido ré       |
| `18`      | Servo da direção (PWM)           |

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

O servidor WebSocket será iniciado em `ws://0.0.0.0:8080`. Envie uma mensagem de texto com um dos comandos abaixo para controlar o carro:

- `forward`
- `backward`
- `left`
- `right`
- `center`
- `stop`

## Próximos passos sugeridos

- Implementar controle de velocidade variável e frenagem suave
- Adicionar um controlador PID para correção fina da direção
- Integrar com serviços de visão computacional e sensores adicionais
