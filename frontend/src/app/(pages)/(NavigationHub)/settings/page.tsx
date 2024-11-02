import { ModeToggle } from '@/components/atomics/ModeToggle';
import { Label } from '@/components/ui/label';
import { Separator } from '@/components/ui/separator';

const Settings = () => {
  return (
    <div className='flex flex-col max-w-lg w-full  min-h-full gap-4  pt-16'>
      <h1 className='text-2xl font-bold'>Configurações</h1>
      <Separator />
      <div className='flex items-center justify-between'>
        <Label className='text-md'>Tema atual</Label>{' '}
        <ModeToggle showSelectedTheme={true} />
      </div>
    </div>
  );
};

export default Settings;
