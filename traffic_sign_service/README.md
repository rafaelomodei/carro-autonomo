# Traffic Sign Service

Servico C++ separado do `autonomous_car_v3`, executado no Ubuntu, para consumir `vision.frame` em `raw`, rodar o modelo atual do Edge Impulse e devolver apenas eventos leves `signal:detected=<signal_id>`.

## Responsabilidade

- consumir o stream WebSocket ja exposto pelo Raspberry
- processar apenas `vision.frame` com `view=raw`
- executar inferencia com o export atual do Edge Impulse
- aplicar debounce por `3` frames, confianca minima `0.60` e cooldown de `2000 ms`
- reenviar somente `signal:detected=stop|turn_left`

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

Chaves publicas:

- `VEHICLE_WS_URL`
- `EDGE_IMPULSE_MODEL_ZIP`
- `DETECTION_MIN_CONFIDENCE`
- `DETECTION_CONFIRMATION_FRAMES`
- `DETECTION_COOLDOWN_MS`
- `INFERENCE_MAX_FPS`
- `FRAME_PREVIEW_ENABLED`

Se `FRAME_PREVIEW_ENABLED=true`, o servico abre uma janela simples com o frame `raw` que esta sendo recebido. Quando `false`, nenhuma janela e aberta.

## Build e Execucao

```bash
./build.sh
./start.sh
```

`build.sh` extrai o zip configurado do Edge Impulse para `build/generated/edge_impulse_model` e compila o binario `traffic_sign_service`.

## Testes

```bash
cd build
ctest --output-on-failure
```

Cobertura implementada:

- parsing de `vision.frame`
- validacao de base64 e views
- mapeamento de labels do modelo atual
- fila `latest-frame-wins`
- debounce/cooldown
- integracao do servico com transporte fake, frames conhecidos e reenvio de `signal:detected`
