'use client';

import { useCar } from '@/hooks/useCar';
import React, { useRef } from 'react';

const Debug = () => {
  const videoRef = useRef<HTMLVideoElement>(null);

  return (
    <div className='relative flex flex-col w-full gap-4 pt-16'>
      <div className='flex items-center justify-center w-full h-64 bg-black'>
        <video ref={videoRef} className='w-full h-full' controls />
      </div>
    </div>
  );
};

export default Debug;
