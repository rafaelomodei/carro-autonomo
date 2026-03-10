'use client';

import { useSession } from 'next-auth/react';
import { RunDestination } from '@/components/organism/RunDestination';
import { WithoutDestination } from '@/components/organism/WithoutDestination';
import { useDestination } from '@/providers/DestinationProvider';
import { useVehicleConfig } from '@/providers/VehicleConfigProvider';
import Image from 'next/image';
import { IMAGES } from '@/assets';
import { NotificationPanel } from '@/components/organism/NotificationPanel';
import {
  formatConnectionStateLabel,
  formatDrivingModeLabel,
  formatTrackingStateLabel,
} from '@/lib/autonomous-car';

const Home = () => {
  const { status } = useSession();
  const { selectedDestination } = useDestination();
  const { autonomousControlTelemetry, connectionState } = useVehicleConfig();

  if (status === 'loading') {
    return <div>Carregando...</div>;
  }

  const currentDrivingMode =
    autonomousControlTelemetry?.driving_mode ?? 'manual';
  const trackingState =
    autonomousControlTelemetry?.tracking_state ?? 'manual';

  return (
    <div className='flex items-center  justify-between w-full pb-16'>
      <div className='flex w-full items-center justify-center'>
        <div className='space-y-2 text-center'>
          <h2 className='text-xl font-semibold'>
            {formatConnectionStateLabel(connectionState)}
          </h2>
          <p className='text-sm text-muted-foreground'>
            {formatDrivingModeLabel(currentDrivingMode)} | tracking:{' '}
            {formatTrackingStateLabel(trackingState)}
          </p>
        </div>
      </div>
      <div className='flex flex-col w-[55%] items-center justify-between h-auto  py-8'>
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
