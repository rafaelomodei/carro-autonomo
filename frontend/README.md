# Frontend

Interface Next.js para operar o `autonomous_car_v3` via WebSocket.

## Ambiente

Crie um arquivo `.env.local` em `frontend/` com as credenciais Firebase:

```bash
FIREBASE_API_KEY=
FIREBASE_AUTH_DOMAIN=
FIREBASE_PROJECT_ID=
FIREBASE_STORAGE_BUCKET=
FIREBASE_MESSAGING_SENDER_ID=
FIREBASE_APP_ID=
NEXT_PUBLIC_DEFAULT_VEHICLE_WS_URL=ws://192.168.15.163:8080
```

## Desenvolvimento

```bash
npm run dev
```

App local: `http://localhost:3000`

Checks:

```bash
npm run lint
npm run test:traffic-sign
npm run build
```

## Contrato com o `autonomous_car_v3`

O frontend usa uma unica conexao WebSocket em papel `client:control`.

URL esperada:

```text
ws://<host>:8080
```

### Mensagens enviadas

O frontend fala apenas o protocolo textual atual do backend:

```text
client:control
config:driving.mode=manual|autonomous
config:steering.sensitivity=<valor>
config:steering.command_step=<valor>
config:autonomous.pid.kp=<valor>
config:autonomous.pid.ki=<valor>
config:autonomous.pid.kd=<valor>
command:manual:forward
command:manual:backward
command:manual:stop
command:manual:left
command:manual:right
command:manual:center
command:autonomous:start
command:autonomous:stop
```

### Mensagens recebidas

O frontend interpreta apenas:

- `telemetry.road_segmentation`
- `telemetry.autonomous_control`
- `telemetry.traffic_sign_detection`
- `telemetry.vision_runtime`
- `vision.frame` em uma segunda conexao WebSocket dedicada ao stream de visao

O estado operacional da UI vem dessas telemetrias. O front pode mostrar um estado pendente logo apos o envio de um comando, mas a confirmacao real vem do backend.
O frontend nao consome diretamente o `traffic_sign_service`; a tela `/debug`
recebe a sinalizacao sempre via `autonomous_car_v3`. A UI tambem nao interpreta
`signal:detected=<signal_id>` e usa apenas o payload JSON estruturado.

### Stream de visao

Na tela `/debug`, o frontend abre uma segunda conexao no mesmo `ws://<host>:8080` quando ao menos uma view esta selecionada.

Mensagens enviadas nessa conexao:

```text
client:telemetry
stream:subscribe=raw,mask,annotated
```

Mensagens recebidas:

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

## Comportamento atual da UI

- `Settings` controla conexao, modo de conducao, `start/stop` autonomo, `steering.sensitivity`, `steering.command_step` e `Kp/Ki/Kd`.
- `Joystick` opera em modo `segurar para mover`: enquanto o botao ou tecla estiver pressionado, o frontend reenfileira comandos manuais em intervalo curto para nao estourar o timeout do backend.
- `Debug` virou a central de visao: seleciona views, abre o stream sob demanda e combina os frames com `telemetry.road_segmentation`, `telemetry.autonomous_control`, `telemetry.traffic_sign_detection` e `telemetry.vision_runtime` recebidas no socket de controle.
- O frontend persiste localmente a URL do WebSocket e os ajustes de runtime que ele controla, reaplicando-os quando reconecta.
- `command:autonomous:start` nao e reenviado automaticamente em reconexoes.

## Reconexao

Quando existe uma URL valida salva, o frontend tenta reconectar automaticamente com backoff progressivo.

Estados publicos usados na UI:

- `disconnected`
- `connecting`
- `connected`
- `reconnecting`
- `telemetry_stale`

`telemetry_stale` indica que o socket continua aberto, mas a ultima telemetria recebida passou do limite esperado.

## Limites desta entrega

- Nao ha compatibilidade com o protocolo JSON antigo do frontend.
- O stream de visao usa JSON + JPEG em base64, nao blobs binarios.
- Nao ha painel de `signals` legado.
- O frontend nao recebe snapshot completo das configuracoes do backend; ele reaplica apenas os parametros que controla.

## Vercel

O repositório inclui `vercel.json` com `ignored build step`. O script
`scripts/skip-frontend-build.sh` evita build na Vercel quando nada em `frontend/`
foi alterado.
