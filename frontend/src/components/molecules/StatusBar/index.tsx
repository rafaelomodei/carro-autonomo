import { useCar } from '@/hooks/useCar';
import { useCurrentTime } from '@/hooks/useCurrentTime';
import { useVehicleConfig } from '@/providers/VehicleConfigProvider';
import { Wifi, WifiOff, Activity, Clock } from 'lucide-react';

const StatusBar = () => {
  const { isConnected } = useCar('ws://localhost:8080');
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
      <div className='flex items-center space-x-2 text-xs'>
        <WifiOff className='w-4 h-4' />
        <span>Sem conexão com o veículo</span>
      </div>
    );
  };

  return (
    <div className='flex w-full items-center justify-end  gap-8 '>
      <div className='flex items-center space-x-2 text-xs'>
        <Activity className='w-4 h-4' />
        <span>15 FPS</span>
      </div>

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
