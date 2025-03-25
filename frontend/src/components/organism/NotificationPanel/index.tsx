import { IMAGES } from '@/assets';
import { Label } from '@/components/ui/label';
import Image from 'next/image';

const NotificationPanel = () => {
  return (
    <div className='flex items-center justify-center gap-4 rounded-md w-fit min-w-72 p-4 my-8 min-h16 bg-foreground-black dark:bg-foreground-light'>
      <Image
        width={60}
        height={60}
        src={IMAGES.trafficSigns.r1ParadaObrigatoria}
        alt='placa de parada obrigatória'
      />

      <Label className='text-lg font-bold'>Parada Obrigatória</Label>
    </div>
  );
};

export { NotificationPanel };
