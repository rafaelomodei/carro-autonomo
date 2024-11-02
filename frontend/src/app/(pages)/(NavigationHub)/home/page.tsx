'use client';

import { useSession } from 'next-auth/react';
import { ModeToggle } from '@/components/atomics/ModeToggle';
import { RunDestination } from '@/components/organism/RunDestination';
import { WithoutDestination } from '@/components/organism/WithoutDestination';
import { useDestination } from '@/providers/DestinationProvider';

const Home = () => {
  const { data: session, status } = useSession();
  const { selectedDestination } = useDestination();

  if (status === 'loading') {
    return <div>Carregando...</div>;
  }

  return (
    <div className='flex items-center  justify-between w-full'>
      <div className='flex w-full items-center justify-center'>
        <h2>Velocidade</h2>
      </div>
      <div className='flex flex-col w-full items-center justify-between h-auto  py-8'>
        <div className='flex'>
          <h1 className='text-3xl font-bold'>Alerta</h1>
          <ModeToggle />
        </div>

        <p className=' font-bold'>Carro andando</p>
      </div>
      <div className='flex w-full items-center justify-center '>
        {selectedDestination ? <RunDestination /> : <WithoutDestination />}
      </div>
    </div>
  );
};

export default Home;
