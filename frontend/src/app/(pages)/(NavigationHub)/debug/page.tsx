'use client';

import React, { useRef, useEffect, useState } from 'react';
import { useVehicleConfig } from '@/providers/VehicleConfigProvider';
import { Button } from '@/components/ui/button';
import { BiSolidJoystick } from 'react-icons/bi';
import { useRouter } from 'next/navigation';
import { WifiOff } from 'lucide-react';
import Image from 'next/image';
import { Label } from '@/components/ui/label';
import SteeringWheel from '@/components/molecules/SteeringWheel';
import VehicleLogs from '@/components/organism/VehicleLogs';

const Debug = () => {
  const router = useRouter();
  const { isConnected, currentFrame } = useVehicleConfig();
  const [frame, setFrame] = useState<string | null>(
    'https://encrypted-tbn0.gstatic.com/images?q=tbn:ANd9GcRMDmS_zioHW1hL8bhKdt3aX4xcc4Ou-dFH8Q&s'
  );
  const requestRef = useRef<number | null>(null);

  useEffect(() => {
    let lastFrame = '';

    const updateFrame = () => {
      if (currentFrame && currentFrame !== lastFrame) {
        setFrame(currentFrame);
        lastFrame = currentFrame;
      }
      requestRef.current = requestAnimationFrame(updateFrame);
    };

    requestRef.current = requestAnimationFrame(updateFrame);

    return () => {
      if (requestRef.current !== null) {
        cancelAnimationFrame(requestRef.current);
      }
    };
  }, [currentFrame]);

  return (
    <div
      className='relative flex items-center justify-center w-full'
      style={{ height: 'calc(100vh - 152px)' }}
    >
      {frame ? (
        <div className='relative w-full h-full'>
          <Image
            src={frame}
            width={1280}
            height={720}
            alt='Vídeo ao vivo'
            className='absolute inset-0 object-cover w-full h-full py-4 rounded-[24px] scale-x-[-1]'
          />
          <Button
            className='absolute right-4 mt-8 gap-4 font-bold'
            variant='outline'
            onClick={() => router.push('/joystick')}
          >
            <BiSolidJoystick className='w-6 h-6' />
            Abrir joystick
          </Button>

          <SteeringWheel angle={1} />
          <VehicleLogs />
        </div>
      ) : (
        <div className='flex flex-col items-center justify-center gap-4'>
          <WifiOff className='w-8 h-8' />
          <Label className='text-center font-bold shadow-2xl'>
            {!isConnected && 'Sem conexão com o veículo!'}
          </Label>
        </div>
      )}
    </div>
  );
};

export default Debug;
