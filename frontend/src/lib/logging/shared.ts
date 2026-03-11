import type {
  AutonomousControlTelemetry,
  RoadSegmentationTelemetry,
} from '@/lib/autonomous-car';

export const RECORDING_VIEW_IDS = [
  'raw',
  'preprocess',
  'mask',
  'annotated',
  'dashboard',
] as const;

export type RecordingViewId = (typeof RECORDING_VIEW_IDS)[number];
export type RecordingState = 'idle' | 'starting' | 'recording' | 'stopping' | 'error';
export type ConversionState = 'idle' | 'converting' | 'completed' | 'failed';

export interface TelemetryRawRecord {
  received_at_ms: number;
  type: RoadSegmentationTelemetry['type'] | AutonomousControlTelemetry['type'];
  payload: RoadSegmentationTelemetry | AutonomousControlTelemetry;
}

export interface UiLogEventRecord {
  event_type: string;
  timestamp_ms: number;
  payload: Record<string, unknown> | null;
}

export interface VisionFrameIndexRecord {
  view: RecordingViewId;
  timestamp_ms: number;
  received_at_ms: number;
  width: number;
  height: number;
}

export interface SessionTelemetryCounts {
  telemetry_raw: number;
  road_segmentation: number;
  autonomous_control: number;
  ui_events: number;
  frame_index: number;
}

export interface SessionVideoOutput {
  view: RecordingViewId;
  mp4_path: string | null;
  temporary_webm_path: string | null;
  bytes_written: number;
  converted: boolean;
}

export interface SessionConversionStatus {
  overall: 'completed' | 'partial' | 'failed';
  views: Record<
    RecordingViewId,
    {
      status: 'converted' | 'missing' | 'failed';
      message?: string;
    }
  >;
}

export interface SessionStartRequest {
  ws_url: string;
  recorded_views: RecordingViewId[];
  user_selected_views_at_start: RecordingViewId[];
}

export interface SessionStartResponse {
  session_id: string;
  session_dir: string;
}

export interface SessionFinishRequest {
  ended_at_ms: number;
  stop_reason: string;
  automatic_stop: boolean;
  telemetry_counts: SessionTelemetryCounts;
  video_outputs: Record<RecordingViewId, string>;
  errors: string[];
}

export const ROAD_SEGMENTATION_CSV_HEADERS = [
  'timestamp_ms',
  'received_at_ms',
  'source',
  'lane_found',
  'confidence_score',
  'lane_center_ratio',
  'steering_error_normalized',
  'lateral_offset_px',
  'heading_error_rad',
  'heading_valid',
  'curvature_indicator_rad',
  'curvature_valid',
  'references_near_valid',
  'references_near_point_x',
  'references_near_point_y',
  'references_near_top_y',
  'references_near_bottom_y',
  'references_near_center_ratio',
  'references_near_lateral_offset_px',
  'references_near_steering_error_normalized',
  'references_near_sample_count',
  'references_mid_valid',
  'references_mid_point_x',
  'references_mid_point_y',
  'references_mid_top_y',
  'references_mid_bottom_y',
  'references_mid_center_ratio',
  'references_mid_lateral_offset_px',
  'references_mid_steering_error_normalized',
  'references_mid_sample_count',
  'references_far_valid',
  'references_far_point_x',
  'references_far_point_y',
  'references_far_top_y',
  'references_far_bottom_y',
  'references_far_center_ratio',
  'references_far_lateral_offset_px',
  'references_far_steering_error_normalized',
  'references_far_sample_count',
] as const;

export const AUTONOMOUS_CONTROL_CSV_HEADERS = [
  'timestamp_ms',
  'received_at_ms',
  'driving_mode',
  'autonomous_started',
  'tracking_state',
  'stop_reason',
  'fail_safe_active',
  'lane_available',
  'confidence_ok',
  'confidence_score',
  'preview_error',
  'heading_error_rad',
  'heading_valid',
  'curvature_indicator_rad',
  'curvature_valid',
  'references_near_valid',
  'references_near_used',
  'references_near_configured_weight',
  'references_near_applied_weight',
  'references_near_lateral_offset_px',
  'references_near_steering_error_normalized',
  'references_mid_valid',
  'references_mid_used',
  'references_mid_configured_weight',
  'references_mid_applied_weight',
  'references_mid_lateral_offset_px',
  'references_mid_steering_error_normalized',
  'references_far_valid',
  'references_far_used',
  'references_far_configured_weight',
  'references_far_applied_weight',
  'references_far_lateral_offset_px',
  'references_far_steering_error_normalized',
  'pid_error',
  'pid_p',
  'pid_i',
  'pid_d',
  'pid_raw_output',
  'pid_output',
  'steering_command',
  'motion_command',
  'projected_path_json',
] as const;

export type RoadSegmentationCsvRow = Record<
  (typeof ROAD_SEGMENTATION_CSV_HEADERS)[number],
  boolean | number | string | null
>;

export type AutonomousControlCsvRow = Record<
  (typeof AUTONOMOUS_CONTROL_CSV_HEADERS)[number],
  boolean | number | string | null
>;

export interface AppendSessionRequest {
  telemetry_raw?: TelemetryRawRecord[];
  road_segmentation_rows?: RoadSegmentationCsvRow[];
  autonomous_control_rows?: AutonomousControlCsvRow[];
  ui_events?: UiLogEventRecord[];
  frame_index?: VisionFrameIndexRecord[];
}

export const createEmptyTelemetryCounts = (): SessionTelemetryCounts => ({
  telemetry_raw: 0,
  road_segmentation: 0,
  autonomous_control: 0,
  ui_events: 0,
  frame_index: 0,
});

const stringifyProjectedPath = (
  projectedPath: AutonomousControlTelemetry['projected_path']
) => JSON.stringify(projectedPath);

export const flattenRoadSegmentationTelemetry = (
  telemetry: RoadSegmentationTelemetry,
  receivedAtMs: number
): RoadSegmentationCsvRow => ({
  timestamp_ms: telemetry.timestamp_ms,
  received_at_ms: receivedAtMs,
  source: telemetry.source,
  lane_found: telemetry.lane_found,
  confidence_score: telemetry.confidence_score,
  lane_center_ratio: telemetry.lane_center_ratio,
  steering_error_normalized: telemetry.steering_error_normalized,
  lateral_offset_px: telemetry.lateral_offset_px,
  heading_error_rad: telemetry.heading_error_rad,
  heading_valid: telemetry.heading_valid,
  curvature_indicator_rad: telemetry.curvature_indicator_rad,
  curvature_valid: telemetry.curvature_valid,
  references_near_valid: telemetry.references.near.valid,
  references_near_point_x: telemetry.references.near.point_x ?? null,
  references_near_point_y: telemetry.references.near.point_y ?? null,
  references_near_top_y: telemetry.references.near.top_y ?? null,
  references_near_bottom_y: telemetry.references.near.bottom_y ?? null,
  references_near_center_ratio: telemetry.references.near.center_ratio ?? null,
  references_near_lateral_offset_px: telemetry.references.near.lateral_offset_px,
  references_near_steering_error_normalized:
    telemetry.references.near.steering_error_normalized,
  references_near_sample_count: telemetry.references.near.sample_count ?? null,
  references_mid_valid: telemetry.references.mid.valid,
  references_mid_point_x: telemetry.references.mid.point_x ?? null,
  references_mid_point_y: telemetry.references.mid.point_y ?? null,
  references_mid_top_y: telemetry.references.mid.top_y ?? null,
  references_mid_bottom_y: telemetry.references.mid.bottom_y ?? null,
  references_mid_center_ratio: telemetry.references.mid.center_ratio ?? null,
  references_mid_lateral_offset_px: telemetry.references.mid.lateral_offset_px,
  references_mid_steering_error_normalized:
    telemetry.references.mid.steering_error_normalized,
  references_mid_sample_count: telemetry.references.mid.sample_count ?? null,
  references_far_valid: telemetry.references.far.valid,
  references_far_point_x: telemetry.references.far.point_x ?? null,
  references_far_point_y: telemetry.references.far.point_y ?? null,
  references_far_top_y: telemetry.references.far.top_y ?? null,
  references_far_bottom_y: telemetry.references.far.bottom_y ?? null,
  references_far_center_ratio: telemetry.references.far.center_ratio ?? null,
  references_far_lateral_offset_px: telemetry.references.far.lateral_offset_px,
  references_far_steering_error_normalized:
    telemetry.references.far.steering_error_normalized,
  references_far_sample_count: telemetry.references.far.sample_count ?? null,
});

export const flattenAutonomousControlTelemetry = (
  telemetry: AutonomousControlTelemetry,
  receivedAtMs: number
): AutonomousControlCsvRow => ({
  timestamp_ms: telemetry.timestamp_ms,
  received_at_ms: receivedAtMs,
  driving_mode: telemetry.driving_mode,
  autonomous_started: telemetry.autonomous_started,
  tracking_state: telemetry.tracking_state,
  stop_reason: telemetry.stop_reason,
  fail_safe_active: telemetry.fail_safe_active,
  lane_available: telemetry.lane_available,
  confidence_ok: telemetry.confidence_ok,
  confidence_score: telemetry.confidence_score,
  preview_error: telemetry.preview_error,
  heading_error_rad: telemetry.heading_error_rad,
  heading_valid: telemetry.heading_valid,
  curvature_indicator_rad: telemetry.curvature_indicator_rad,
  curvature_valid: telemetry.curvature_valid,
  references_near_valid: telemetry.references.near.valid,
  references_near_used: telemetry.references.near.used ?? false,
  references_near_configured_weight:
    telemetry.references.near.configured_weight ?? null,
  references_near_applied_weight: telemetry.references.near.applied_weight ?? null,
  references_near_lateral_offset_px: telemetry.references.near.lateral_offset_px,
  references_near_steering_error_normalized:
    telemetry.references.near.steering_error_normalized,
  references_mid_valid: telemetry.references.mid.valid,
  references_mid_used: telemetry.references.mid.used ?? false,
  references_mid_configured_weight:
    telemetry.references.mid.configured_weight ?? null,
  references_mid_applied_weight: telemetry.references.mid.applied_weight ?? null,
  references_mid_lateral_offset_px: telemetry.references.mid.lateral_offset_px,
  references_mid_steering_error_normalized:
    telemetry.references.mid.steering_error_normalized,
  references_far_valid: telemetry.references.far.valid,
  references_far_used: telemetry.references.far.used ?? false,
  references_far_configured_weight:
    telemetry.references.far.configured_weight ?? null,
  references_far_applied_weight: telemetry.references.far.applied_weight ?? null,
  references_far_lateral_offset_px: telemetry.references.far.lateral_offset_px,
  references_far_steering_error_normalized:
    telemetry.references.far.steering_error_normalized,
  pid_error: telemetry.pid.error,
  pid_p: telemetry.pid.p,
  pid_i: telemetry.pid.i,
  pid_d: telemetry.pid.d,
  pid_raw_output: telemetry.pid.raw_output,
  pid_output: telemetry.pid.output,
  steering_command: telemetry.steering_command,
  motion_command: telemetry.motion_command,
  projected_path_json: stringifyProjectedPath(telemetry.projected_path),
});

export const serializeCsvValue = (value: boolean | number | string | null) => {
  if (value === null || typeof value === 'undefined') {
    return '';
  }

  if (typeof value === 'boolean' || typeof value === 'number') {
    return String(value);
  }

  if (value.includes('"') || value.includes(',') || value.includes('\n')) {
    return `"${value.replaceAll('"', '""')}"`;
  }

  return value;
};

export const serializeCsvRow = <Header extends string>(
  headers: readonly Header[],
  row: Record<Header, boolean | number | string | null>
) => `${headers.map((header) => serializeCsvValue(row[header])).join(',')}\n`;
