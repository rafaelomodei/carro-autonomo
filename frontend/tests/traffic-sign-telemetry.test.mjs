import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';

import ts from 'typescript';

const sourcePath = path.resolve(process.cwd(), 'src/lib/autonomous-car.ts');
const source = fs.readFileSync(sourcePath, 'utf8');
const transpiled = ts.transpileModule(source, {
  compilerOptions: {
    module: ts.ModuleKind.CommonJS,
    target: ts.ScriptTarget.ES2020,
  },
}).outputText;

const moduleRef = { exports: {} };
const sandbox = {
  module: moduleRef,
  exports: moduleRef.exports,
  require: (specifier) => {
    throw new Error(`Unexpected require in test harness: ${specifier}`);
  },
  console,
  process,
};

vm.runInNewContext(transpiled, sandbox, {
  filename: 'autonomous-car.test.js',
});

const {
  formatTrafficSignDetectorStateLabel,
  formatTrafficSignIdLabel,
  parseTelemetryMessage,
} = moduleRef.exports;

const telemetryPayload = JSON.stringify({
  type: 'telemetry.traffic_sign_detection',
  timestamp_ms: 123456789,
  source: 'Camera index 0',
  detector_state: 'confirmed',
  roi: {
    left_ratio: 0.55,
    right_ratio: 1.0,
    right_width_ratio: 0.45,
    top_ratio: 0.08,
    bottom_ratio: 0.72,
  },
  raw_detections: [
    {
      sign_id: 'stop',
      model_label: 'Parada Obrigatoria sign',
      display_label: 'Parada obrigatoria',
      confidence_score: 0.97,
      bbox_frame: { x: 200, y: 40, width: 50, height: 50 },
      bbox_roi: { x: 70, y: 10, width: 50, height: 50 },
      consecutive_frames: 2,
      required_frames: 2,
      confirmed_at_ms: 123456700,
      last_seen_at_ms: 123456789,
    },
  ],
  candidate: null,
  active_detection: {
    sign_id: 'stop',
    model_label: 'Parada Obrigatoria sign',
    display_label: 'Parada obrigatoria',
    confidence_score: 0.97,
    bbox_frame: { x: 200, y: 40, width: 50, height: 50 },
    bbox_roi: { x: 70, y: 10, width: 50, height: 50 },
    consecutive_frames: 2,
    required_frames: 2,
    confirmed_at_ms: 123456700,
    last_seen_at_ms: 123456789,
  },
  last_error: '',
});

const parsed = parseTelemetryMessage(telemetryPayload);
const runtimeParsed = parseTelemetryMessage(
  JSON.stringify({
    type: 'telemetry.vision_runtime',
    timestamp_ms: 123456800,
    source: 'Camera index 0',
    core_fps: 19.2,
    stream_fps: 4.5,
    traffic_sign_fps: 3.9,
    traffic_sign_inference_ms: 11.2,
    stream_encode_ms: 24.4,
    traffic_sign_dropped_frames: 2,
    stream_dropped_frames: 6,
    sign_result_age_ms: 140,
  })
);

assert.ok(parsed, 'A nova telemetria de sinalizacao deve ser aceita pelo parser.');
assert.equal(
  parsed.type,
  'telemetry.traffic_sign_detection',
  'O parser deve identificar o novo tipo de telemetria.'
);
assert.equal(
  parsed.active_detection?.display_label,
  'Parada obrigatoria',
  'A deteccao ativa deve ser preservada no payload parseado.'
);
assert.equal(
  runtimeParsed?.type,
  'telemetry.vision_runtime',
  'O parser deve aceitar a nova telemetria de runtime.'
);
assert.equal(
  runtimeParsed?.stream_dropped_frames,
  6,
  'A telemetria de runtime deve preservar contadores de descarte.'
);
assert.equal(
  formatTrafficSignDetectorStateLabel('idle'),
  'Aguardando deteccao',
  'O formatter deve cobrir o estado idle.'
);
assert.equal(
  formatTrafficSignDetectorStateLabel('candidate'),
  'Candidato',
  'O formatter deve cobrir o estado candidate.'
);
assert.equal(
  formatTrafficSignDetectorStateLabel('confirmed'),
  'Confirmado',
  'O formatter deve cobrir o estado confirmed.'
);
assert.equal(
  formatTrafficSignDetectorStateLabel('error'),
  'Erro',
  'O formatter deve cobrir o estado error.'
);
assert.equal(
  formatTrafficSignIdLabel('turn_left'),
  'Vire a esquerda',
  'O formatter de sign_id deve cobrir a placa de vire a esquerda.'
);

console.log('Frontend traffic sign telemetry tests passed.');
