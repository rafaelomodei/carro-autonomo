'use client';

import {
  createContext,
  ReactNode,
  useContext,
  useEffect,
  useRef,
  useState,
} from 'react';
import { VisionFrameMessage } from '@/lib/autonomous-car';
import {
  AppendSessionRequest,
  ConversionState,
  createEmptyTelemetryCounts,
  flattenAutonomousControlTelemetry,
  flattenRoadSegmentationTelemetry,
  RECORDING_VIEW_IDS,
  RecordingState,
  RecordingViewId,
  SessionFinishRequest,
  SessionStartRequest,
  SessionStartResponse,
  SessionTelemetryCounts,
  UiLogEventRecord,
} from '@/lib/logging/shared';
import {
  TELEMETRY_RECEIVED_EVENT,
  TelemetryReceivedDetail,
  UI_LOG_EVENT,
  UiLogEventDetail,
  VISION_FRAME_RECEIVED_EVENT,
  VisionFrameReceivedDetail,
} from '@/lib/vehicle-events';
import { useVehicleConfig } from '@/providers/VehicleConfigProvider';
import { useVehicleVision } from '@/providers/VehicleVisionProvider';

interface VehicleLoggingContextProps {
  activeSessionId: string | null;
  conversionState: ConversionState;
  lastStopReason: string | null;
  recordingStartedAt: number | null;
  recordingState: RecordingState;
  recordingViews: RecordingViewId[];
  toggleRecording: () => Promise<void>;
  writeError: string | null;
}

const VehicleLoggingContext = createContext<VehicleLoggingContextProps | null>(
  null
);

const DEFAULT_CANVAS_WIDTH = 1280;
const DEFAULT_CANVAS_HEIGHT = 720;
const VIDEO_TIMESLICE_MS = 2000;
const FLUSH_INTERVAL_MS = 1000;

const createRecordingViewMap = <T,>(
  factory: (view: RecordingViewId) => T
): Record<RecordingViewId, T> =>
  RECORDING_VIEW_IDS.reduce(
    (accumulator, view) => ({
      ...accumulator,
      [view]: factory(view),
    }),
    {} as Record<RecordingViewId, T>
  );

const createEmptyAppendPayload = (): AppendSessionRequest => ({});

const hasAppendPayloadContent = (payload: AppendSessionRequest) =>
  Boolean(
    payload.telemetry_raw?.length ||
      payload.road_segmentation_rows?.length ||
      payload.autonomous_control_rows?.length ||
      payload.ui_events?.length ||
      payload.frame_index?.length
  );

const getMediaRecorderMimeType = () => {
  if (typeof MediaRecorder === 'undefined') {
    return null;
  }

  const preferredTypes = [
    'video/webm;codecs=vp9',
    'video/webm;codecs=vp8',
    'video/webm',
  ];

  return (
    preferredTypes.find((mimeType) => MediaRecorder.isTypeSupported(mimeType)) ??
    null
  );
};

const getResponseErrorMessage = async (response: Response) => {
  try {
    const body = (await response.json()) as { error?: string };
    return body.error ?? `Falha HTTP ${response.status}.`;
  } catch {
    return `Falha HTTP ${response.status}.`;
  }
};

export const useVehicleLogging = () => {
  const context = useContext(VehicleLoggingContext);

  if (!context) {
    throw new Error(
      'useVehicleLogging deve ser usado dentro de VehicleLoggingProvider.'
    );
  }

  return context;
};

export const VehicleLoggingProvider = ({
  children,
}: {
  children: ReactNode;
}) => {
  const { config, connectionState, isConnected } = useVehicleConfig();
  const { frames, selectedViews, setForcedViews } = useVehicleVision();
  const [activeSessionId, setActiveSessionId] = useState<string | null>(null);
  const [conversionState, setConversionState] =
    useState<ConversionState>('idle');
  const [lastStopReason, setLastStopReason] = useState<string | null>(null);
  const [recordingStartedAt, setRecordingStartedAt] = useState<number | null>(
    null
  );
  const [recordingState, setRecordingState] =
    useState<RecordingState>('idle');
  const [recordingViews, setRecordingViews] = useState<RecordingViewId[]>([]);
  const [writeError, setWriteError] = useState<string | null>(null);

  const activeSessionIdRef = useRef<string | null>(null);
  const canvasContextsRef = useRef<
    Partial<Record<RecordingViewId, CanvasRenderingContext2D | null>>
  >({});
  const canvasElementsRef = useRef<Partial<Record<RecordingViewId, HTMLCanvasElement>>>(
    {}
  );
  const chunkIndexRef = useRef<Record<RecordingViewId, number>>(
    createRecordingViewMap(() => 0)
  );
  const drawQueuesRef = useRef<Record<RecordingViewId, Promise<void>>>(
    createRecordingViewMap(() => Promise.resolve())
  );
  const flushIntervalRef = useRef<number | null>(null);
  const flushQueueRef = useRef<AppendSessionRequest>(createEmptyAppendPayload());
  const pendingAppendRef = useRef<Promise<void>>(Promise.resolve());
  const recorderStopInFlightRef = useRef(false);
  const recorderRefs = useRef<Partial<Record<RecordingViewId, MediaRecorder>>>({});
  const recordingStartedAtRef = useRef<number | null>(null);
  const sessionErrorsRef = useRef<string[]>([]);
  const stateRef = useRef<RecordingState>('idle');
  const drawFrameOnCanvasRef = useRef<
    (view: RecordingViewId, frame: VisionFrameMessage) => void
  >(() => undefined);
  const finalizeRecordingRef = useRef<
    (stopReason: string, automaticStop: boolean) => Promise<void>
  >(async () => undefined);
  const telemetryCountsRef = useRef<SessionTelemetryCounts>(
    createEmptyTelemetryCounts()
  );
  const uploadQueuesRef = useRef<Record<RecordingViewId, Promise<void>>>(
    createRecordingViewMap(() => Promise.resolve())
  );

  stateRef.current = recordingState;

  const setErrorState = (message: string) => {
    setWriteError(message);

    if (!sessionErrorsRef.current.includes(message)) {
      sessionErrorsRef.current = [...sessionErrorsRef.current, message];
    }
  };

  const mergeAppendPayload = (payload: AppendSessionRequest) => {
    if (!activeSessionIdRef.current) {
      return;
    }

    if (payload.telemetry_raw?.length) {
      flushQueueRef.current.telemetry_raw = [
        ...(flushQueueRef.current.telemetry_raw ?? []),
        ...payload.telemetry_raw,
      ];
    }

    if (payload.road_segmentation_rows?.length) {
      flushQueueRef.current.road_segmentation_rows = [
        ...(flushQueueRef.current.road_segmentation_rows ?? []),
        ...payload.road_segmentation_rows,
      ];
    }

    if (payload.autonomous_control_rows?.length) {
      flushQueueRef.current.autonomous_control_rows = [
        ...(flushQueueRef.current.autonomous_control_rows ?? []),
        ...payload.autonomous_control_rows,
      ];
    }

    if (payload.ui_events?.length) {
      flushQueueRef.current.ui_events = [
        ...(flushQueueRef.current.ui_events ?? []),
        ...payload.ui_events,
      ];
    }

    if (payload.frame_index?.length) {
      flushQueueRef.current.frame_index = [
        ...(flushQueueRef.current.frame_index ?? []),
        ...payload.frame_index,
      ];
    }
  };

  const pushUiEvent = (
    eventType: string,
    payload: Record<string, unknown> | null = null,
    timestampMs = Date.now()
  ) => {
    if (!activeSessionIdRef.current) {
      return;
    }

    telemetryCountsRef.current.ui_events += 1;
    mergeAppendPayload({
      ui_events: [
        {
          event_type: eventType,
          timestamp_ms: timestampMs,
          payload,
        } satisfies UiLogEventRecord,
      ],
    });
  };

  const takeQueuedAppendPayload = () => {
    const currentPayload = flushQueueRef.current;
    flushQueueRef.current = createEmptyAppendPayload();
    return currentPayload;
  };

  const requeueAppendPayload = (payload: AppendSessionRequest) => {
    const currentPayload = flushQueueRef.current;
    flushQueueRef.current = createEmptyAppendPayload();
    mergeAppendPayload(payload);
    mergeAppendPayload(currentPayload);
  };

  const flushPendingData = async () => {
    const sessionId = activeSessionIdRef.current;

    if (!sessionId) {
      return;
    }

    const payload = takeQueuedAppendPayload();

    if (!hasAppendPayloadContent(payload)) {
      return;
    }

    pendingAppendRef.current = pendingAppendRef.current
      .then(async () => {
        const response = await fetch(`/api/logging/session/${sessionId}/append`, {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
          },
          body: JSON.stringify(payload),
        });

        if (!response.ok) {
          throw new Error(await getResponseErrorMessage(response));
        }
      })
      .catch((error) => {
        requeueAppendPayload(payload);
        const message =
          error instanceof Error
            ? error.message
            : 'Falha ao persistir dados da sessao.';
        setErrorState(message);
      });

    await pendingAppendRef.current;
  };

  const resetRecorderState = () => {
    recorderRefs.current = {};
    canvasContextsRef.current = {};
    canvasElementsRef.current = {};
    chunkIndexRef.current = createRecordingViewMap(() => 0);
    drawQueuesRef.current = createRecordingViewMap(() => Promise.resolve());
    uploadQueuesRef.current = createRecordingViewMap(() => Promise.resolve());
  };

  const getCanvas = (view: RecordingViewId) => {
    if (!canvasElementsRef.current[view]) {
      const canvas = document.createElement('canvas');
      canvas.width = DEFAULT_CANVAS_WIDTH;
      canvas.height = DEFAULT_CANVAS_HEIGHT;
      canvasElementsRef.current[view] = canvas;
      canvasContextsRef.current[view] = canvas.getContext('2d');

      const context = canvasContextsRef.current[view];

      if (context) {
        context.fillStyle = '#000000';
        context.fillRect(0, 0, canvas.width, canvas.height);
        context.fillStyle = '#ffffff';
        context.font = '24px sans-serif';
        context.fillText(view, 24, 40);
      }
    }

    return canvasElementsRef.current[view] ?? null;
  };

  const drawFrameOnCanvas = (view: RecordingViewId, frame: VisionFrameMessage) => {
    const canvas = getCanvas(view);

    if (!canvas) {
      return;
    }

    const dataUrl = `data:${frame.mime};base64,${frame.data}`;

    drawQueuesRef.current[view] = drawQueuesRef.current[view]
      .catch(() => undefined)
      .then(async () => {
        const image = new Image();
        image.src = dataUrl;

        try {
          await image.decode();
        } catch {
          await new Promise<void>((resolve) => {
            image.onload = () => resolve();
            image.onerror = () => resolve();
          });
        }

        if (canvas.width !== frame.width || canvas.height !== frame.height) {
          canvas.width = frame.width;
          canvas.height = frame.height;
          canvasContextsRef.current[view] = canvas.getContext('2d');
        }

        const context = canvasContextsRef.current[view];

        if (!context) {
          return;
        }

        context.drawImage(image, 0, 0, canvas.width, canvas.height);
      });
  };

  const uploadVideoChunk = async (
    sessionId: string,
    view: RecordingViewId,
    chunk: Blob
  ) => {
    const chunkIndex = chunkIndexRef.current[view];
    chunkIndexRef.current[view] += 1;

    uploadQueuesRef.current[view] = uploadQueuesRef.current[view]
      .catch(() => undefined)
      .then(async () => {
        const response = await fetch(`/api/logging/session/${sessionId}/video/${view}`, {
          method: 'POST',
          headers: {
            'Content-Type': 'application/octet-stream',
            'x-chunk-index': String(chunkIndex),
          },
          body: await chunk.arrayBuffer(),
        });

        if (!response.ok) {
          throw new Error(await getResponseErrorMessage(response));
        }
      })
      .catch((error) => {
        const message =
          error instanceof Error
            ? error.message
            : `Falha ao enviar chunk de video da view ${view}.`;
        setErrorState(message);
        pushUiEvent('video.chunk_upload_failed', {
          view,
          chunk_index: chunkIndex,
          message,
        });
      });

    await uploadQueuesRef.current[view];
  };

  const startVideoRecorder = (sessionId: string, view: RecordingViewId) => {
    const canvas = getCanvas(view);
    const mimeType = getMediaRecorderMimeType();

    if (!canvas || typeof MediaRecorder === 'undefined') {
      throw new Error('MediaRecorder nao esta disponivel no navegador atual.');
    }

    const existingFrame = frames[view];

    if (existingFrame) {
      drawFrameOnCanvas(view, existingFrame);
    }

    const mediaRecorder = mimeType
      ? new MediaRecorder(canvas.captureStream(15), { mimeType })
      : new MediaRecorder(canvas.captureStream(15));

    mediaRecorder.ondataavailable = (event) => {
      if (event.data.size === 0 || !activeSessionIdRef.current) {
        return;
      }

      void uploadVideoChunk(sessionId, view, event.data);
    };

    mediaRecorder.onerror = () => {
      const message = `Falha no MediaRecorder da view ${view}.`;
      setErrorState(message);
      pushUiEvent('video.recorder_error', { view, message });
    };

    mediaRecorder.start(VIDEO_TIMESLICE_MS);
    recorderRefs.current[view] = mediaRecorder;
  };

  const stopVideoRecorders = async () => {
    const stopPromises = RECORDING_VIEW_IDS.map(
      (view) =>
        new Promise<void>((resolve) => {
          const mediaRecorder = recorderRefs.current[view];

          if (!mediaRecorder || mediaRecorder.state === 'inactive') {
            void uploadQueuesRef.current[view].finally(resolve);
            return;
          }

          mediaRecorder.addEventListener(
            'stop',
            () => {
              void uploadQueuesRef.current[view].finally(resolve);
            },
            { once: true }
          );

          try {
            mediaRecorder.stop();
          } catch {
            void uploadQueuesRef.current[view].finally(resolve);
          }
        })
    );

    await Promise.all(stopPromises);
  };

  const clearFlushLoop = () => {
    if (flushIntervalRef.current !== null) {
      window.clearInterval(flushIntervalRef.current);
      flushIntervalRef.current = null;
    }
  };

  const startFlushLoop = () => {
    clearFlushLoop();
    flushIntervalRef.current = window.setInterval(() => {
      void flushPendingData();
    }, FLUSH_INTERVAL_MS);
  };

  const finalizeRecording = async (
    stopReason: string,
    automaticStop: boolean
  ) => {
    const sessionId = activeSessionIdRef.current;

    if (!sessionId || recorderStopInFlightRef.current) {
      return;
    }

    recorderStopInFlightRef.current = true;
    stateRef.current = 'stopping';
    setRecordingState('stopping');
    setLastStopReason(stopReason);
    pushUiEvent('recording.stopped', {
      automatic_stop: automaticStop,
      stop_reason: stopReason,
    });

    try {
      clearFlushLoop();
      await stopVideoRecorders();
      await flushPendingData();
      await pendingAppendRef.current;
      setForcedViews([]);
      setConversionState('converting');

      const finishPayload: SessionFinishRequest = {
        ended_at_ms: Date.now(),
        stop_reason: stopReason,
        automatic_stop: automaticStop,
        telemetry_counts: telemetryCountsRef.current,
        video_outputs: createRecordingViewMap((view) => `videos/${view}.mp4`),
        errors: sessionErrorsRef.current,
      };

      activeSessionIdRef.current = null;

      const response = await fetch(`/api/logging/session/${sessionId}/finish`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify(finishPayload),
      });

      if (!response.ok) {
        throw new Error(await getResponseErrorMessage(response));
      }

      const sessionSummary = (await response.json()) as {
        conversion_status?: {
          overall?: 'completed' | 'partial' | 'failed';
        };
      };

      setConversionState(
        sessionSummary.conversion_status?.overall === 'completed'
          ? 'completed'
          : 'failed'
      );
      stateRef.current = 'idle';
      setRecordingState('idle');
      setActiveSessionId(null);
      setRecordingStartedAt(null);
      setRecordingViews([]);
    } catch (error) {
      const message =
        error instanceof Error ? error.message : 'Falha ao finalizar gravacao.';
      setErrorState(message);
      setConversionState('failed');
      stateRef.current = 'error';
      setRecordingState('error');
      setActiveSessionId(null);
      setRecordingStartedAt(null);
      setRecordingViews([]);
      setForcedViews([]);
      activeSessionIdRef.current = null;
    } finally {
      recorderStopInFlightRef.current = false;
      resetRecorderState();
    }
  };

  const startRecording = async () => {
    if (recordingState === 'starting' || recordingState === 'stopping') {
      return;
    }

    if (!isConnected) {
      stateRef.current = 'error';
      setRecordingState('error');
      setErrorState(
        `Nao foi possivel iniciar a gravacao com o controle ${connectionState}.`
      );
      return;
    }

    if (!config.connectionUrl.trim()) {
      stateRef.current = 'error';
      setRecordingState('error');
      setErrorState('Defina uma URL WebSocket valida antes de iniciar a gravacao.');
      return;
    }

    if (typeof window === 'undefined' || typeof MediaRecorder === 'undefined') {
      stateRef.current = 'error';
      setRecordingState('error');
      setErrorState('O navegador atual nao suporta MediaRecorder para gravacao.');
      return;
    }

    stateRef.current = 'starting';
    setRecordingState('starting');
    setWriteError(null);
    setLastStopReason(null);
    setConversionState('idle');
    telemetryCountsRef.current = createEmptyTelemetryCounts();
    sessionErrorsRef.current = [];
    flushQueueRef.current = createEmptyAppendPayload();
    pendingAppendRef.current = Promise.resolve();
    resetRecorderState();

    try {
      const payload: SessionStartRequest = {
        ws_url: config.connectionUrl.trim(),
        recorded_views: [...RECORDING_VIEW_IDS],
        user_selected_views_at_start: selectedViews,
      };
      const response = await fetch('/api/logging/session/start', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify(payload),
      });

      if (!response.ok) {
        throw new Error(await getResponseErrorMessage(response));
      }

      const session = (await response.json()) as SessionStartResponse;
      const startedAtMs = Date.now();

      activeSessionIdRef.current = session.session_id;
      recordingStartedAtRef.current = startedAtMs;
      setActiveSessionId(session.session_id);
      setRecordingStartedAt(startedAtMs);
      setRecordingViews([...RECORDING_VIEW_IDS]);
      setForcedViews([...RECORDING_VIEW_IDS]);

      for (const view of RECORDING_VIEW_IDS) {
        startVideoRecorder(session.session_id, view);
      }

      pushUiEvent('recording.started', {
        session_id: session.session_id,
        ws_url: config.connectionUrl.trim(),
        selected_views_at_start: selectedViews,
      });
      startFlushLoop();
      stateRef.current = 'recording';
      setRecordingState('recording');
    } catch (error) {
      const message =
        error instanceof Error ? error.message : 'Falha ao iniciar a gravacao.';
      await stopVideoRecorders();
      clearFlushLoop();
      stateRef.current = 'error';
      setRecordingState('error');
      setErrorState(message);
      setForcedViews([]);
      activeSessionIdRef.current = null;
      setActiveSessionId(null);
      setRecordingStartedAt(null);
      setRecordingViews([]);
      resetRecorderState();
    }
  };

  const toggleRecording = async () => {
    if (recordingState === 'recording') {
      await finalizeRecording('manual_toggle', false);
      return;
    }

    if (recordingState === 'idle' || recordingState === 'error') {
      await startRecording();
    }
  };

  drawFrameOnCanvasRef.current = drawFrameOnCanvas;
  finalizeRecordingRef.current = finalizeRecording;

  useEffect(() => {
    const handleTelemetryReceived = (event: Event) => {
      const detail = (event as CustomEvent<TelemetryReceivedDetail>).detail;

      if (!activeSessionIdRef.current) {
        return;
      }

      telemetryCountsRef.current.telemetry_raw += 1;
      mergeAppendPayload({
        telemetry_raw: [
          {
            received_at_ms: detail.receivedAtMs,
            type: detail.message.type,
            payload: detail.message,
          },
        ],
      });

      if (detail.message.type === 'telemetry.road_segmentation') {
        telemetryCountsRef.current.road_segmentation += 1;
        mergeAppendPayload({
          road_segmentation_rows: [
            flattenRoadSegmentationTelemetry(detail.message, detail.receivedAtMs),
          ],
        });
        return;
      }

      if (detail.message.type === 'telemetry.traffic_sign_detection') {
        return;
      }

      telemetryCountsRef.current.autonomous_control += 1;
      mergeAppendPayload({
        autonomous_control_rows: [
          flattenAutonomousControlTelemetry(detail.message, detail.receivedAtMs),
        ],
      });

      if (stateRef.current !== 'recording') {
        return;
      }

      if (
        detail.message.stop_reason === 'lane_lost' ||
        detail.message.stop_reason === 'low_confidence'
      ) {
        void finalizeRecordingRef.current(detail.message.stop_reason, true);
        return;
      }

      if (detail.message.tracking_state === 'fail_safe') {
        void finalizeRecordingRef.current('fail_safe', true);
      }
    };

    const handleUiLogEvent = (event: Event) => {
      const detail = (event as CustomEvent<UiLogEventDetail>).detail;

      if (!activeSessionIdRef.current) {
        return;
      }

      telemetryCountsRef.current.ui_events += 1;
      mergeAppendPayload({
        ui_events: [
          {
            event_type: detail.eventType,
            timestamp_ms: detail.timestampMs,
            payload: detail.payload,
          },
        ],
      });
    };

    const handleVisionFrameReceived = (event: Event) => {
      const detail = (event as CustomEvent<VisionFrameReceivedDetail>).detail;

      if (!activeSessionIdRef.current) {
        return;
      }

      telemetryCountsRef.current.frame_index += 1;
      mergeAppendPayload({
        frame_index: [
          {
            view: detail.frame.view,
            timestamp_ms: detail.frame.timestamp_ms,
            received_at_ms: detail.receivedAtMs,
            width: detail.frame.width,
            height: detail.frame.height,
          },
        ],
      });
      drawFrameOnCanvasRef.current(detail.frame.view, detail.frame);
    };

    window.addEventListener(
      TELEMETRY_RECEIVED_EVENT,
      handleTelemetryReceived as EventListener
    );
    window.addEventListener(UI_LOG_EVENT, handleUiLogEvent as EventListener);
    window.addEventListener(
      VISION_FRAME_RECEIVED_EVENT,
      handleVisionFrameReceived as EventListener
    );

    return () => {
      window.removeEventListener(
        TELEMETRY_RECEIVED_EVENT,
        handleTelemetryReceived as EventListener
      );
      window.removeEventListener(UI_LOG_EVENT, handleUiLogEvent as EventListener);
      window.removeEventListener(
        VISION_FRAME_RECEIVED_EVENT,
        handleVisionFrameReceived as EventListener
      );
      clearFlushLoop();
    };
  }, []);

  return (
    <VehicleLoggingContext.Provider
      value={{
        activeSessionId,
        conversionState,
        lastStopReason,
        recordingStartedAt,
        recordingState,
        recordingViews,
        toggleRecording,
        writeError,
      }}
    >
      {children}
    </VehicleLoggingContext.Provider>
  );
};
