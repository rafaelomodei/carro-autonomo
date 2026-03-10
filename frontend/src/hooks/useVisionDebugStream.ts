'use client';

import { useEffect, useRef, useState } from 'react';
import {
  buildClientRoleMessage,
  buildVisionStreamSubscriptionMessage,
  ConnectionState,
  parseVisionFrameMessage,
  RECONNECT_DELAYS_MS,
  TELEMETRY_STALE_THRESHOLD_MS,
  VisionDebugViewId,
  VisionFrameMessage,
} from '@/lib/autonomous-car';
import { isValidWebSocketUrl } from '@/lib/websocket';

interface UseVisionDebugStreamResult {
  connectionState: ConnectionState;
  frames: Partial<Record<VisionDebugViewId, VisionFrameMessage>>;
  isConnected: boolean;
  lastFrameAt: number | null;
}

export const useVisionDebugStream = (
  connectionUrl: string,
  selectedViews: VisionDebugViewId[]
): UseVisionDebugStreamResult => {
  const [connectionState, setConnectionState] =
    useState<ConnectionState>('disconnected');
  const [frames, setFrames] = useState<
    Partial<Record<VisionDebugViewId, VisionFrameMessage>>
  >({});
  const [lastFrameAt, setLastFrameAt] = useState<number | null>(null);

  const normalizedUrl = connectionUrl.trim();
  const subscriptionMessage =
    buildVisionStreamSubscriptionMessage(selectedViews);
  const hasRequestedViews = selectedViews.length > 0;
  const isConnected =
    connectionState === 'connected' || connectionState === 'telemetry_stale';

  const socketRef = useRef<WebSocket | null>(null);
  const reconnectTimerRef = useRef<number | null>(null);
  const reconnectAttemptRef = useRef(0);
  const subscriptionMessageRef = useRef(subscriptionMessage);

  subscriptionMessageRef.current = subscriptionMessage;

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

        setFrames((currentFrames) => ({
          ...currentFrames,
          [parsedFrame.view]: parsedFrame,
        }));
        setLastFrameAt(Date.now());
        setConnectionState('connected');
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

  return {
    connectionState,
    frames,
    isConnected,
    lastFrameAt,
  };
};
