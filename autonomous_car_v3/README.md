# Autonomous Car v3

Versao focada em controle do veiculo no Raspberry Pi com um pipeline compartilhado de segmentacao de estrada, debug local e telemetria via WebSocket.

## O que mudou

- O pipeline de segmentacao agora vem do core compartilhado em `shared/road_segmentation`.
- A visao foi separada em dois arquivos:
  - `config/vision.env`: origem de captura, janela de debug e taxa maxima de telemetria.
  - `config/road_segmentation.env`: parametros `LANE_*` do pipeline.
- O servidor WebSocket suporta multiplos clientes, com no maximo um controlador ativo e qualquer numero de consumidores de telemetria.
- A telemetria de segmentacao sai como JSON no mesmo WebSocket com `type: "telemetry.road_segmentation"`.

## Perfis de execucao

### 1. Hardware completo

Usa `wiringPi`, GPIO e segmentacao ao vivo.

```bash
./build.sh
./start.sh
```

Se `wiringPi` nao estiver instalado, esse binario nao sera gerado.

### 2. Visao/debug local

Executa WebSocket + segmentacao sem depender de `wiringPi`.

```bash
./build.sh
./start.sh --vision-debug
```

Se o binario de hardware nao existir, `./start.sh` faz fallback automatico para este perfil.

## Arquivos de configuracao

### `config/autonomous_car.env`

Mantem apenas configuracoes de hardware, direcao e modo de conducao:

- `MOTOR_COMMAND_TIMEOUT_MS`
- `STEERING_SENSITIVITY`
- `STEERING_COMMAND_STEP`
- `STEERING_CENTER_ANGLE`
- `STEERING_LEFT_LIMIT_DEGREES`
- `STEERING_RIGHT_LIMIT_DEGREES`
- `MOTOR_LEFT_INVERTED`
- `MOTOR_RIGHT_INVERTED`
- `DRIVING_MODE`

### `config/vision.env`

- `VISION_SOURCE_MODE=camera|video|image`
- `VISION_SOURCE_PATH`
- `VISION_CAMERA_INDEX`
- `VISION_DEBUG_WINDOW_ENABLED`
- `VISION_TELEMETRY_MAX_FPS`
- `VISION_SEGMENTATION_CONFIG_PATH`

Defaults:

```env
VISION_SOURCE_MODE=camera
VISION_CAMERA_INDEX=0
VISION_DEBUG_WINDOW_ENABLED=true
VISION_TELEMETRY_MAX_FPS=10
VISION_SEGMENTATION_CONFIG_PATH=road_segmentation.env
```

### `config/road_segmentation.env`

Contem os mesmos `LANE_*` ja validados no laboratorio, sem renomear o contrato.

## Fontes de captura

O servico de visao aceita:

- camera do Raspberry (`VISION_SOURCE_MODE=camera`)
- video local (`VISION_SOURCE_MODE=video`)
- imagem local (`VISION_SOURCE_MODE=image`)

Quando o modo nao for `camera`, `VISION_SOURCE_PATH` pode ser relativo ao diretorio de `vision.env`.
Se a fonte local falhar, o servico tenta fallback para a camera configurada.

## Debug local

Quando `VISION_DEBUG_WINDOW_ENABLED=true`, o servico abre um dashboard com o renderer do laboratorio.

Atalhos:

- `q` ou `ESC`: encerra o servico de visao
- `p`: pausa/retoma video ou camera
- `n`: avanca um frame quando pausado
- `r`: recarrega `vision.env` e `road_segmentation.env`

## WebSocket

Servidor padrao:

```text
ws://0.0.0.0:8080
```

Mensagens de entrada:

- `client:control`
- `client:telemetry`
- `command:<origem>:<acao>`
- `config:<chave>=<valor>`

Compatibilidade:

- a primeira conexao que enviar `command:` ou `config:` sem registro explicito assume `control`
- conexoes extras que tentarem controlar o carro caem como `telemetry`

Exemplos:

```text
client:control
command:manual:forward
command:manual:steering=0.25
config:driving.mode=autonomous
```

Telemetria publicada:

```json
{
  "type": "telemetry.road_segmentation",
  "timestamp_ms": 1710000000000,
  "source": "Camera index 0",
  "lane_found": true,
  "confidence_score": 0.87,
  "lane_center_ratio": 0.52,
  "steering_error_normalized": 0.11,
  "lateral_offset_px": 14.5,
  "heading_error_rad": 0.23,
  "heading_valid": true,
  "curvature_indicator_rad": -0.07,
  "curvature_valid": true,
  "references": {
    "near": {
      "valid": true,
      "point_x": 160,
      "point_y": 200,
      "top_y": 150,
      "bottom_y": 220,
      "center_ratio": 0.55,
      "lateral_offset_px": 18.0,
      "steering_error_normalized": 0.14,
      "sample_count": 12
    }
  }
}
```

## Build e testes

```bash
cmake -S . -B build
cmake --build build
cd build
ctest --output-on-failure
```

No ambiente sem `wiringPi`, os testes e o binario `autonomous_car_v3_vision_debug` continuam disponiveis.

## Limitacoes desta etapa

- a segmentacao apenas observa e publica telemetria
- nao ha resposta automatica de direcao nem integracao com PID
- o frame de debug nao e enviado pelo WebSocket; apenas metricas e estado
