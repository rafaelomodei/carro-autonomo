import { Badge } from '@/components/ui/badge';
import { useCurrentTime } from '@/hooks/useCurrentTime';
import {
  formatConnectionStateLabel,
  formatDrivingModeLabel,
} from '@/lib/autonomous-car';
import { useVehicleConfig } from '@/providers/VehicleConfigProvider';
import { Activity, Clock, Wifi, WifiOff } from 'lucide-react';

const StatusBar = () => {
  const currentTime = useCurrentTime();
  const {
    autonomousControlTelemetry,
    connectionState,
    isConnected,
    lastTelemetryAt,
    pendingDrivingMode,
  } = useVehicleConfig();

  const currentDrivingMode =
    autonomousControlTelemetry?.driving_mode ?? pendingDrivingMode ?? 'manual';
  const lastTelemetryLabel = lastTelemetryAt
    ? new Date(lastTelemetryAt).toLocaleTimeString('pt-BR')
    : 'Aguardando';

  return (
    <div className='flex w-full flex-wrap items-center justify-end gap-4 text-xs'>
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
