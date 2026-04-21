# Traffic Sign Service

Servico C++ separado do `autonomous_car_v3`, executado no Ubuntu, para processar placas de transito localmente com Edge Impulse, exibir um dashboard de debug em tempo real e, quando estiver em modo `websocket`, devolver tanto `signal:detected=<signal_id>` quanto `telemetry.traffic_sign_detection`.

## Responsabilidade

- rodar no PC Ubuntu em vez do Raspberry
- receber frames de `image`, `video`, `camera` ou `websocket`
- aplicar ROI de sinalizacao e executar a inferencia
- manter estado temporal `idle/candidate/confirmed/error`
- exibir dashboard com frame completo, ROI, input do modelo, FPS e tempo de inferencia
- publicar `signal:detected` e telemetria JSON apenas no modo `websocket`

Este servico nao controla motor, servo, PID ou modo de conducao.

## Bootstrap Ubuntu

```bash
./bootstrap_ubuntu.sh
```

Pacotes instalados:

- `cmake`
- `libopencv-dev`
- `libwebsocketpp-dev`
- `libasio-dev`
- `nlohmann-json3-dev`
- `unzip`

## Configuracao

Arquivo padrao: [`config/service.env`](/home/omodei/Documentos/projects/carro-autonomo/traffic_sign_service/config/service.env)

Chaves principais:

- `FRAME_SOURCE_MODE=image|video|camera|websocket`
- `FRAME_SOURCE_PATH`
- `CAMERA_INDEX`
- `VEHICLE_WS_URL`
- `EDGE_IMPULSE_BACKEND=eim|embedded_cpp`
- `EDGE_IMPULSE_EIM_PATH`
- `EDGE_IMPULSE_MODEL_ZIP`
- `EIM_PRINT_INFO_ON_START`
- `TRAFFIC_SIGN_PREPROCESS_GRAYSCALE`
- `TRAFFIC_SIGN_ROI_LEFT_RATIO`
- `TRAFFIC_SIGN_ROI_RIGHT_RATIO`
- `TRAFFIC_SIGN_ROI_TOP_RATIO`
- `TRAFFIC_SIGN_ROI_BOTTOM_RATIO`
- `TRAFFIC_SIGN_DEBUG_ROI_ENABLED`
- `TRAFFIC_SIGN_MIN_CONFIDENCE`
- `TRAFFIC_SIGN_MIN_CONSECUTIVE_FRAMES`
- `TRAFFIC_SIGN_MAX_MISSED_FRAMES`
- `TRAFFIC_SIGN_MAX_RAW_DETECTIONS`
- `DETECTION_COOLDOWN_MS`
- `INFERENCE_MAX_FPS`
- `FRAME_PREVIEW_ENABLED`

Notas:

- o backend padrao no codigo e `eim`
- o `service.env` agora aponta por padrao para o fluxo Linux oficial: `EDGE_IMPULSE_BACKEND=eim`
- `embedded_cpp` continua como fallback e compila direto o zip configurado, mas esse caminho segue CPU-only dentro deste servico
- o zip Linux atual e `tcc-pare-direita-esquerda-cam-raspberry-cpp-linux-v20-impulse.zip`
- `start.sh` gera automaticamente um `.eim` local quando `EDGE_IMPULSE_BACKEND=eim` e o binario ainda nao existe
- `TRAFFIC_SIGN_PREPROCESS_GRAYSCALE=false` preserva cor para modelos treinados em RGB; `true` força tons de cinza antes do empacotamento RGB888
- o metadata do pacote `cpp-linux-v20` atual reporta `image_channel_count=1`, entao o backend `eim` forca grayscale automaticamente para esse modelo
- o encoder de imagem agora segue o fluxo oficial do Edge Impulse: resize no modo do Studio e pixels empacotados em RGB888
- o OpenCV instalado aqui nao expõe modulos CUDA; a aceleracao nao depende de OpenCV CUDA
- segundo a documentacao oficial do Edge Impulse para Linux, `x86_64` usa `USE_FULL_TFLITE=1`; o caminho GPU/TensorRT documentado fica para Jetson

## Build

```bash
./build.sh
```

`build.sh` continua extraindo o zip configurado do Edge Impulse para `build/generated/edge_impulse_model` e compila o binario `traffic_sign_service`. Esse zip e usado pelo backend `embedded_cpp`.

## Gerando o `.eim` Linux

```bash
./build_linux_eim.sh
```

Esse script:

- usa o zip configurado em `EDGE_IMPULSE_MODEL_ZIP`
- clona localmente o repositório oficial `example-standalone-inferencing-linux` do Edge Impulse em `build/edge_impulse_linux_sdk`
- extrai o modelo `cpp-linux-v20`
- gera `build/edge_impulse_linux_sdk/build/model.eim`

No Ubuntu `x86_64`, a geracao e feita com o caminho oficial `USE_FULL_TFLITE=1`. Na validacao local deste repositório o artefato gerado nao linkou `CUDA` nem `TensorRT`, entao ele melhora o alinhamento com o deploy Linux oficial, mas nao transforma a GTX 1050 Ti em backend de inferencia automaticamente.

## Execucao

Modo video local:

```bash
./start.sh
```

Modo camera:

```bash
FRAME_SOURCE_MODE=camera CAMERA_INDEX=0 ./start.sh
```

Modo imagem:

```bash
FRAME_SOURCE_MODE=image FRAME_SOURCE_PATH=/abs/caminho/placa.jpg ./start.sh
```

Modo websocket:

```bash
FRAME_SOURCE_MODE=websocket VEHICLE_WS_URL=ws://IP_DO_RASPBERRY:8080 ./start.sh
```

Atalhos da janela:

- `q` ou `ESC`: encerra a visualizacao

## Telemetria

No modo `websocket`, o servico publica:

- `signal:detected=<signal_id>`
- `telemetry.traffic_sign_detection`

O JSON de `telemetry.traffic_sign_detection` reaproveita os nomes de campo do V3:

- `roi`
- `raw_detections`
- `candidate`
- `active_detection`
- `last_error`

Labels desconhecidas ficam em `raw_detections`, mas nao geram `signal:detected`.

## Testes

```bash
cd build
ctest --output-on-failure
```

Cobertura implementada:

- parsing de `vision.frame`
- validacao de `service.env`
- ROI e remapeamento de bounding box
- filtro temporal `candidate/confirmed`
- cooldown de eventos `signal:detected`
- bridge EIM com `--print-info`, `hello` e `classify`
- telemetria JSON detalhada
- fonte WebSocket com handshake `client:telemetry` + `stream:subscribe=raw`
- integracao do servico com fonte fake, publisher fake e loop central multi-fonte
