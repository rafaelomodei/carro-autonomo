import { Badge } from '@/components/ui/badge';
import { useCurrentTime } from '@/hooks/useCurrentTime';
import {
  formatConnectionStateLabel,
  formatDrivingModeLabel,
} from '@/lib/autonomous-car';
import { useVehicleConfig } from '@/providers/VehicleConfigProvider';
import { useVehicleLogging } from '@/providers/VehicleLoggingProvider';
import { Activity, Clock, Wifi, WifiOff } from 'lucide-react';

const formatElapsedTime = (startTimestamp: number | null) => {
  if (!startTimestamp) {
    return '00:00';
  }

  const totalSeconds = Math.max(0, Math.floor((Date.now() - startTimestamp) / 1000));
  const minutes = Math.floor(totalSeconds / 60)
    .toString()
    .padStart(2, '0');
  const seconds = (totalSeconds % 60).toString().padStart(2, '0');

  return `${minutes}:${seconds}`;
};

const StatusBar = () => {
  const currentTime = useCurrentTime();
  const {
    autonomousControlTelemetry,
    connectionState,
    isConnected,
    lastTelemetryAt,
    pendingDrivingMode,
  } = useVehicleConfig();
  const {
    activeSessionId,
    conversionState,
    lastStopReason,
    recordingStartedAt,
    recordingState,
    writeError,
  } = useVehicleLogging();

  const currentDrivingMode =
    autonomousControlTelemetry?.driving_mode ?? pendingDrivingMode ?? 'manual';
  const lastTelemetryLabel = lastTelemetryAt
    ? new Date(lastTelemetryAt).toLocaleTimeString('pt-BR')
    : 'Aguardando';
  const recordingElapsedLabel = formatElapsedTime(recordingStartedAt);

  return (
    <div className='flex w-full flex-wrap items-center justify-end gap-4 text-xs'>
      {recordingState === 'recording' && activeSessionId && (
        <div className='flex items-center gap-2'>
          <Badge variant='destructive'>
            Gravando {activeSessionId.slice(0, 8)} · {recordingElapsedLabel}
          </Badge>
        </div>
      )}

      {recordingState === 'starting' && (
        <div className='flex items-center gap-2'>
          <Badge variant='secondary'>Preparando gravacao</Badge>
        </div>
      )}

      {recordingState === 'stopping' && (
        <div className='flex items-center gap-2'>
          <Badge variant='secondary'>Fechando sessao</Badge>
        </div>
      )}

      {conversionState === 'converting' && (
        <div className='flex items-center gap-2'>
          <Badge variant='secondary'>Exportando MP4</Badge>
        </div>
      )}

      {lastStopReason && recordingState === 'idle' && (
        <div className='flex items-center gap-2'>
          <Badge variant='outline'>Ultimo log: {lastStopReason}</Badge>
        </div>
      )}

      {writeError && (
        <div className='flex items-center gap-2'>
          <Badge variant='destructive'>Erro no logger</Badge>
        </div>
      )}

      <div className='flex items-center gap-2'>
        {isConnected ? (
          <Wifi className='h-4 w-4' />
        ) : (
          <WifiOff className='h-4 w-4' />
        )}
        <span>{formatConnectionStateLabel(connectionState)}</span>
      </div>

      <div className='flex items-center gap-2'>
        <Activity className='h-4 w-4' />
        <span>{formatDrivingModeLabel(currentDrivingMode)}</span>
        {pendingDrivingMode && (
          <Badge variant='secondary' className='text-[10px]'>
            sincronizando
          </Badge>
        )}
      </div>

      <div className='flex items-center gap-2'>
        <Activity className='h-4 w-4' />
        <span>Ultima telemetria: {lastTelemetryLabel}</span>
      </div>

      <div className='flex items-center gap-2'>
        <Clock className='h-4 w-4' />
        <span>{currentTime}</span>
      </div>
    </div>
  );
};

export default StatusBar;
