import { useCurrentTime } from '@/hooks/useCurrentTime';
import {
  useVehicleConfig,
  VEHICLE_CAR_CONNECTION_URL,
} from '@/providers/VehicleConfigProvider';
import { Wifi, WifiOff, Activity, Clock } from 'lucide-react';

const StatusBar = () => {
  const { isConnected, config, setConfig } = useVehicleConfig();
  const currentTime = useCurrentTime();
  const vehicleConfig = useVehicleConfig();

  const handleConnect = () => {
    if (isConnected)
      return (
        <div className='flex items-center space-x-2 text-xs'>
          <Wifi className='w-4 h-4' />
          <span>Conectado ao veículo</span>
        </div>
      );

    return (
      <div
        className='flex items-center space-x-2 text-xs'
        role='button'
        onClick={() =>
          setConfig({
            ...config,
            carConnection: VEHICLE_CAR_CONNECTION_URL,
          })
        }
      >
        <WifiOff className='w-4 h-4' />
        <span>Sem conexão com o veículo</span>
      </div>
    );
  };

  return (
    <div className='flex w-full items-center justify-end  gap-8 '>
      {vehicleConfig.currentFrame && (
        <div className='flex items-center space-x-2 text-xs'>
          <Activity className='w-4 h-4' />
          <span>{`${vehicleConfig.currentFrame} FPS`}</span>
        </div>
      )}

      <div className='flex items-center space-x-2 text-xs'>
        {vehicleConfig.config.driveMode.icon}
        <span>{vehicleConfig.config.driveMode.label}</span>
      </div>

      {handleConnect()}

      <div className='flex items-center space-x-2 text-xs'>
        <Clock className='w-4 h-4' />
        <span>{currentTime}</span>
      </div>
    </div>
  );
};

export default StatusBar;
