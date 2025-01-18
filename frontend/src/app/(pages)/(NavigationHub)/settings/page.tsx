'use client';

import { ModeToggle } from '@/components/atomics/ModeToggle';
import { Label } from '@/components/ui/label';
import { Separator } from '@/components/ui/separator';
import { Switch } from '@/components/ui/switch';
import { useVehicleConfig, DriveMode } from '@/providers/VehicleConfigProvider';

const Settings = () => {
  const { config, setConfig } = useVehicleConfig();

  const handleDriveModeChange = (value: boolean) => {
    setConfig({
      ...config,
      driveMode: value ? DriveMode.MANUAL : DriveMode.AUTONOMOUS,
    });
  };

  return (
    <div className='flex flex-col max-w-lg w-full min-h-full gap-8 pt-16'>
      <div>
        <h1 className='text-2xl font-bold'>Configurações</h1>
        <Separator />
      </div>
      <div className='flex items-center justify-between'>
        <Label className='text-md'>Tema atual</Label>
        <ModeToggle showSelectedTheme={true} />
      </div>

      <div className='flex items-center justify-between'>
        <Label className='text-md'>Modo de condução manual</Label>
        <Switch
          id='drivingMode'
          checked={config.driveMode.id === DriveMode.MANUAL.id}
          onCheckedChange={handleDriveModeChange}
        />
      </div>
    </div>
  );
};

export default Settings;
