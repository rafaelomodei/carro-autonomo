'use client';

import { useCallback, useEffect, useRef } from 'react';
import {
  ChevronDown,
  ChevronLeft,
  ChevronRight,
  ChevronUp,
  StopCircle,
} from 'lucide-react';
import { Button } from '@/components/ui/button';
import {
  MANUAL_COMMAND_INTERVAL_MS,
  formatDrivingModeLabel,
} from '@/lib/autonomous-car';
import { useVehicleConfig } from '@/providers/VehicleConfigProvider';

const Joystick = () => {
  const {
    autonomousControlTelemetry,
    canSendManualCommands,
    connectionState,
    sendManualMotion,
    sendManualSteering,
  } = useVehicleConfig();
  const motionIntervalRef = useRef<number | null>(null);
  const steeringIntervalRef = useRef<number | null>(null);

  const clearMotionLoop = useCallback(() => {
    if (motionIntervalRef.current !== null) {
      window.clearInterval(motionIntervalRef.current);
      motionIntervalRef.current = null;
    }
  }, []);

  const stopMotionLoop = () => {
    clearMotionLoop();
    sendManualMotion('stop');
  };

  const clearSteeringLoop = useCallback(() => {
    if (steeringIntervalRef.current !== null) {
      window.clearInterval(steeringIntervalRef.current);
      steeringIntervalRef.current = null;
    }
  }, []);

  const stopSteeringLoop = () => {
    clearSteeringLoop();
    sendManualSteering('center');
  };

  const startMotionLoop = (command: 'forward' | 'backward') => {
    if (!canSendManualCommands) {
      return;
    }

    clearMotionLoop();
    sendManualMotion(command);
    motionIntervalRef.current = window.setInterval(() => {
      sendManualMotion(command);
    }, MANUAL_COMMAND_INTERVAL_MS);
  };

  const startSteeringLoop = (command: 'left' | 'right') => {
    if (!canSendManualCommands) {
      return;
    }

    clearSteeringLoop();
    sendManualSteering(command);
    steeringIntervalRef.current = window.setInterval(() => {
      sendManualSteering(command);
    }, MANUAL_COMMAND_INTERVAL_MS);
  };

  useEffect(() => {
    if (!canSendManualCommands) {
      clearMotionLoop();
      clearSteeringLoop();
    }
  }, [canSendManualCommands, clearMotionLoop, clearSteeringLoop]);

  useEffect(() => {
    return () => {
      clearMotionLoop();
      clearSteeringLoop();
    };
  }, [clearMotionLoop, clearSteeringLoop]);

  const currentDrivingMode =
    autonomousControlTelemetry?.driving_mode ?? 'manual';
  const disableManualControls = !canSendManualCommands;

  return (
    <div className='flex w-full max-w-5xl flex-col gap-8 p-4'>
      <div className='space-y-2'>
        <h1 className='text-3xl font-bold'>Joystick manual</h1>
        <p className='text-sm text-muted-foreground'>
          Segure os controles para manter o comando ativo. Ao soltar, o front
          envia `stop` para movimento e `center` para direcao.
        </p>
        <p className='text-sm text-muted-foreground'>
          Estado atual: {formatDrivingModeLabel(currentDrivingMode)} | conexao:{' '}
          {connectionState}
        </p>
      </div>

      <div className='grid gap-6 lg:grid-cols-[1fr_auto_1fr]'>
        <div className='rounded-2xl border p-6'>
          <p className='mb-4 text-sm font-semibold'>Direcao</p>
          <div className='flex items-center justify-center gap-6'>
            <Button
              variant='outline'
              className='h-24 w-24'
              disabled={disableManualControls}
              onPointerDown={() => startSteeringLoop('left')}
              onPointerUp={stopSteeringLoop}
              onPointerLeave={stopSteeringLoop}
              onPointerCancel={stopSteeringLoop}
            >
              <ChevronLeft className='h-10 w-10' />
            </Button>

            <Button
              variant='outline'
              className='h-24 w-24'
              disabled={disableManualControls}
              onPointerDown={() => startSteeringLoop('right')}
              onPointerUp={stopSteeringLoop}
              onPointerLeave={stopSteeringLoop}
              onPointerCancel={stopSteeringLoop}
            >
              <ChevronRight className='h-10 w-10' />
            </Button>
          </div>
        </div>

        <div className='rounded-2xl border p-6'>
          <p className='mb-4 text-sm font-semibold'>Emergencia</p>
          <Button
            variant='outline'
            className='h-24 w-24'
            disabled={disableManualControls}
            onClick={() => {
              stopMotionLoop();
              stopSteeringLoop();
            }}
          >
            <StopCircle className='h-10 w-10' />
          </Button>
        </div>

        <div className='rounded-2xl border p-6'>
          <p className='mb-4 text-sm font-semibold'>Movimento</p>
          <div className='flex flex-col items-center gap-6'>
            <Button
              variant='outline'
              className='h-24 w-24'
              disabled={disableManualControls}
              onPointerDown={() => startMotionLoop('forward')}
              onPointerUp={stopMotionLoop}
              onPointerLeave={stopMotionLoop}
              onPointerCancel={stopMotionLoop}
            >
              <ChevronUp className='h-10 w-10' />
            </Button>
            <Button
              variant='outline'
              className='h-24 w-24'
              disabled={disableManualControls}
              onPointerDown={() => startMotionLoop('backward')}
              onPointerUp={stopMotionLoop}
              onPointerLeave={stopMotionLoop}
              onPointerCancel={stopMotionLoop}
            >
              <ChevronDown className='h-10 w-10' />
            </Button>
          </div>
        </div>
      </div>

      <div className='rounded-2xl border border-dashed p-4 text-sm text-muted-foreground'>
        {disableManualControls
          ? 'Os controles manuais ficam indisponiveis quando a conexao cai ou quando o veiculo esta em modo autonomo.'
          : 'Atalhos de teclado ativos: W/S para movimento, A/D para direcao e espaco para parada imediata.'}
      </div>
    </div>
  );
};

export default Joystick;
