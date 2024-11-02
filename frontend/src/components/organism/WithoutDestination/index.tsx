import { Button } from '@/components/ui/button';
import { useRouter } from 'next/navigation';

const WithoutDestination = () => {
  const router = useRouter();


  return (
    <div className='flex flex-col items-center gap-16'>
      <h1 className='text-2xl font-bold '>Sem destino definido</h1>
      <Button className=' font-bold ' onClick={() => router.push('/navigation')}>Definir destino</Button>
    </div>
  );
};

export { WithoutDestination };
