'use client';

import { useState } from 'react';
import { useVehicleConfig } from '@/providers/VehicleConfigProvider';
import {
  ChevronDown,
  ChevronUp,
  MapPin,
  Wifi,
  Gauge,
  Sparkles,
  CarFront,
} from 'lucide-react';
import { useDestination } from '@/providers/DestinationProvider';
import SignalDetectionPanel from '@/components/molecules/SignalDetectionPanel';
import { IMAGES } from '@/assets';

const VehicleLogs = () => {
  const { config, isConnected } = useVehicleConfig();
  const [expanded, setExpanded] = useState(true);

  const { selectedDestination } = useDestination();

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

          <div>
            <p className='text-md font-bold'>Sinalização vertical:</p>
            <SignalDetectionPanel
              signs={[
                {
                  confidence: 20,
                  label: 'Parada Obrigatória',
                  iconUrl: IMAGES.trafficSigns.r1ParadaObrigatoria,
                },
                {
                  confidence: 87,
                  label: 'Estacionamento',
                  iconUrl: IMAGES.trafficSigns.r6bEstacionamentoRegulamentado,
                },
              ]}
            />
          </div>

          <div>
            <p className='text-md font-bold'>Sinalização Horizontal:</p>
            <SignalDetectionPanel
              signs={[
                {
                  confidence: 70,
                  label: 'Faixa à esquerda',
                },
                {
                  confidence: 90,
                  label: 'Faixa à direita',
                },
              ]}
            />
          </div>
        </div>
      )}
    </div>
  );
};

export default VehicleLogs;
