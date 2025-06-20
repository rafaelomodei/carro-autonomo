'use client';

import { useState } from 'react';
import { useVehicleConfig } from '@/providers/VehicleConfigProvider';
import { ChevronDown, ChevronUp, MapPin } from 'lucide-react';
import { useDestination } from '@/providers/DestinationProvider';
import SignalDetectionPanel from '@/components/molecules/SignalDetectionPanel';
import { IMAGES } from '@/assets';

const VehicleLogs = () => {
  const [expanded, setExpanded] = useState(true);
  const { selectedDestination } = useDestination();
  const { config, recognizedSignals } = useVehicleConfig();

  return (
    <div className='absolute top-8 left-4 bg-white dark:bg-background shadow-md rounded-lg p-4 w-72 text-sm'>
      <div
        className='flex items-center justify-between cursor-pointer'
        onClick={() => setExpanded(!expanded)}
      >
        <h2 className='font-bold'>Logs do sistema</h2>
        {expanded ? (
          <ChevronUp className='w-4 h-4' />
        ) : (
          <ChevronDown className='w-4 h-4' />
        )}
      </div>

      {expanded && (
        <div className='mt-4 space-y-4'>
          {selectedDestination && (
            <div>
              <p className='text-xs '>Destino</p>
              <div className='flex items-center gap-2 mt-1'>
                <MapPin className='w-4 h-4' />
                <span>{`Ponto ${selectedDestination}`}</span>{' '}
                {/* Você pode tornar isso dinâmico depois */}
              </div>
            </div>
          )}

          {Boolean(recognizedSignals.VERTICAL.length) && (
            <div>
              <p className='text-md font-bold'>Sinalização vertical:</p>
              <SignalDetectionPanel signs={recognizedSignals.VERTICAL} />
            </div>
          )}

          {Boolean(recognizedSignals.HORIZONTAL.length) && (
            <div>
              <p className='text-md font-bold'>Sinalização Horizontal:</p>
              <SignalDetectionPanel signs={recognizedSignals.HORIZONTAL} />
            </div>
          )}

          <div>
            <p className='text-xs'>Modo de condução</p>
            <div className='flex items-center gap-2 mt-1'>
              {config.driveMode.icon}
              <span>{config.driveMode.label}</span>
            </div>
          </div>
        </div>
      )}
    </div>
  );
};

export default VehicleLogs;
