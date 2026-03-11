'use client';

import {
  createContext,
  ReactNode,
  useContext,
  useEffect,
  useRef,
  useState,
} from 'react';
import {
  buildClientRoleMessage,
  buildVisionStreamSubscriptionMessage,
  ConnectionState,
  parseVisionFrameMessage,
  RECONNECT_DELAYS_MS,
  TELEMETRY_STALE_THRESHOLD_MS,
  VisionDebugViewId,
  VisionFrameMessage,
  VISION_DEBUG_VIEW_IDS,
} from '@/lib/autonomous-car';
import { emitUiLogEvent, emitVisionFrameReceived } from '@/lib/vehicle-events';
import { isValidWebSocketUrl } from '@/lib/websocket';
import { useVehicleConfig } from '@/providers/VehicleConfigProvider';

interface VehicleVisionContextProps {
  connectionState: ConnectionState;
  frames: Partial<Record<VisionDebugViewId, VisionFrameMessage>>;
  forcedViews: VisionDebugViewId[];
  isConnected: boolean;
  lastFrameAt: number | null;
  requestedViews: VisionDebugViewId[];
  selectedViews: VisionDebugViewId[];
  setForcedViews: (views: VisionDebugViewId[]) => void;
  setSelectedViews: (views: VisionDebugViewId[]) => void;
}

const VehicleVisionContext = createContext<VehicleVisionContextProps | null>(null);

export const useVehicleVision = () => {
  const context = useContext(VehicleVisionContext);

  if (!context) {
    throw new Error(
      'useVehicleVision deve ser usado dentro de VehicleVisionProvider.'
    );
  }

  return context;
};

export const VehicleVisionProvider = ({
  children,
}: {
  children: ReactNode;
}) => {
  const { config } = useVehicleConfig();
  const [connectionState, setConnectionState] =
    useState<ConnectionState>('disconnected');
  const [frames, setFrames] = useState<
    Partial<Record<VisionDebugViewId, VisionFrameMessage>>
  >({});
  const [forcedViews, setForcedViews] = useState<VisionDebugViewId[]>([]);
  const [lastFrameAt, setLastFrameAt] = useState<number | null>(null);
  const [selectedViews, setSelectedViews] = useState<VisionDebugViewId[]>([]);

  const socketRef = useRef<WebSocket | null>(null);
  const reconnectTimerRef = useRef<number | null>(null);
  const reconnectAttemptRef = useRef(0);
  const previousConnectionStateRef = useRef<ConnectionState>('disconnected');
  const subscriptionMessageRef = useRef('');

  const normalizedUrl = config.connectionUrl.trim();
  const requestedViews = VISION_DEBUG_VIEW_IDS.filter(
    (view) => selectedViews.includes(view) || forcedViews.includes(view)
  );
  const hasRequestedViews = requestedViews.length > 0;
  const subscriptionMessage =
    buildVisionStreamSubscriptionMessage(requestedViews);
  const isConnected =
    connectionState === 'connected' || connectionState === 'telemetry_stale';

  subscriptionMessageRef.current = subscriptionMessage;

  useEffect(() => {
    if (previousConnectionStateRef.current === connectionState) {
      return;
    }

    emitUiLogEvent('vision.connection_state_changed', {
      next_state: connectionState,
      previous_state: previousConnectionStateRef.current,
    });
    previousConnectionStateRef.current = connectionState;
  }, [connectionState]);

  useEffect(() => {
    if (!hasRequestedViews || !isValidWebSocketUrl(normalizedUrl)) {
      reconnectAttemptRef.current = 0;

      if (socketRef.current) {
        socketRef.current.close();
        socketRef.current = null;
      }

      if (reconnectTimerRef.current !== null) {
        window.clearTimeout(reconnectTimerRef.current);
        reconnectTimerRef.current = null;
      }

      setConnectionState('disconnected');
      return;
    }

    let cancelled = false;

    const connect = () => {
      if (cancelled) {
        return;
      }

      setConnectionState(
        reconnectAttemptRef.current === 0 ? 'connecting' : 'reconnecting'
      );

      const socket = new WebSocket(normalizedUrl);
      socketRef.current = socket;

      socket.onopen = () => {
        if (cancelled) {
          socket.close();
          return;
        }

        reconnectAttemptRef.current = 0;
        setConnectionState('connected');
        socket.send(buildClientRoleMessage('telemetry'));
        socket.send(subscriptionMessageRef.current);
      };

      socket.onmessage = (event) => {
        if (cancelled || typeof event.data !== 'string') {
          return;
        }

        const parsedFrame = parseVisionFrameMessage(event.data);

        if (!parsedFrame) {
          return;
        }

        const receivedAtMs = Date.now();

        setFrames((currentFrames) => ({
          ...currentFrames,
          [parsedFrame.view]: parsedFrame,
        }));
        setLastFrameAt(receivedAtMs);
        setConnectionState('connected');
        emitVisionFrameReceived({
          frame: parsedFrame,
          receivedAtMs,
        });
      };

      socket.onerror = (error) => {
        console.error('Erro no WebSocket de stream de visao:', error);
      };

      socket.onclose = () => {
        if (socketRef.current === socket) {
          socketRef.current = null;
        }

        if (cancelled) {
          return;
        }

        const nextDelay =
          RECONNECT_DELAYS_MS[
            Math.min(reconnectAttemptRef.current, RECONNECT_DELAYS_MS.length - 1)
          ];

        reconnectAttemptRef.current += 1;
        setConnectionState('reconnecting');
        reconnectTimerRef.current = window.setTimeout(() => {
          reconnectTimerRef.current = null;
          connect();
        }, nextDelay);
      };
    };

    connect();

    return () => {
      cancelled = true;
      reconnectAttemptRef.current = 0;

      if (reconnectTimerRef.current !== null) {
        window.clearTimeout(reconnectTimerRef.current);
        reconnectTimerRef.current = null;
      }

      if (socketRef.current) {
        const activeSocket = socketRef.current;
        socketRef.current = null;
        activeSocket.close();
      }
    };
  }, [hasRequestedViews, normalizedUrl]);

  useEffect(() => {
    const socket = socketRef.current;

    if (!socket || socket.readyState !== WebSocket.OPEN) {
      return;
    }

    socket.send(subscriptionMessage);
  }, [subscriptionMessage]);

  useEffect(() => {
    if (!hasRequestedViews) {
      return;
    }

    const healthCheckInterval = window.setInterval(() => {
      const socket = socketRef.current;

      if (!socket || socket.readyState !== WebSocket.OPEN) {
        return;
      }

      setConnectionState((currentState) => {
        if (
          lastFrameAt &&
          Date.now() - lastFrameAt > TELEMETRY_STALE_THRESHOLD_MS
        ) {
          return 'telemetry_stale';
        }

        if (currentState === 'telemetry_stale') {
          return 'connected';
        }

        return currentState;
      });
    }, 1000);

    return () => {
      window.clearInterval(healthCheckInterval);
    };
  }, [hasRequestedViews, lastFrameAt]);

  return (
    <VehicleVisionContext.Provider
      value={{
        connectionState,
        frames,
        forcedViews,
        isConnected,
        lastFrameAt,
        requestedViews,
        selectedViews,
        setForcedViews,
        setSelectedViews,
      }}
    >
      {children}
    </VehicleVisionContext.Provider>
  );
};
