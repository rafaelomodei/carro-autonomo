'use client';

import {
  ChevronDown,
  ChevronLeft,
  ChevronRight,
  ChevronUp,
  StopCircle,
} from 'lucide-react';
import { useVehicleConfig } from '@/providers/VehicleConfigProvider';
import { Button } from '@/components/ui/button';

const Joystick = () => {
  const { sendMessage } = useVehicleConfig();

  const handleCommand = (action: string, payload?: Record<string, unknown>) => {
    const command = { type: 'commands', payload: [{ action, ...payload }] };
    sendMessage(command);
  };

  return (
    <div className='flex items-center justify-between w-full p-4'>
      {/* Setas de direção no canto esquerdo */}
      <div className='flex items-center gap-8'>
        <Button
          variant='outline'
          className='w-20 h-20'
          onClick={() => handleCommand('turn', { direction: 'left' })}
        >
          <ChevronLeft className='w-8 h-8' />
        </Button>
        <Button
          variant='outline'
          className='w-20 h-20'
          onClick={() => handleCommand('turn', { direction: 'right' })}
        >
          <ChevronRight className='w-8 h-8' />
        </Button>
      </div>

      <div className='flex items-center h-48 justify-end  gap-8'>
        <Button
          variant='outline'
          className='w-40 h-20'
          onClick={() => handleCommand('accelerate', { speed: 0 })}
        >
          <StopCircle className='w-8 h-8' />
        </Button>

        {/* Botões de freio e acelerador no canto direito */}
        <div className='flex flex-col items-center gap-8'>
          <Button
            variant='outline'
            className='w-20 h-20'
            onClick={() => handleCommand('accelerate', { speed: 200 })}
          >
            <ChevronUp className='w-8 h-8' />
          </Button>
          <Button
            variant='outline'
            className='w-20 h-20'
            onClick={() => handleCommand('accelerate', { speed: -200 })}
          >
            <ChevronDown className='w-8 h-8' />
          </Button>
        </div>
      </div>
    </div>
  );
};

export default Joystick;
