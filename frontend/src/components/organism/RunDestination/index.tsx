import { Button } from '@/components/ui/button';
import { Label } from '@/components/ui/label';
import { useDestination } from '@/providers/DestinationProvider';
import { MapPin, Navigation2 } from 'lucide-react';
import { useRouter } from 'next/navigation';

const RunDestination = () => {
  const router = useRouter();
  const { selectedDestination } = useDestination();

  const handleNavigation = (route: string) => {
    router.push(`/${route}`);
  };

  return (
    <div className='flex flex-col items-center gap-4'>
      <Button
        variant='secondary'
        size='lg'
        className='flex items-center gap-4'
        onClick={() => handleNavigation(`navigation`)}
      >
        <MapPin className='w-5 h-5' />
        <Label>Destino:</Label>
        <Label className='font-bold'>Ponto {selectedDestination}</Label>
      </Button>

      <div className='text-2xl'>
        <span className='flex flex-col w-2 animate-text-glow bg-gradient-to-t from-transparent via-black dark:via-white to-transparent bg-[length:100%_200%] bg-clip-text text-transparent'>
          | | | |
        </span>
      </div>

      <Navigation2 className='w-5 h-5 animate-pulse-grow' />
    </div>
  );
};

export { RunDestination };
