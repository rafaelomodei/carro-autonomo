'use client';

import { ModeToggle } from '@/components/atomics/ModeToggle';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { Separator } from '@/components/ui/separator';
import { Switch } from '@/components/ui/switch';
import { isValidWebSocketUrl } from '@/lib/websocket';
import { useVehicleConfig, DriveMode } from '@/providers/VehicleConfigProvider';
import { useEffect, useState } from 'react';

const Settings = () => {
  const { config, setConfig } = useVehicleConfig();
  const [url, setUrl] = useState<string | undefined>();
  const [initialUrl, setInitialUrl] = useState(config.carConnection);
  const [isSaveEnabled, setIsSaveEnabled] = useState(false);

  useEffect(() => {
    if (!isValidWebSocketUrl(url)) return setIsSaveEnabled(false);

    setIsSaveEnabled(url !== initialUrl);
  }, [url, initialUrl]);

  const handleDriveModeChange = (value: boolean) => {
    setConfig({
      ...config,
      driveMode: value ? DriveMode.MANUAL : DriveMode.AUTONOMOUS,
    });
  };

  const handleSave = () => {
    setInitialUrl(config.carConnection);
    setIsSaveEnabled(false);

    setConfig({
      ...config,
      carConnection: url,
    });
  };

  return (
    <div className='flex flex-col max-w-lg w-full min-h-full gap-8 pt-16'>
      <div>
        <h1 className='text-2xl font-bold'>Configurações</h1>
        <Separator />
      </div>
      <div className='flex items-center justify-between'>
        <Label className='text-md'>Servidor do veículo</Label>
        <Input
          id='server-car'
          bg-orange-950
          onChange={(e) => setUrl(e.target.value)}
          value={config.carConnection}
          className='max-w-60'
        />
        <Button onClick={handleSave} disabled={!isSaveEnabled}>
          Salvar
        </Button>
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
