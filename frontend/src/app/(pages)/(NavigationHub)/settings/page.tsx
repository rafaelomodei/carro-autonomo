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

  const handlePidControlP = (value: number[]) => {
    setConfig({
      ...config,
      pidControl: { ...config.pidControl, p: value[0] },
    });
  };

  const handlePidControlI = (value: number[]) => {
    setConfig({
      ...config,
      pidControl: { ...config.pidControl, i: value[0] },
    });
  };

  const handlePidControlD = (value: number[]) => {
    setConfig({
      ...config,
      pidControl: { ...config.pidControl, d: value[0] },
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
    <div className='flex flex-col max-w-lg w-full min-h-full overflow-y-auto gap-8 pt-16'>
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

      <div
        className={`flex-col flex gap-4 ${
          config.driveMode.id !== DriveMode.MANUAL.id ? 'opacity-45' : ''
        }`}
      >
        <Label className='text-md'>
          Sensibilidade da direção:{' '}
          <strong>{config.steeringSensitivity}</strong>
        </Label>
        <Slider
          disabled={config.driveMode.id !== DriveMode.MANUAL.id}
          defaultValue={[config.steeringSensitivity]}
          onValueChange={(value) => handleDriveSteeringSensitivity(value)}
          max={1}
          min={0.1}
          step={0.1}
        />
      </div>

      <div
        className={`flex-col flex gap-4 ${
          config.driveMode.id === DriveMode.MANUAL.id ? 'opacity-45' : ''
        }`}
      >
        <Label className='text-md font-bold'>Controle PID</Label>

        <div className='flex w-full gap-8'>
          <div className='flex-col flex gap-4 items-center w-full'>
            <Label className='text-md'>Kp: {config.pidControl.p}</Label>
            <Slider
              defaultValue={[config.steeringSensitivity]}
              disabled={config.driveMode.id === DriveMode.MANUAL.id}
              onValueChange={(value) => handlePidControlP(value)}
              max={1}
              min={0.1}
              step={0.1}
            />
          </div>
          <div className='flex-col flex gap-4 items-center w-full'>
            <Label className='text-md'>Ki: {config.pidControl.i}</Label>
            <Slider
              defaultValue={[config.steeringSensitivity]}
              disabled={config.driveMode.id === DriveMode.MANUAL.id}
              onValueChange={(value) => handlePidControlI(value)}
              max={1}
              min={0.1}
              step={0.1}
            />
          </div>
          <div className='flex-col flex gap-4 items-center w-full'>
            <Label className='text-md'>Kd: {config.pidControl.d}</Label>
            <Slider
              defaultValue={[config.steeringSensitivity]}
              disabled={config.driveMode.id === DriveMode.MANUAL.id}
              onValueChange={(value) => handlePidControlD(value)}
              max={1}
              min={0.1}
              step={0.1}
            />
          </div>
        </div>
      </div>

      <Separator />
      <div className='h-8' />
    </div>
  );
};

export default Settings;
