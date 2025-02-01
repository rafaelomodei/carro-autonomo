'use client';

import React from 'react';
import { useVehicleConfig } from '@/providers/VehicleConfigProvider';
import { Button } from '@/components/ui/button';
import { BiSolidJoystick } from 'react-icons/bi';
import { useRouter } from 'next/navigation';
import { WifiOff } from 'lucide-react';
import Image from 'next/image';
import { Label } from '@/components/ui/label';

const Debug = () => {
  const router = useRouter();
  const { isConnected, currentFrame } = useVehicleConfig();

  return (
    <div
      className='relative flex items-center justify-center w-full'
      style={{
        height: 'calc(100vh - 152px)', // Cálculo direto da altura
      }}
    >
      {currentFrame ? (
        <div className='relative w-full h-full'>
          <Image
            src={currentFrame}
            alt='Vídeo ao vivo'
            className='absolute inset-0 object-cover w-full h-full py-4 rounded-[24px]'
          />
          <Button
            className='absolute right-4 mt-8 gap-4 font-bold'
            variant='outline'
            onClick={() => router.push('/joystick')}
          >
            <BiSolidJoystick className='w-6 h-6' />
            Abrir joystick
          </Button>
        </div>
      ) : (
        <div className='flex flex-col items-center justify-center gap-4'>
          <WifiOff className='w-8 h-8' />
          <Label className='text-white text-center font-bold shadow-2xl'>
            {!isConnected && 'Sem conexão com o veículo!'}
          </Label>
        </div>
      )}
    </div>
  );
};

export default Debug;
