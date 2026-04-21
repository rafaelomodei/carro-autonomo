# Autonomous Car v3

Versao do runtime que roda no Raspberry Pi e concentra:

- captura de video
- segmentacao de estrada
- controle autonomo lateral
- stream de visao via WebSocket
- telemetria do veiculo
- relay de eventos e telemetria de placas vindos de um servico externo

Documentacao operacional detalhada: [`docs/runtime_operation.md`](./docs/runtime_operation.md)

## O que esta rodando no Raspberry

O `autonomous_car_v3` nao executa mais Edge Impulse nem inferencia local de placas.

Hoje o Raspberry fica responsavel por:

- produzir `vision.frame` para clientes assinantes
- publicar `telemetry.road_segmentation`
- publicar `telemetry.autonomous_control`
- publicar `telemetry.vision_runtime`
- receber `signal:detected=<signal_id>` de um servico externo
- receber JSON `telemetry.traffic_sign_detection` de um servico externo e redistribuir o payload bruto para os demais clientes

Em outras palavras: a deteccao de placas ficou 100% no `traffic_sign_service`, enquanto o V3 virou a fonte de stream/telemetria do carro e o ponto de relay da comunicacao WebSocket.

## Perfis de execucao

### 1. Hardware completo

Usa `wiringPi`, GPIO, segmentacao ao vivo e aplicacao real do comando autonomo nos motores/servo.

```bash
./build.sh
./start.sh
```

Se `wiringPi` nao estiver instalado, esse binario nao sera gerado.

### 2. Visao/debug local

Executa WebSocket + segmentacao + PID/debug sem depender de `wiringPi`.

```bash
./build.sh
./start.sh --vision-debug
```

Se o binario de hardware nao existir, `./start.sh` faz fallback automatico para este perfil.

## Build

Dependencia recomendada no Raspberry Pi:

```bash
sudo apt install ninja-build
```

O `build.sh` reconfigura o projeto com `Ninja` por padrao e compila em paralelo com todos os cores disponiveis do Raspberry Pi.

Exemplos:

```bash
./build.sh
BUILD_JOBS=4 ./build.sh
BUILD_GENERATOR="Unix Makefiles" ./build.sh
```

## Arquivos de configuracao

### `config/autonomous_car.env`

Mantem configuracoes de hardware, direcao, modo de conducao e tuning do controlador autonomo:

- `MOTOR_COMMAND_TIMEOUT_MS`
- `STEERING_SENSITIVITY`
- `STEERING_COMMAND_STEP`
- `STEERING_CENTER_ANGLE`
- `STEERING_LEFT_LIMIT_DEGREES`
- `STEERING_RIGHT_LIMIT_DEGREES`
- `MOTOR_LEFT_INVERTED`
- `MOTOR_RIGHT_INVERTED`
- `DRIVING_MODE`
- `AUTONOMOUS_PID_KP`
- `AUTONOMOUS_PID_KI`
- `AUTONOMOUS_PID_KD`
- `AUTONOMOUS_PID_OUTPUT_LIMIT`
- `AUTONOMOUS_PREVIEW_NEAR_WEIGHT`
- `AUTONOMOUS_PREVIEW_MID_WEIGHT`
- `AUTONOMOUS_PREVIEW_FAR_WEIGHT`
- `AUTONOMOUS_MAX_STEERING_DELTA_PER_UPDATE`
- `AUTONOMOUS_MIN_CONFIDENCE`
- `AUTONOMOUS_LANE_LOSS_TIMEOUT_MS`

Detalhes do controlador: [`docs/pid_control.md`](./docs/pid_control.md)

### `config/vision.env`

Controla apenas a origem de captura, o debug local e os limites de publicacao:

- `VISION_SOURCE_MODE=camera|video|image`
- `VISION_SOURCE_PATH`
- `VISION_CAMERA_INDEX`
- `VISION_DEBUG_WINDOW_ENABLED`
- `VISION_TELEMETRY_MAX_FPS`
- `VISION_STREAM_MAX_FPS`
- `VISION_STREAM_JPEG_QUALITY`
- `VISION_SEGMENTATION_CONFIG_PATH`

Defaults:

```env
VISION_SOURCE_MODE=camera
VISION_SOURCE_PATH=
VISION_CAMERA_INDEX=0
VISION_DEBUG_WINDOW_ENABLED=true
VISION_TELEMETRY_MAX_FPS=10
VISION_STREAM_MAX_FPS=5
VISION_STREAM_JPEG_QUALITY=70
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

Quando `VISION_DEBUG_WINDOW_ENABLED=true`, o servico abre uma janela unica com:

- painel 2x2 da segmentacao compartilhada
- card lateral de controle autonomo
- indicador visual do comando de direção
- mapa superior sintetico com referencias `near/mid/far` e trajetoria prevista

Atalhos:

- `q` ou `ESC`: encerra o servico de visao
- `p`: pausa/retoma video ou camera
- `n`: avanca um frame quando pausado

Nao existe mais janela local dedicada de placas, ROI de placas nem bounding boxes desenhadas no Raspberry.

## WebSocket

Servidor padrao:

```text
ws://0.0.0.0:8080
```

### Mensagens de entrada

- `client:control`
- `client:telemetry`
- `command:<origem>:<acao>`
- `config:<chave>=<valor>`
- `stream:subscribe=<csv_views>`
- `signal:detected=<signal_id>`
- payload JSON `telemetry.traffic_sign_detection`

Compatibilidade:

- a primeira conexao que enviar `command:` ou `config:` sem registro explicito assume `control`
- conexoes extras que tentarem controlar o carro caem como `telemetry`
- `signal:detected=<signal_id>` continua sendo o canal canonico para registrar o ultimo evento de placa recebido pelo Raspberry
- `telemetry.traffic_sign_detection` e aceito como payload bruto e redistribuido sem reserializacao

Exemplos:

```text
client:control
command:manual:forward
command:manual:steering=0.25
command:autonomous:start
command:autonomous:stop
config:driving.mode=autonomous
stream:subscribe=raw,mask,annotated
signal:detected=stop
{"type":"telemetry.traffic_sign_detection","timestamp_ms":1710000000000,...}
```

### Telemetria publicada

Saidas mantidas pelo V3:

- `telemetry.road_segmentation`
- `telemetry.autonomous_control`
- `telemetry.vision_runtime`
- `vision.frame`
- `telemetry.traffic_sign_detection` apenas quando recebida de um servico externo e relayada pelo servidor

Exemplo de `telemetry.vision_runtime`:

```json
{
  "type": "telemetry.vision_runtime",
  "timestamp_ms": 1710000000000,
  "source": "Camera index 0",
  "core_fps": 18.4,
  "stream_fps": 4.2,
  "traffic_sign_fps": 0.0,
  "traffic_sign_inference_ms": 0.0,
  "stream_encode_ms": 28.3,
  "traffic_sign_dropped_frames": 0,
  "stream_dropped_frames": 8,
  "sign_result_age_ms": -1
}
```

Os campos `traffic_sign_*` permanecem no contrato por compatibilidade com o frontend, mas ficam neutros porque nao existe mais inferencia local de placas no Raspberry.

### Stream de visao

Frames de visao seguem sendo publicados sob demanda:

```json
{
  "type": "vision.frame",
  "view": "dashboard",
  "timestamp_ms": 1710000000000,
  "mime": "image/jpeg",
  "width": 1280,
  "height": 720,
  "data": "<base64>"
}
```

Views suportadas:

- `raw`
- `preprocess`
- `mask`
- `annotated`
- `dashboard`

Semantica do stream:

- `stream:subscribe=` com lista vazia desliga o envio de frames para aquela sessao
- o servidor mantem a assinatura por sessao, sem afetar outros clientes
- o `RoadSegmentationService` so renderiza/serializa as views pedidas ou a janela local
- o `traffic_sign_service` continua usando esse stream para consumir `raw`

## Testes

```bash
cmake -S . -B build
cmake --build build --parallel
cd build
ctest --output-on-failure
```

No ambiente sem `wiringPi`, os testes e o binario `autonomous_car_v3_vision_debug` continuam disponiveis.
