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
  applyRuntimeSetting,
  AutonomousControlTelemetry,
  buildAutonomousCommandMessage,
  buildClientRoleMessage,
  buildDrivingModeMessage,
  buildManualMotionMessage,
  buildManualSteeringMessage,
  buildRuntimeSettingMessage,
  ConnectionState,
  DEFAULT_VEHICLE_RUNTIME_CONFIG,
  DrivingMode,
  MANUAL_COMMAND_INTERVAL_MS,
  ManualMotionCommand,
  ManualSteeringCommand,
  parseTelemetryMessage,
  RECONNECT_DELAYS_MS,
  RoadSegmentationTelemetry,
  RuntimeSettingKey,
  TELEMETRY_STALE_THRESHOLD_MS,
  VehicleRuntimeConfig,
  VEHICLE_RUNTIME_STORAGE_KEY,
  normalizeVehicleRuntimeConfig,
} from '@/lib/autonomous-car';
import { emitTelemetryReceived, emitUiLogEvent } from '@/lib/vehicle-events';
import { isValidWebSocketUrl } from '@/lib/websocket';

interface VehicleConfigContextProps {
  config: VehicleRuntimeConfig;
  connectionState: ConnectionState;
  isConnected: boolean;
  lastTelemetryAt: number | null;
  effectiveDrivingMode: DrivingMode;
  pendingDrivingMode: DrivingMode | null;
  pendingAutonomousCommand: 'start' | 'stop' | null;
  roadSegmentationTelemetry: RoadSegmentationTelemetry | null;
  autonomousControlTelemetry: AutonomousControlTelemetry | null;
  canSendManualCommands: boolean;
  saveConnectionUrl: (url: string) => void;
  setDrivingMode: (mode: DrivingMode) => void;
  toggleDrivingMode: () => void;
  startAutonomous: () => void;
  stopAutonomous: () => void;
  sendManualMotion: (command: ManualMotionCommand) => boolean;
  sendManualSteering: (command: ManualSteeringCommand) => boolean;
  beginManualMotionHold: (
    direction: Exclude<ManualMotionCommand, 'stop'>
  ) => void;
  endManualMotionHold: () => void;
  beginManualSteeringHold: (
    direction: Exclude<ManualSteeringCommand, 'center'>
  ) => void;
  endManualSteeringHold: () => void;
  stopManualControl: () => void;
  updateRuntimeSetting: (key: RuntimeSettingKey, value: number) => void;
}

const VehicleConfigContext = createContext<VehicleConfigContextProps | null>(null);

export const useVehicleConfig = () => {
  const context = useContext(VehicleConfigContext);

  if (!context) {
    throw new Error(
      'useVehicleConfig deve ser usado dentro de VehicleConfigProvider.'
    );
  }

  return context;
};

export const VehicleConfigProvider = ({
  children,
}: {
  children: ReactNode;
}) => {
  const [config, setConfig] = useState<VehicleRuntimeConfig>(
    DEFAULT_VEHICLE_RUNTIME_CONFIG
  );
  const [connectionState, setConnectionState] =
    useState<ConnectionState>('disconnected');
  const [lastTelemetryAt, setLastTelemetryAt] = useState<number | null>(null);
  const [pendingDrivingMode, setPendingDrivingMode] =
    useState<DrivingMode | null>(null);
  const [pendingAutonomousCommand, setPendingAutonomousCommand] = useState<
    'start' | 'stop' | null
  >(null);
  const [roadSegmentationTelemetry, setRoadSegmentationTelemetry] =
    useState<RoadSegmentationTelemetry | null>(null);
  const [autonomousControlTelemetry, setAutonomousControlTelemetry] =
    useState<AutonomousControlTelemetry | null>(null);
  const [isHydrated, setIsHydrated] = useState(false);

  const socketRef = useRef<WebSocket | null>(null);
  const reconnectTimerRef = useRef<number | null>(null);
  const reconnectAttemptRef = useRef(0);
  const motionLoopRef = useRef<number | null>(null);
  const steeringLoopRef = useRef<number | null>(null);
  const activeMotionRef = useRef<ManualMotionCommand | null>(null);
  const activeSteeringRef = useRef<ManualSteeringCommand | null>(null);
  const configRef = useRef(config);
  const pendingDrivingModeRef = useRef<DrivingMode | null>(pendingDrivingMode);
  const pendingAutonomousCommandRef = useRef<'start' | 'stop' | null>(
    pendingAutonomousCommand
  );
  const manualControlAllowedRef = useRef(false);
  const sendTextMessageRef = useRef<(message: string) => boolean>(() => false);
  const clearMotionLoopRef = useRef<(dispatchStop: boolean) => void>(
    () => undefined
  );
  const clearSteeringLoopRef = useRef<(dispatchCenter: boolean) => void>(
    () => undefined
  );
  const clearActiveManualLoopsRef = useRef<(dispatchReset: boolean) => void>(
    () => undefined
  );
  const startMotionLoopRef = useRef<
    (direction: Exclude<ManualMotionCommand, 'stop'>) => void
  >(() => undefined);
  const startSteeringLoopRef = useRef<
    (direction: Exclude<ManualSteeringCommand, 'center'>) => void
  >(() => undefined);

  configRef.current = config;
  pendingDrivingModeRef.current = pendingDrivingMode;
  pendingAutonomousCommandRef.current = pendingAutonomousCommand;

  const isConnected =
    connectionState === 'connected' || connectionState === 'telemetry_stale';
  const effectiveDrivingMode =
    pendingDrivingMode === 'autonomous'
      ? 'autonomous'
      : autonomousControlTelemetry?.driving_mode ?? pendingDrivingMode ?? 'manual';
  const canSendManualCommands =
    isConnected && effectiveDrivingMode === 'manual';

  manualControlAllowedRef.current = canSendManualCommands;

  sendTextMessageRef.current = (message: string) => {
    const socket = socketRef.current;

    if (!socket || socket.readyState !== WebSocket.OPEN) {
      return false;
    }

    socket.send(message);
    return true;
  };

  clearMotionLoopRef.current = (dispatchStop: boolean) => {
    if (motionLoopRef.current !== null) {
      window.clearInterval(motionLoopRef.current);
      motionLoopRef.current = null;
    }

    activeMotionRef.current = null;

    if (dispatchStop) {
      sendTextMessageRef.current(buildManualMotionMessage('stop'));
    }
  };

  clearSteeringLoopRef.current = (dispatchCenter: boolean) => {
    if (steeringLoopRef.current !== null) {
      window.clearInterval(steeringLoopRef.current);
      steeringLoopRef.current = null;
    }

    activeSteeringRef.current = null;

    if (dispatchCenter) {
      sendTextMessageRef.current(buildManualSteeringMessage('center'));
    }
  };

  clearActiveManualLoopsRef.current = (dispatchReset: boolean) => {
    clearMotionLoopRef.current(dispatchReset);
    clearSteeringLoopRef.current(dispatchReset);
  };

  startMotionLoopRef.current = (
    direction: Exclude<ManualMotionCommand, 'stop'>
  ) => {
    if (!manualControlAllowedRef.current) {
      return;
    }

    if (
      activeMotionRef.current === direction &&
      motionLoopRef.current !== null
    ) {
      return;
    }

    clearMotionLoopRef.current(false);

    if (!sendTextMessageRef.current(buildManualMotionMessage(direction))) {
      return;
    }

    activeMotionRef.current = direction;
    motionLoopRef.current = window.setInterval(() => {
      if (!manualControlAllowedRef.current) {
        clearMotionLoopRef.current(false);
        return;
      }

      sendTextMessageRef.current(buildManualMotionMessage(direction));
    }, MANUAL_COMMAND_INTERVAL_MS);
  };

  startSteeringLoopRef.current = (
    direction: Exclude<ManualSteeringCommand, 'center'>
  ) => {
    if (!manualControlAllowedRef.current) {
      return;
    }

    if (
      activeSteeringRef.current === direction &&
      steeringLoopRef.current !== null
    ) {
      return;
    }

    clearSteeringLoopRef.current(false);

    if (!sendTextMessageRef.current(buildManualSteeringMessage(direction))) {
      return;
    }

    activeSteeringRef.current = direction;
    steeringLoopRef.current = window.setInterval(() => {
      if (!manualControlAllowedRef.current) {
        clearSteeringLoopRef.current(false);
        return;
      }

      sendTextMessageRef.current(buildManualSteeringMessage(direction));
    }, MANUAL_COMMAND_INTERVAL_MS);
  };

  useEffect(() => {
    if (typeof window === 'undefined') {
      return;
    }

    try {
      const storedConfig = window.localStorage.getItem(
        VEHICLE_RUNTIME_STORAGE_KEY
      );

      if (storedConfig) {
        const parsedConfig = JSON.parse(storedConfig) as Partial<VehicleRuntimeConfig>;
        setConfig(normalizeVehicleRuntimeConfig(parsedConfig));
      }
    } catch (error) {
      console.warn('Falha ao carregar configuracoes salvas do veiculo.', error);
    } finally {
      setIsHydrated(true);
    }
  }, []);

  useEffect(() => {
    if (!isHydrated || typeof window === 'undefined') {
      return;
    }

    window.localStorage.setItem(
      VEHICLE_RUNTIME_STORAGE_KEY,
      JSON.stringify(config)
    );
  }, [config, isHydrated]);

  useEffect(() => {
    if (!canSendManualCommands) {
      clearActiveManualLoopsRef.current(false);
    }
  }, [canSendManualCommands]);

  useEffect(() => {
    if (!isHydrated) {
      return;
    }

    const savedUrl = config.connectionUrl.trim();

    if (!savedUrl || !isValidWebSocketUrl(savedUrl)) {
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

    const syncRuntimeSettings = (socket: WebSocket) => {
      const currentConfig = configRef.current;

      socket.send(
        buildRuntimeSettingMessage(
          'steering.sensitivity',
          currentConfig.steeringSensitivity
        )
      );
      socket.send(
        buildRuntimeSettingMessage(
          'steering.command_step',
          currentConfig.steeringCommandStep
        )
      );
      socket.send(
        buildRuntimeSettingMessage(
          'autonomous.pid.kp',
          currentConfig.pidControl.p
        )
      );
      socket.send(
        buildRuntimeSettingMessage(
          'autonomous.pid.ki',
          currentConfig.pidControl.i
        )
      );
      socket.send(
        buildRuntimeSettingMessage(
          'autonomous.pid.kd',
          currentConfig.pidControl.d
        )
      );
    };

    const connect = () => {
      if (cancelled) {
        return;
      }

      setConnectionState(
        reconnectAttemptRef.current === 0 ? 'connecting' : 'reconnecting'
      );

      const socket = new WebSocket(savedUrl);
      socketRef.current = socket;

      socket.onopen = () => {
        if (cancelled) {
          socket.close();
          return;
        }

        reconnectAttemptRef.current = 0;
        setConnectionState('connected');
        socket.send(buildClientRoleMessage('control'));
        syncRuntimeSettings(socket);
      };

      socket.onmessage = (event) => {
        if (cancelled || typeof event.data !== 'string') {
          return;
        }

        const parsedMessage = parseTelemetryMessage(event.data);

        if (!parsedMessage) {
          return;
        }

        const receivedAtMs = Date.now();

        setLastTelemetryAt(receivedAtMs);
        setConnectionState('connected');
        emitTelemetryReceived({
          message: parsedMessage,
          receivedAtMs,
        });

        if (parsedMessage.type === 'telemetry.road_segmentation') {
          setRoadSegmentationTelemetry(parsedMessage);
          return;
        }

        setAutonomousControlTelemetry(parsedMessage);

        if (pendingDrivingModeRef.current === parsedMessage.driving_mode) {
          setPendingDrivingMode(null);
        }

        if (
          pendingAutonomousCommandRef.current === 'start' &&
          parsedMessage.autonomous_started
        ) {
          setPendingAutonomousCommand(null);
        }

        if (
          pendingAutonomousCommandRef.current === 'stop' &&
          !parsedMessage.autonomous_started
        ) {
          setPendingAutonomousCommand(null);
        }

        if (parsedMessage.driving_mode === 'manual') {
          setPendingAutonomousCommand(null);
        }
      };

      socket.onerror = (error) => {
        console.error('Erro no WebSocket do veiculo:', error);
      };

      socket.onclose = () => {
        if (socketRef.current === socket) {
          socketRef.current = null;
        }

        clearActiveManualLoopsRef.current(false);

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

      clearActiveManualLoopsRef.current(false);

      if (socketRef.current) {
        const activeSocket = socketRef.current;
        socketRef.current = null;
        activeSocket.close();
      }
    };
  }, [config.connectionUrl, isHydrated]);

  useEffect(() => {
    if (!isHydrated) {
      return;
    }

    const telemetryHealthInterval = window.setInterval(() => {
      const socket = socketRef.current;

      if (!socket || socket.readyState !== WebSocket.OPEN) {
        return;
      }

      setConnectionState((currentState) => {
        if (
          lastTelemetryAt &&
          Date.now() - lastTelemetryAt > TELEMETRY_STALE_THRESHOLD_MS
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
      window.clearInterval(telemetryHealthInterval);
    };
  }, [isHydrated, lastTelemetryAt]);

  const saveConnectionUrl = (url: string) => {
    const normalizedUrl = url.trim();

    setConfig((currentConfig) => ({
      ...currentConfig,
      connectionUrl: normalizedUrl,
    }));
  };

  const updateRuntimeSetting = (key: RuntimeSettingKey, value: number) => {
    if (!Number.isFinite(value)) {
      return;
    }

    setConfig((currentConfig) => applyRuntimeSetting(currentConfig, key, value));
    sendTextMessageRef.current(buildRuntimeSettingMessage(key, value));
  };

  const setDrivingMode = (mode: DrivingMode) => {
    clearActiveManualLoopsRef.current(false);

    if (sendTextMessageRef.current(buildDrivingModeMessage(mode))) {
      emitUiLogEvent('control.driving_mode_requested', {
        mode,
      });
      setPendingDrivingMode(mode);

      if (mode === 'manual') {
        setPendingAutonomousCommand(null);
      }
    }
  };

  const toggleDrivingMode = () => {
    setDrivingMode(effectiveDrivingMode === 'manual' ? 'autonomous' : 'manual');
  };

  const startAutonomous = () => {
    clearActiveManualLoopsRef.current(false);

    if (sendTextMessageRef.current(buildAutonomousCommandMessage('start'))) {
      emitUiLogEvent('control.autonomous_start_requested', null);
      setPendingAutonomousCommand('start');
    }
  };

  const stopAutonomous = () => {
    if (sendTextMessageRef.current(buildAutonomousCommandMessage('stop'))) {
      emitUiLogEvent('control.autonomous_stop_requested', null);
      setPendingAutonomousCommand('stop');
    }
  };

  const sendManualMotion = (command: ManualMotionCommand) => {
    if (!manualControlAllowedRef.current) {
      return false;
    }

    return sendTextMessageRef.current(buildManualMotionMessage(command));
  };

  const sendManualSteering = (command: ManualSteeringCommand) => {
    if (!manualControlAllowedRef.current) {
      return false;
    }

    return sendTextMessageRef.current(buildManualSteeringMessage(command));
  };

  useEffect(() => {
    emitUiLogEvent('control.connection_state_changed', {
      state: connectionState,
    });
  }, [connectionState]);

  return (
    <VehicleConfigContext.Provider
      value={{
        config,
        connectionState,
        isConnected,
        lastTelemetryAt,
        effectiveDrivingMode,
        pendingDrivingMode,
        pendingAutonomousCommand,
        roadSegmentationTelemetry,
        autonomousControlTelemetry,
        canSendManualCommands,
        saveConnectionUrl,
        setDrivingMode,
        toggleDrivingMode,
        startAutonomous,
        stopAutonomous,
        sendManualMotion,
        sendManualSteering,
        beginManualMotionHold: startMotionLoopRef.current,
        endManualMotionHold: () =>
          clearMotionLoopRef.current(manualControlAllowedRef.current),
        beginManualSteeringHold: startSteeringLoopRef.current,
        endManualSteeringHold: () =>
          clearSteeringLoopRef.current(manualControlAllowedRef.current),
        stopManualControl: () =>
          clearActiveManualLoopsRef.current(manualControlAllowedRef.current),
        updateRuntimeSetting,
      }}
    >
      {children}
    </VehicleConfigContext.Provider>
  );
};
