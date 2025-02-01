'use client';

import { useSession } from 'next-auth/react';
import { ModeToggle } from '@/components/atomics/ModeToggle';
import { RunDestination } from '@/components/organism/RunDestination';
import { WithoutDestination } from '@/components/organism/WithoutDestination';
import { useDestination } from '@/providers/DestinationProvider';
import Image from 'next/image';
import { IMAGES } from '@/assets';
import { NotificationPanel } from '@/components/organism/NotificationPanel';

const Home = () => {
  const { status } = useSession();
  const { selectedDestination } = useDestination();

  if (status === 'loading') {
    return <div>Carregando...</div>;
  }

  return (
    <div className='flex items-center  justify-between w-full pb-16'>
      <div className='flex w-full items-center justify-center'>
        <h2>Velocidade</h2>
      </div>
      <div className='flex flex-col w-[60%] items-center justify-between h-auto  py-8'>
        <NotificationPanel />
        <Image
          className='object-cover'
          fill={false}
          src={IMAGES.car.carDefault}
          alt='car'
        />
      </div>
      <div className='flex w-full items-center justify-center '>
        {selectedDestination ? <RunDestination /> : <WithoutDestination />}
      </div>
    </div>
  );
};

export default Home;
