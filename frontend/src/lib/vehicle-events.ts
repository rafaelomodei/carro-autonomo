'use client';

import {
  VehicleTelemetryMessage,
  VisionFrameMessage,
} from '@/lib/autonomous-car';

export const TELEMETRY_RECEIVED_EVENT = 'autonomous-car:telemetry-received';
export const VISION_FRAME_RECEIVED_EVENT = 'autonomous-car:vision-frame-received';
export const UI_LOG_EVENT = 'autonomous-car:ui-log';

export interface TelemetryReceivedDetail {
  message: VehicleTelemetryMessage;
  receivedAtMs: number;
}

export interface VisionFrameReceivedDetail {
  frame: VisionFrameMessage;
  receivedAtMs: number;
}

export interface UiLogEventDetail {
  eventType: string;
  timestampMs: number;
  payload: Record<string, unknown> | null;
}

const emitCustomEvent = <Detail>(eventName: string, detail: Detail) => {
  if (typeof window === 'undefined') {
    return;
  }

  window.dispatchEvent(new CustomEvent<Detail>(eventName, { detail }));
};

export const emitTelemetryReceived = (detail: TelemetryReceivedDetail) => {
  emitCustomEvent(TELEMETRY_RECEIVED_EVENT, detail);
};

export const emitVisionFrameReceived = (detail: VisionFrameReceivedDetail) => {
  emitCustomEvent(VISION_FRAME_RECEIVED_EVENT, detail);
};

export const emitUiLogEvent = (
  eventType: string,
  payload: Record<string, unknown> | null = null
) => {
  emitCustomEvent(UI_LOG_EVENT, {
    eventType,
    timestampMs: Date.now(),
    payload,
  } satisfies UiLogEventDetail);
};
