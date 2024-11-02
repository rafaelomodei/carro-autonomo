'use client';

import React, { useEffect, useRef, useState } from 'react';
import { ModeToggle } from '@/components/atomics/ModeToggle';
import { Label } from '@/components/ui/label';
import { Separator } from '@/components/ui/separator';
import { useWebSocket } from '@/hook/useWebSocket';

const Debug = () => {
  const { videoBlob, isConnected } = useWebSocket();
  const videoRef = useRef<HTMLVideoElement>(null);

  useEffect(() => {
    if (videoBlob && videoRef.current) {
      const videoUrl = URL.createObjectURL(videoBlob);
      videoRef.current.src = videoUrl;
      videoRef.current.play();
    }
  }, [videoBlob]);

  return (
    <div className='relative flex flex-col w-full gap-4 pt-16'>
      <div className='flex items-center justify-center w-full h-64 bg-black'>
        <video ref={videoRef} className='w-full h-full' controls />
      </div>
    </div>
  );
};

export default Debug;
