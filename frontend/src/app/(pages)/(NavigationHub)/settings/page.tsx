'use client';

import { ModeToggle } from '@/components/atomics/ModeToggle';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { Separator } from '@/components/ui/separator';
import { Slider } from '@/components/ui/slider';
import { Switch } from '@/components/ui/switch';
import { isValidWebSocketUrl } from '@/lib/websocket';
import { useVehicleConfig, DriveMode } from '@/providers/VehicleConfigProvider';
import { useEffect, useState } from 'react';

const Settings = () => {
  const { config, setConfig } = useVehicleConfig();
  const [url, setUrl] = useState<string>('');
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

  const handleDriveSteeringSensitivity = (value: number[]) => {
    setConfig({
      ...config,
      steeringSensitivity: value[0], // O valor é o primeiro elemento do array
    });
  };

  const handleServerSave = () => {
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
        <h1 className='text-3xl font-bold'>Configurações</h1>
        <Separator />
      </div>
      <div className='flex items-center justify-between'>
        <Label className='text-md'>Servidor do veículo</Label>
        <Input
          id='server-car'
          onChange={(e) => setUrl(e.target.value)}
          value={url}
          defaultValue={config.carConnection}
          className='max-w-60'
        />
        <Button onClick={handleServerSave} disabled={!isSaveEnabled}>
          Salvar
        </Button>
      </div>

      <div className='flex items-center justify-between'>
        <Label className='text-md'>Tema atual</Label>
        <ModeToggle showSelectedTheme={true} />
      </div>

      <div>
        <h1 className='text-2xl font-bold'>Direção</h1>
        <Separator />
      </div>

      <div className='flex items-center justify-between'>
        <Label className='text-md'>Modo de condução manual</Label>
        <Switch
          id='drivingMode'
          checked={config.driveMode.id === DriveMode.MANUAL.id}
          onCheckedChange={handleDriveModeChange}
        />
      </div>

      <div className='flex-col flex gap-4'>
        <Label className='text-md'>
          Sensibilidade da direção:{' '}
          <strong>{config.steeringSensitivity}</strong>
        </Label>
        <Slider
          defaultValue={[config.steeringSensitivity]}
          onValueChange={(value) => handleDriveSteeringSensitivity(value)}
          max={1}
          min={0.1}
          step={0.1}
        />
      </div>
      <Separator />
    </div>
  );
};

export default Settings;
