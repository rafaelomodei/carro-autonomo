# Operacao do `autonomous_car_v3`

Este documento descreve como a aplicacao opera hoje no Raspberry Pi, tomando o `frontend` e outros clientes WebSocket como interfaces externas. O foco aqui e runtime, concorrencia, fluxo de dados, contratos publicos e comportamento operacional observado no codigo atual.

Base de leitura principal:

- `src/main.cpp`
- `src/vision_debug_main.cpp`
- `src/services/RoadSegmentationService.cpp`
- `src/services/WebSocketServer.cpp`
- `src/services/autonomous_control/AutonomousControlService.cpp`

Fora de escopo:

- versoes antigas `autonomous_car/` e `autonomous_car_v2/`
- arquitetura interna do `frontend`
- planejamento futuro de produto

## Evolucao recente

Os dois ultimos commits alteraram de forma relevante a forma como o sistema opera:

- `1cf3d1d` integrou a deteccao de placas ao servico de visao. A partir daqui o runtime passou a recortar ROI, rodar inferencia Edge Impulse/FOMO, aplicar filtro temporal e publicar `telemetry.traffic_sign_detection`, alem de exibir o resultado no overlay de debug.
- `00909ec` adicionou `telemetry.vision_runtime`, introduziu `LatestValueMailbox` para placas e stream, organizou melhor o fan-out de telemetria/stream e deixou o desacoplamento entre `core`, placas e stream mais explicito. Na pratica, isso reduz o risco de um consumidor lento travar o pipeline principal.

## Visao geral operacional

O `autonomous_car_v3` roda como um processo C++ no Raspberry Pi com dois perfis de execucao:

- `autonomous_car_v3`: perfil de hardware completo, depende de `wiringPi`, registra comandos manuais e aplica comandos autonomos em motor e servo.
- `autonomous_car_v3_vision_debug`: perfil de debug local, nao depende de `wiringPi`, mantem WebSocket, segmentacao, overlays e telemetria, mas nao atua fisicamente no veiculo.

Os dois perfis compartilham o mesmo nucleo de visao:

- `RoadSegmentationService` captura frame, roda segmentacao, alimenta o controle autonomo, dispara a pipeline de placas e faz fan-out de stream/debug.
- `WebSocketServer` recebe comandos/configuracoes e distribui telemetrias/frames.
- `AutonomousControlService` calcula o estado autonomo, o preview error, o PID e o estado de fail-safe.

```mermaid
flowchart LR
    cam["Camera do Raspberry"]
    envs["Arquivos .env: autonomous_car.env, vision.env, road_segmentation.env, traffic_sign.env"]
    clients["Frontend e outros clientes WebSocket"]
    vehicle["Motor DC e servo"]

    subgraph pi[Raspberry Pi]
        app["Processo autonomous_car_v3"]
        ws["WebSocketServer"]
        vision["RoadSegmentationService"]
        control["AutonomousControlService"]
        hw["MotorController e SteeringController"]
    end

    envs --> app
    cam --> vision
    app --> ws
    app --> vision
    vision --> control
    vision --> ws
    control --> hw
    hw --> vehicle

    clients -. cliente control .-> ws
    clients -. cliente telemetry .-> ws
    clients -. comandos e configs .-> ws
    clients -. subscribe de stream .-> ws
    ws --> clients
```

## Bootstrap e shutdown

O bootstrap varia um pouco entre o binario de hardware e o binario de debug, mas a espinha dorsal e a mesma:

- carregar `autonomous_car.env`
- resolver `vision.env`
- instanciar `AutonomousControlService`
- subir `WebSocketServer`
- subir `RoadSegmentationService`
- aguardar `SIGINT` ou `SIGTERM`
- executar parada segura

No binario de hardware, o processo ainda:

- inicializa `wiringPi`
- cria `MotorController` e `SteeringController`
- registra comandos manuais no `CommandRouter`
- injeta um `control_sink` que aplica `forward/stop` e `setSteering`

No binario `vision_debug`, o controle autonomo existe apenas para telemetria e visualizacao; nao ha atuacao fisica.

```mermaid
sequenceDiagram
    autonumber
    participant user as Operador ou start.sh
    participant entry as main ou vision_debug_main
    participant gpio as wiringPi
    participant config as ConfigurationManager
    participant control as AutonomousControlService
    participant ws as WebSocketServer
    participant vision as RoadSegmentationService
    participant hw as MotorController e SteeringController

    user->>entry: iniciar processo
    alt binario de hardware
        entry->>gpio: wiringPiSetupGpio()
        gpio-->>entry: ok
    else binario vision_debug
        Note over entry: sem wiringPi e sem atuadores reais
    end

    entry->>config: loadDefaults()
    entry->>config: loadFromFile(config/autonomous_car.env)
    entry->>entry: resolveProjectPath(config/vision.env)
    entry->>control: updateConfig() e setDrivingMode()
    entry->>ws: criar servidor

    alt binario de hardware
        entry->>hw: criar controllers e registrar comandos manuais
        entry->>vision: criar com control_sink real
    else binario vision_debug
        entry->>vision: criar sem control_sink real
    end

    entry->>ws: start()
    entry->>vision: start()
    Note over entry,vision: loop principal aguarda SIGINT ou SIGTERM

    user-->>entry: SIGINT ou SIGTERM
    entry->>control: stopAutonomous(service_stop)
    entry->>vision: stop()
    entry->>ws: stop()

    alt binario de hardware
        entry->>hw: stop() e center()
    end

    entry-->>user: processo encerrado
```

## Concorrencia interna

O comportamento atual do runtime e melhor entendido como quatro workers coordenados:

- `core_thread`: prioridade funcional do pipeline, faz captura e segmentacao.
- `traffic_sign_thread`: consome jobs de ROI e roda a deteccao de placas.
- `stream_thread`: renderiza views e serializa `vision.frame` apenas quando existe assinatura.
- `telemetry_thread`: publica telemetrias agregadas e `telemetry.vision_runtime`.

Estruturas de sincronizacao relevantes:

- `LatestValueMailbox<TrafficSignJob>`: guarda apenas o ultimo job de placas.
- `LatestValueMailbox<shared_ptr<CoreFrameSnapshot>>`: guarda apenas o ultimo snapshot para stream/debug.
- `TelemetryTopicQueue`: mantem apenas o ultimo payload por topico de telemetria pendente.

Isso implementa backpressure por descarte controlado. Se a inferencia de placas ou o encode JPEG ficarem lentos, o `core_thread` continua processando frames mais novos sem acumular uma fila infinita.

```mermaid
flowchart LR
    source["FrameSource"]
    ws["WebSocketServer"]
    drop["Backpressure controlado: mailboxes mantem o item mais recente e a fila mantem o ultimo payload por topico"]

    subgraph rs[RoadSegmentationService]
        core["core_thread"]
        signbox["LatestValueMailbox TrafficSignJob"]
        streambox["LatestValueMailbox CoreFrameSnapshot"]
        teleq["TelemetryTopicQueue"]
        sign["traffic_sign_thread"]
        latest["latest_traffic_sign_result"]
        stream["stream_thread"]
        tele["telemetry_thread"]
    end

    source --> core
    core -->|ROI job| signbox
    signbox --> sign
    sign -->|resultado filtrado| latest
    sign -->|telemetry.traffic_sign_detection| teleq

    core -->|CoreFrameSnapshot| streambox
    streambox --> stream
    latest --> stream
    stream -->|vision.frame sob demanda| ws

    core -->|telemetry.road_segmentation| teleq
    core -->|telemetry.autonomous_control| teleq
    teleq --> tele
    tele -->|telemetry.* e telemetry.vision_runtime| ws

    drop -. protege .-> signbox
    drop -. protege .-> streambox
    drop -. protege .-> teleq
```

## Pipeline por frame

O `core_thread` e o ponto mais importante do runtime:

1. le um frame da fonte configurada
2. roda `RoadSegmentationPipeline::process`
3. gera `RoadSegmentationResult`
4. chama `AutonomousControlService::process`
5. produz telemetrias de segmentacao e controle
6. publica snapshot para stream/debug
7. recorta ROI para placas quando essa funcionalidade esta habilitada

Detalhes importantes:

- `near` e `mid` sao obrigatorios para rastreamento autonomo; `far` e opcional.
- o `control_sink` so existe no binario de hardware.
- o stream nao renderiza nada se nao houver `stream:subscribe=*` nem janela local.

```mermaid
flowchart LR
    capture["FrameSource read"]
    pipeline["RoadSegmentationPipeline process"]
    seg["RoadSegmentationResult"]
    control["AutonomousControlService process"]
    snapshot["CoreFrameSnapshot"]
    teleq["TelemetryTopicQueue"]
    signjob["TrafficSignJob com ROI"]
    signbox["LatestValueMailbox TrafficSignJob"]
    streambox["LatestValueMailbox CoreFrameSnapshot"]
    render["VisionDebugViewRenderer"]
    ws["WebSocketServer"]
    sink["control_sink"]
    act["Motor e servo"]

    capture --> pipeline --> seg
    seg --> control
    seg -->|telemetry.road_segmentation| teleq
    control -->|telemetry.autonomous_control| teleq

    seg --> snapshot
    control --> snapshot
    snapshot --> streambox
    streambox --> render
    render -->|vision.frame sob assinatura| ws

    capture -->|recorte ROI se placas habilitadas| signjob --> signbox

    control --> sink
    sink --> act
```

## Pipeline de placas

A deteccao de placas esta integrada ao runtime de visao, mas hoje ela ainda e apenas observacional:

- alimenta `telemetry.traffic_sign_detection`
- alimenta overlays de `annotated` e `dashboard`
- nao envia comandos de movimento ou direcao ao veiculo

Fluxo atual:

- o `core_thread` calcula a ROI na lateral direita do frame
- a ROI vira um `TrafficSignJob`
- o `traffic_sign_thread` executa `EdgeImpulseTrafficSignDetector`
- o resultado bruto passa por `TrafficSignTemporalFilter`
- o estado pode ser `disabled`, `idle`, `candidate`, `confirmed` ou `error`
- o resultado mais recente e publicado em telemetria e usado no overlay

`buildRenderableTrafficSignResult()` ainda esconde do overlay resultados muito antigos para evitar que uma deteccao expirada continue aparecendo como atual.

```mermaid
flowchart LR
    frame["Frame atual"]
    roi["buildTrafficSignRoi"]
    crop["Recorte da ROI"]
    mailbox["LatestValueMailbox TrafficSignJob"]
    detector["EdgeImpulseTrafficSignDetector"]
    filter["TrafficSignTemporalFilter"]
    states["Estados: disabled | idle | candidate | confirmed | error"]
    latest["latest_traffic_sign_result"]
    renderable["buildRenderableTrafficSignResult"]
    telemetry["telemetry.traffic_sign_detection"]
    overlay["Overlay annotated e dashboard"]
    noctrl["Hoje nao comanda o veiculo"]

    frame --> roi --> crop --> mailbox --> detector --> filter
    filter --> states
    filter --> latest
    latest --> telemetry
    latest --> renderable --> overlay
    latest -. apenas observacao .-> noctrl
```

## Protocolo WebSocket

O servidor usa um unico endpoint textual em `ws://0.0.0.0:8080`. Ele acumula tres papeis:

- roteamento de comandos
- aplicacao de configuracoes em runtime
- distribuicao de telemetria e stream de visao

Contrato de sessao:

- existe no maximo um cliente `control`
- clientes adicionais ficam como `telemetry`
- por compatibilidade, a primeira conexao que enviar `command:*` ou `config:*` sem `client:*` previo tenta assumir `control` implicitamente
- assinaturas de stream sao por sessao

```mermaid
sequenceDiagram
    autonumber
    participant a as Cliente A
    participant b as Cliente B
    participant ws as WebSocketServer
    participant reg as ClientRegistry
    participant router as CommandRouter
    participant config as ConfigurationManager

    a->>ws: config driving.mode=autonomous
    ws->>reg: ensureController(A)
    reg-->>ws: A assume control implicitamente
    ws->>config: updateSetting(driving.mode)

    b->>ws: client telemetry
    ws->>reg: requestRole(B, telemetry)
    reg-->>ws: ok

    b->>ws: client control
    ws->>reg: requestRole(B, control)
    reg-->>ws: recusado e B permanece telemetry

    a->>ws: command autonomous start
    ws->>reg: ensureController(A)
    ws->>router: route(autonomous, start, modo atual)
    router-->>ws: handled

    b->>ws: command manual left
    ws->>reg: ensureController(B)
    reg-->>ws: recusado
    Note over ws,b: comandos e configs sem papel de controle sao ignorados

    b->>ws: stream subscribe dashboard,annotated
    ws->>ws: registra views da sessao B

    ws-->>a: telemetry.* e vision.frame conforme assinatura
    ws-->>b: telemetry.* e vision.frame conforme assinatura
```

### Interfaces publicas de entrada

- `client:control`
- `client:telemetry`
- `command:manual:forward`
- `command:manual:backward`
- `command:manual:stop`
- `command:manual:throttle=<valor>`
- `command:manual:left`
- `command:manual:right`
- `command:manual:center`
- `command:manual:steering=<valor>`
- `command:autonomous:start`
- `command:autonomous:stop`
- `config:driving.mode=manual|autonomous`
- `config:steering.sensitivity=<valor>`
- `config:steering.command_step=<valor>`
- `config:autonomous.pid.kp=<valor>`
- `config:autonomous.pid.ki=<valor>`
- `config:autonomous.pid.kd=<valor>`
- `stream:subscribe=<csv_views>`

Observacao:

- `command:manual:*` so esta ativo no binario de hardware `autonomous_car_v3`
- no binario `autonomous_car_v3_vision_debug`, os comandos ativos sao os de autonomia (`start` e `stop`) e as configuracoes em runtime

### Interfaces publicas de saida

- `telemetry.road_segmentation`
- `telemetry.autonomous_control`
- `telemetry.traffic_sign_detection`
- `telemetry.vision_runtime`
- `vision.frame`

Views suportadas em `vision.frame`:

- `raw`
- `preprocess`
- `mask`
- `annotated`
- `dashboard`

## Controle autonomo

O `AutonomousControlService` usa a segmentacao para manter o veiculo alinhado com a pista. O comportamento detalhado do PID continua documentado em [`pid_control.md`](./pid_control.md), mas operacionalmente o estado atual e:

- `manual`: o roteamento aceita apenas `command:manual:*`
- `idle`: modo autonomo armado, mas ainda sem `start`
- `searching`: autonomia iniciada, mas sem pista valida naquele instante
- `tracking`: pista valida e confianca suficiente
- `fail_safe`: parada segura apos lane loss ou baixa confianca alem da tolerancia

Regras operacionais relevantes:

- `command:autonomous:start` so tem efeito quando `driving_mode=autonomous`
- `command:autonomous:stop` desarma a autonomia
- troca de modo para `manual` reseta PID e para o carro
- nao existe retomada automatica apos `fail_safe`; e preciso novo `command:autonomous:start`

```mermaid
stateDiagram-v2
    [*] --> boot
    boot --> manual : modo manual
    boot --> idle : modo autonomo

    manual --> idle : driving.mode = autonomous
    idle --> manual : driving.mode = manual
    searching --> manual : driving.mode = manual
    tracking --> manual : driving.mode = manual
    fail_safe --> manual : driving.mode = manual

    idle --> searching : command autonomous start
    searching --> tracking : pista valida e confianca ok
    tracking --> searching : pista indisponivel dentro da tolerancia

    searching --> idle : command autonomous stop
    tracking --> idle : command autonomous stop

    searching --> fail_safe : lane loss timeout excedido
    tracking --> fail_safe : lane loss timeout excedido
    searching --> fail_safe : low confidence persistente
    tracking --> fail_safe : low confidence persistente

    fail_safe --> searching : novo command autonomous start
```

## Degradacao e fail-safe

O fail-safe atual foi desenhado para parar com seguranca sem depender do cliente WebSocket para decidir isso em tempo real.

Casos principais:

- perda de pista por mais tempo que `AUTONOMOUS_LANE_LOSS_TIMEOUT_MS`
- confianca abaixo de `AUTONOMOUS_MIN_CONFIDENCE` acima da tolerancia
- `command:autonomous:stop`
- troca de modo
- encerramento do servico

No binario de hardware, isso chega ate os atuadores. No binario `vision_debug`, o efeito fica restrito a telemetria e overlay.

```mermaid
sequenceDiagram
    autonumber
    participant core as core_thread
    participant control as AutonomousControlService
    participant sink as control_sink
    participant hw as Motor e servo
    participant ws as WebSocketServer e clientes

    core->>control: process(segmentation_result, timestamp_ms)

    alt pista ausente dentro da tolerancia
        control-->>core: state=searching, motion=forward, steering=ultimo comando
        core->>ws: telemetry.autonomous_control
    else lane loss ou baixa confianca alem da tolerancia
        control-->>core: state=fail_safe, autonomous_started=false, motion=stopped
        core->>ws: telemetry.autonomous_control com stop_reason
        opt binario de hardware
            core->>sink: snapshot fail_safe
            sink->>hw: stop()
            sink->>hw: center()
        end
        Note over core,ws: no vision_debug o efeito fica apenas em debug e telemetria
    else encerramento do servico
        core->>control: stopAutonomous(service_stop)
        core->>ws: ultimo snapshot com service_stop
        opt binario de hardware
            core->>sink: snapshot service_stop
            sink->>hw: stop()
            sink->>hw: center()
        end
    end
```

## Arquivos de configuracao

Responsabilidades dos arquivos carregados no runtime:

- `config/autonomous_car.env`
  - pinos e dinamica de motor
  - centro e limites de direcao
  - `DRIVING_MODE`
  - tuning do PID autonomo
  - limites de fail-safe
- `config/vision.env`
  - origem de captura
  - camera index ou path local
  - habilitacao da janela local
  - `VISION_TELEMETRY_MAX_FPS`
  - `VISION_STREAM_MAX_FPS`
  - `TRAFFIC_SIGN_TARGET_FPS`
  - `VISION_STREAM_JPEG_QUALITY`
  - paths dos arquivos de segmentacao e placas
- `config/road_segmentation.env`
  - parametros `LANE_*` do pipeline compartilhado
- `config/traffic_sign.env`
  - enable/disable de placas
  - definicao da ROI
  - confianca minima
  - filtro temporal de confirmacao e expiracao

## Limites atuais

- o alvo `autonomous_car_v3` depende de `wiringPi`; quando a biblioteca nao existe, esse binario nao e gerado
- sem `wiringPi`, o fluxo suportado e `autonomous_car_v3_vision_debug`
- o stream de visao usa o mesmo WebSocket textual; nao existe endpoint HTTP/MJPEG separado
- a deteccao de placas hoje nao comanda o veiculo; ela alimenta apenas telemetria e overlay

