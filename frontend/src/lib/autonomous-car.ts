'use client';

export type DrivingMode = 'manual' | 'autonomous';
export type ConnectionState =
  | 'disconnected'
  | 'connecting'
  | 'connected'
  | 'reconnecting'
  | 'telemetry_stale';
export type ManualMotionCommand = 'forward' | 'backward' | 'stop';
export type ManualSteeringCommand = 'left' | 'right' | 'center';
export const VISION_DEBUG_VIEW_IDS = [
  'raw',
  'preprocess',
  'mask',
  'annotated',
  'dashboard',
] as const;
export type VisionDebugViewId = (typeof VISION_DEBUG_VIEW_IDS)[number];
export type RuntimeSettingKey =
  | 'steering.sensitivity'
  | 'steering.command_step'
  | 'autonomous.pid.kp'
  | 'autonomous.pid.ki'
  | 'autonomous.pid.kd';

export interface TelemetryReference {
  valid: boolean;
  point_x?: number;
  point_y?: number;
  top_y?: number;
  bottom_y?: number;
  center_ratio?: number;
  lateral_offset_px: number;
  steering_error_normalized: number;
  sample_count?: number;
  used?: boolean;
  configured_weight?: number;
  applied_weight?: number;
}

export interface RoadSegmentationTelemetry {
  type: 'telemetry.road_segmentation';
  timestamp_ms: number;
  source: string;
  lane_found: boolean;
  confidence_score: number;
  lane_center_ratio: number;
  steering_error_normalized: number;
  lateral_offset_px: number;
  heading_error_rad: number;
  heading_valid: boolean;
  curvature_indicator_rad: number;
  curvature_valid: boolean;
  references: {
    near: TelemetryReference;
    mid: TelemetryReference;
    far: TelemetryReference;
  };
}

export interface AutonomousControlTelemetry {
  type: 'telemetry.autonomous_control';
  timestamp_ms: number;
  driving_mode: DrivingMode;
  autonomous_started: boolean;
  tracking_state: 'manual' | 'idle' | 'searching' | 'tracking' | 'fail_safe';
  stop_reason:
    | 'none'
    | 'command_stop'
    | 'mode_change'
    | 'lane_lost'
    | 'low_confidence'
    | 'service_stop';
  fail_safe_active: boolean;
  lane_available: boolean;
  confidence_ok: boolean;
  confidence_score: number;
  preview_error: number;
  heading_error_rad: number;
  heading_valid: boolean;
  curvature_indicator_rad: number;
  curvature_valid: boolean;
  references: {
    near: TelemetryReference;
    mid: TelemetryReference;
    far: TelemetryReference;
  };
  pid: {
    error: number;
    p: number;
    i: number;
    d: number;
    raw_output: number;
    output: number;
  };
  steering_command: number;
  motion_command: 'stopped' | 'forward';
  projected_path: Array<{
    x: number;
    y: number;
  }>;
}

export type TrafficSignDetectorState =
  | 'disabled'
  | 'idle'
  | 'candidate'
  | 'confirmed'
  | 'error';

export type TrafficSignId = 'stop' | 'turn_left' | 'turn_right' | 'unknown';

export interface TrafficSignBoundingBox {
  x: number;
  y: number;
  width: number;
  height: number;
}

export interface TrafficSignDetection {
  sign_id: TrafficSignId;
  model_label: string;
  display_label: string;
  confidence_score: number;
  bbox_frame: TrafficSignBoundingBox;
  bbox_roi: TrafficSignBoundingBox;
  consecutive_frames: number;
  required_frames: number;
  confirmed_at_ms: number;
  last_seen_at_ms: number;
}

export interface TrafficSignDetectionTelemetry {
  type: 'telemetry.traffic_sign_detection';
  timestamp_ms: number;
  source: string;
  detector_state: TrafficSignDetectorState;
  roi: {
    right_width_ratio: number;
    top_ratio: number;
    bottom_ratio: number;
  };
  raw_detections: TrafficSignDetection[];
  candidate: TrafficSignDetection | null;
  active_detection: TrafficSignDetection | null;
  last_error: string;
}

export interface VisionFrameMessage {
  type: 'vision.frame';
  view: VisionDebugViewId;
  timestamp_ms: number;
  mime: string;
  width: number;
  height: number;
  data: string;
}

export type VehicleTelemetryMessage =
  | RoadSegmentationTelemetry
  | AutonomousControlTelemetry
  | TrafficSignDetectionTelemetry;

export const DEFAULT_VEHICLE_CONNECTION_URL =
  process.env.NEXT_PUBLIC_DEFAULT_VEHICLE_WS_URL?.trim() ||
  'ws://192.168.15.163:8080';

export interface RuntimePidConfig {
  p: number;
  i: number;
  d: number;
}

export interface VehicleRuntimeConfig {
  connectionUrl: string;
  steeringSensitivity: number;
  steeringCommandStep: number;
  pidControl: RuntimePidConfig;
}

export const DEFAULT_VEHICLE_RUNTIME_CONFIG: VehicleRuntimeConfig = {
  connectionUrl: DEFAULT_VEHICLE_CONNECTION_URL,
  steeringSensitivity: 1,
  steeringCommandStep: 0.2,
  pidControl: {
    p: 0.45,
    i: 0.02,
    d: 0.08,
  },
};

export const VEHICLE_RUNTIME_STORAGE_KEY = 'autonomous_car_v3.frontend.runtime';
export const TELEMETRY_STALE_THRESHOLD_MS = 3000;
export const MANUAL_COMMAND_INTERVAL_MS = 75;
export const RECONNECT_DELAYS_MS = [500, 1000, 2000, 4000, 8000];

export const parseTelemetryMessage = (
  payload: string
): VehicleTelemetryMessage | null => {
  try {
    const parsed = JSON.parse(payload) as VehicleTelemetryMessage;

    if (
      parsed &&
      typeof parsed === 'object' &&
      (parsed.type === 'telemetry.road_segmentation' ||
        parsed.type === 'telemetry.autonomous_control' ||
        parsed.type === 'telemetry.traffic_sign_detection')
    ) {
      return parsed;
    }

    return null;
  } catch (error) {
    console.warn('Mensagem WebSocket ignorada por JSON invalido.', error);
    return null;
  }
};

export const buildClientRoleMessage = (role: 'control' | 'telemetry'): string =>
  `client:${role}`;

export const buildDrivingModeMessage = (mode: DrivingMode): string =>
  `config:driving.mode=${mode}`;

export const buildRuntimeSettingMessage = (
  key: RuntimeSettingKey,
  value: number
): string => `config:${key}=${value}`;

export const buildManualMotionMessage = (
  command: ManualMotionCommand
): string => `command:manual:${command}`;

export const buildManualSteeringMessage = (
  command: ManualSteeringCommand
): string => `command:manual:${command}`;

export const buildAutonomousCommandMessage = (
  command: 'start' | 'stop'
): string => `command:autonomous:${command}`;

export const buildVisionStreamSubscriptionMessage = (
  views: VisionDebugViewId[]
): string => `stream:subscribe=${views.join(',')}`;

export const applyRuntimeSetting = (
  current: VehicleRuntimeConfig,
  key: RuntimeSettingKey,
  value: number
): VehicleRuntimeConfig => {
  switch (key) {
    case 'steering.sensitivity':
      return {
        ...current,
        steeringSensitivity: value,
      };
    case 'steering.command_step':
      return {
        ...current,
        steeringCommandStep: value,
      };
    case 'autonomous.pid.kp':
      return {
        ...current,
        pidControl: {
          ...current.pidControl,
          p: value,
        },
      };
    case 'autonomous.pid.ki':
      return {
        ...current,
        pidControl: {
          ...current.pidControl,
          i: value,
        },
      };
    case 'autonomous.pid.kd':
      return {
        ...current,
        pidControl: {
          ...current.pidControl,
          d: value,
        },
      };
  }
};

export const normalizeVehicleRuntimeConfig = (
  input: Partial<VehicleRuntimeConfig> | null | undefined
): VehicleRuntimeConfig => {
  const pidControl: Partial<RuntimePidConfig> = input?.pidControl ?? {};

  return {
    connectionUrl:
      typeof input?.connectionUrl === 'string'
        ? input.connectionUrl
        : DEFAULT_VEHICLE_RUNTIME_CONFIG.connectionUrl,
    steeringSensitivity:
      typeof input?.steeringSensitivity === 'number'
        ? input.steeringSensitivity
        : DEFAULT_VEHICLE_RUNTIME_CONFIG.steeringSensitivity,
    steeringCommandStep:
      typeof input?.steeringCommandStep === 'number'
        ? input.steeringCommandStep
        : DEFAULT_VEHICLE_RUNTIME_CONFIG.steeringCommandStep,
    pidControl: {
      p:
        typeof pidControl.p === 'number'
          ? pidControl.p
          : DEFAULT_VEHICLE_RUNTIME_CONFIG.pidControl.p,
      i:
        typeof pidControl.i === 'number'
          ? pidControl.i
          : DEFAULT_VEHICLE_RUNTIME_CONFIG.pidControl.i,
      d:
        typeof pidControl.d === 'number'
          ? pidControl.d
          : DEFAULT_VEHICLE_RUNTIME_CONFIG.pidControl.d,
    },
  };
};

export const parseVisionFrameMessage = (
  payload: string
): VisionFrameMessage | null => {
  try {
    const parsed = JSON.parse(payload) as Partial<VisionFrameMessage>;
    const validView =
      typeof parsed.view === 'string' &&
      VISION_DEBUG_VIEW_IDS.includes(parsed.view as VisionDebugViewId);

    if (
      parsed.type === 'vision.frame' &&
      validView &&
      typeof parsed.timestamp_ms === 'number' &&
      typeof parsed.mime === 'string' &&
      typeof parsed.width === 'number' &&
      typeof parsed.height === 'number' &&
      typeof parsed.data === 'string'
    ) {
      return parsed as VisionFrameMessage;
    }

    return null;
  } catch (error) {
    console.warn('Mensagem de frame ignorada por JSON invalido.', error);
    return null;
  }
};

export const formatConnectionStateLabel = (state: ConnectionState): string => {
  switch (state) {
    case 'disconnected':
      return 'Desconectado';
    case 'connecting':
      return 'Conectando';
    case 'connected':
      return 'Conectado';
    case 'reconnecting':
      return 'Reconectando';
    case 'telemetry_stale':
      return 'Telemetria atrasada';
  }
};

export const formatDrivingModeLabel = (mode: DrivingMode): string =>
  mode === 'manual' ? 'Conducao manual' : 'Conducao autonoma';

export const formatTrackingStateLabel = (
  state: AutonomousControlTelemetry['tracking_state']
): string => {
  switch (state) {
    case 'manual':
      return 'Manual';
    case 'idle':
      return 'Aguardando start';
    case 'searching':
      return 'Buscando pista';
    case 'tracking':
      return 'Rastreando pista';
    case 'fail_safe':
      return 'Fail-safe';
  }
};

export const formatStopReasonLabel = (
  reason: AutonomousControlTelemetry['stop_reason']
): string => {
  switch (reason) {
    case 'none':
      return 'Nenhum';
    case 'command_stop':
      return 'Parada por comando';
    case 'mode_change':
      return 'Troca de modo';
    case 'lane_lost':
      return 'Pista perdida';
    case 'low_confidence':
      return 'Baixa confianca';
    case 'service_stop':
      return 'Servico encerrado';
  }
};

export const formatTrafficSignDetectorStateLabel = (
  state: TrafficSignDetectorState
): string => {
  switch (state) {
    case 'disabled':
      return 'Desabilitado';
    case 'idle':
      return 'Aguardando deteccao';
    case 'candidate':
      return 'Candidato';
    case 'confirmed':
      return 'Confirmado';
    case 'error':
      return 'Erro';
  }
};

export const formatTrafficSignIdLabel = (signId: TrafficSignId): string => {
  switch (signId) {
    case 'stop':
      return 'Parada obrigatoria';
    case 'turn_left':
      return 'Vire a esquerda';
    case 'turn_right':
      return 'Vire a direita';
    case 'unknown':
      return 'Sinal desconhecido';
  }
};

export const formatVisionDebugViewLabel = (view: VisionDebugViewId): string => {
  switch (view) {
    case 'raw':
      return 'Camera';
    case 'preprocess':
      return 'Preprocessamento';
    case 'mask':
      return 'Mascara';
    case 'annotated':
      return 'Saida anotada';
    case 'dashboard':
      return 'Dashboard';
  }
};
