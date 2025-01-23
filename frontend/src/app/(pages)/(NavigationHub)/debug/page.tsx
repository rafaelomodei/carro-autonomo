'use client';

import React from 'react';
import { useVehicleConfig } from '@/providers/VehicleConfigProvider';

const Debug = () => {
  const { isConnected, currentFrame } = useVehicleConfig();

  return (
    <div
      className='relative flex items-center justify-center w-full'
      style={{
        height: 'calc(100vh - 152px)', // Cálculo direto da altura
      }}
    >
      {currentFrame ? (
        <img
          src={currentFrame}
          alt='Vídeo ao vivo'
          className='absolute inset-0 object-cover w-full h-full'
        />
      ) : (
        <h2 className='text-white text-center'>
          {!isConnected && 'Conecte a um servidor webscoket'}
        </h2>
      )}
    </div>
  );
};

export default Debug;
