'use client';

import { MapPin } from 'lucide-react';
import DestinationButton from '@/components/atomics/DestinationButton';
import { useRouter } from 'next/navigation';
import {
  DestinationType,
  useDestination,
} from '@/providers/DestinationProvider';

const NavigationTo = () => {
  const { selectedDestination, setDestination } = useDestination();

  const router = useRouter();

  const destinations = [
    DestinationType.POINT_A,
    DestinationType.POINT_B,
    DestinationType.POINT_C,
  ];

  const handleSelect = (destination: DestinationType) => {
    setDestination(destination);
    router.push('/home');
  };

  return (
    <div className='flex flex-col items-center w-full p-8 '>
      <h1 className='text-2xl font-bold mb-8'>Selecione o destino</h1>

      <div className='flex gap-8'>
        {destinations.map((destination) => (
          <DestinationButton
            key={destination}
            label={`Ponto ${destination}`}
            Icon={MapPin}
            isSelected={selectedDestination === destination}
            onClick={() => handleSelect(destination)}
          />
        ))}
      </div>
    </div>
  );
};

export default NavigationTo;
