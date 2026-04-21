# Autonomous Car v3

Versao focada em controle do veiculo no Raspberry Pi com um pipeline compartilhado de segmentacao de estrada, debug local e telemetria via WebSocket.

Documentacao operacional atual: [`docs/runtime_operation.md`](./docs/runtime_operation.md)

## O que mudou

- O pipeline de segmentacao agora vem do core compartilhado em `shared/road_segmentation`.
- O modo autonomo agora usa PID lateral em cima das referencias `near/mid/far` da segmentacao.
- A visao foi separada em dois arquivos:
  - `config/vision.env`: origem de captura, janela de debug e taxa maxima de telemetria.
  - `config/road_segmentation.env`: parametros `LANE_*` do pipeline.
- O servidor WebSocket suporta multiplos clientes, com no maximo um controlador ativo e qualquer numero de consumidores de telemetria.
- O mesmo WebSocket publica `telemetry.road_segmentation` e `telemetry.autonomous_control`.

## Perfis de execucao

### 1. Hardware completo

Usa `wiringPi`, GPIO, segmentacao ao vivo e aplicacao real do comando autonomo nos motores/servo.

```bash
./build.sh
./start.sh
```

Se `wiringPi` nao estiver instalado, esse binario nao sera gerado.
O script usa `Ninja` por padrao e compila em paralelo com todos os cores disponiveis do Raspberry Pi.

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

O `build.sh` reconfigura o projeto com `Ninja` por padrao e chama o build com paralelismo automatico.
Se a pasta `build/` ainda estiver com cache antigo de `Unix Makefiles`, ela e recriada automaticamente para migrar o gerador.

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

Detalhes do controlador e do painel local em `docs/pid_control.md`.

### `config/vision.env`

- `VISION_SOURCE_MODE=camera|video|image`
- `VISION_SOURCE_PATH`
- `VISION_CAMERA_INDEX`
- `VISION_DEBUG_WINDOW_ENABLED`
- `VISION_TELEMETRY_MAX_FPS`
- `VISION_STREAM_MAX_FPS`
- `TRAFFIC_SIGN_TARGET_FPS`
- `VISION_STREAM_JPEG_QUALITY`
- `VISION_SEGMENTATION_CONFIG_PATH`
- `VISION_TRAFFIC_SIGN_CONFIG_PATH`

Defaults:

```env
VISION_SOURCE_MODE=camera
VISION_CAMERA_INDEX=0
VISION_DEBUG_WINDOW_ENABLED=true
TRAFFIC_SIGN_DEBUG_WINDOW_ENABLED=false
VISION_TELEMETRY_MAX_FPS=10
VISION_STREAM_MAX_FPS=5
TRAFFIC_SIGN_TARGET_FPS=4
VISION_STREAM_JPEG_QUALITY=70
VISION_SEGMENTATION_CONFIG_PATH=road_segmentation.env
VISION_TRAFFIC_SIGN_CONFIG_PATH=traffic_sign.env
```

### `config/traffic_sign.env`

- `TRAFFIC_SIGN_ENABLED`
- `TRAFFIC_SIGN_ROI_LEFT_RATIO`
- `TRAFFIC_SIGN_ROI_RIGHT_RATIO`
- `TRAFFIC_SIGN_ROI_TOP_RATIO`
- `TRAFFIC_SIGN_ROI_BOTTOM_RATIO`
- `TRAFFIC_SIGN_DEBUG_ROI_ENABLED`
- `TRAFFIC_SIGN_MIN_CONFIDENCE`
- `TRAFFIC_SIGN_MIN_CONSECUTIVE_FRAMES`
- `TRAFFIC_SIGN_MAX_MISSED_FRAMES`
- `TRAFFIC_SIGN_MAX_RAW_DETECTIONS`

Compatibilidade:

- `TRAFFIC_SIGN_ROI_RIGHT_WIDTH_RATIO` continua aceito como fallback quando `LEFT/RIGHT` nao forem definidos.

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

Quando `VISION_DEBUG_WINDOW_ENABLED=true`, o servico abre um dashboard expandido:

- painel 2x2 da segmentacao compartilhada
- ROI e bounding boxes de sinalizacao desenhadas nos tiles `Original` e `Saida anotada`
- card lateral de controle autonomo com estado, erro composto e termos `P/I/D`
- indicador visual do comando de direção
- mapa superior sintetico com referencias `near/mid/far` e trajetoria prevista

Quando `TRAFFIC_SIGN_DEBUG_WINDOW_ENABLED=true`, o servico abre uma segunda janela
dedicada ao debug da sinalizacao:

- tile esquerdo com o recorte bruto da ROI usado na inferencia
- tile direito com a imagem preprocessada/redimensionada enviada ao Edge Impulse
- status do detector, confianca, FPS, tempo de inferencia e idade do resultado
- labels carregadas do modelo compilado no binario
- lembrete explicito de que o zip em `edgeImpulse/` nao e usado em runtime

Leitura do card `Sinalizacao`:

- `Core FPS`: frequencia do loop principal de captura + segmentacao.
- `UI FPS`: frequencia dos frames realmente codificados e enviados para a interface.
- `Placa FPS`: frequencia das inferencias de sinalizacao.
- `Infer placa`: tempo medio da inferencia de placas em milissegundos.
- `UI enc`: tempo de codificacao do frame JPEG da interface em milissegundos.
- `Drop P/UI`: quantos jobs foram descartados na fila da sinalizacao e no stream quando o consumidor ficou mais lento que o produtor.
- `idade`: atraso do ultimo resultado de placa usado no overlay.

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
- `stream:subscribe=<csv_views>`
- `signal:detected=<signal_id>`

Compatibilidade:

- a primeira conexao que enviar `command:` ou `config:` sem registro explicito assume `control`
- conexoes extras que tentarem controlar o carro caem como `telemetry`

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
```

Semantica operacional:

- `config:driving.mode=manual|autonomous` troca o modo de conducao.
- No modo `manual`, apenas comandos `command:manual:*` sao aceitos.
- No modo `autonomous`, o carro fica parado ate receber `command:autonomous:start`.
- `command:autonomous:stop`, troca de modo, encerramento do servico ou perda de pista acima do timeout executam parada segura.
- `signal:detected=<signal_id>` aceita `stop`, `turn_left` e `turn_right`, registra o ultimo evento recebido e nao aciona movimento.

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

```json
{
  "type": "telemetry.autonomous_control",
  "timestamp_ms": 1710000000000,
  "driving_mode": "autonomous",
  "autonomous_started": true,
  "tracking_state": "tracking",
  "stop_reason": "none",
  "fail_safe_active": false,
  "lane_available": true,
  "confidence_ok": true,
  "confidence_score": 0.87,
  "preview_error": 0.08,
  "steering_command": 0.06,
  "motion_command": "forward",
  "pid": {
    "error": 0.08,
    "p": 0.04,
    "i": 0.01,
    "d": 0.01,
    "raw_output": 0.06,
    "output": 0.06
  }
}
```

```json
{
  "type": "telemetry.traffic_sign_detection",
  "timestamp_ms": 1710000000000,
  "source": "Camera index 0",
  "detector_state": "confirmed",
  "roi": {
    "left_ratio": 0.55,
    "right_ratio": 1.0,
    "top_ratio": 0.08,
    "bottom_ratio": 0.72,
    "right_width_ratio": 0.45
  },
  "raw_detections": []
}
```

Frames de visao publicados sob demanda:

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

- `stream:subscribe=` com lista vazia desliga o envio de frames para aquela sessao.
- o servidor mantem a assinatura por sessao, sem afetar outros clientes.
- o `RoadSegmentationService` so renderiza/serializa as views atualmente pedidas.
- o stream usa o mesmo WebSocket textual existente; nao ha endpoint HTTP/MJPEG separado.

## Build e testes

```bash
cmake -S . -B build
cmake --build build --parallel
cd build
ctest --output-on-failure
```

No ambiente sem `wiringPi`, os testes e o binario `autonomous_car_v3_vision_debug` continuam disponiveis.

## Limitacoes desta etapa

- o controle autonomo desta fase atua apenas na direção lateral; o movimento continua como frente/parado
- o `vision_debug` nao aciona GPIO; ele apenas calcula PID, painel local e telemetria
- o stream de visao usa JPEG em base64; esta fase prioriza simplicidade de integracao sobre eficiencia binaria maxima
