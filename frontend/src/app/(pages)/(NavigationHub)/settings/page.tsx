'use client';

import { useEffect, useState } from 'react';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { Separator } from '@/components/ui/separator';
import { Slider } from '@/components/ui/slider';
import { Switch } from '@/components/ui/switch';
import { ModeToggle } from '@/components/atomics/ModeToggle';
import {
  DEFAULT_VEHICLE_CONNECTION_URL,
  formatConnectionStateLabel,
  formatDrivingModeLabel,
  formatStopReasonLabel,
  formatTrackingStateLabel,
} from '@/lib/autonomous-car';
import { isValidWebSocketUrl } from '@/lib/websocket';
import { useVehicleConfig } from '@/providers/VehicleConfigProvider';

const Settings = () => {
  const {
    autonomousControlTelemetry,
    config,
    connectionState,
    isConnected,
    pendingAutonomousCommand,
    pendingDrivingMode,
    saveConnectionUrl,
    setDrivingMode,
    startAutonomous,
    stopAutonomous,
    updateRuntimeSetting,
  } = useVehicleConfig();

  const [connectionUrl, setConnectionUrl] = useState(config.connectionUrl);
  const [steeringSensitivity, setSteeringSensitivity] = useState([
    config.steeringSensitivity,
  ]);
  const [steeringCommandStep, setSteeringCommandStep] = useState([
    config.steeringCommandStep,
  ]);
  const [pidKp, setPidKp] = useState([config.pidControl.p]);
  const [pidKi, setPidKi] = useState([config.pidControl.i]);
  const [pidKd, setPidKd] = useState([config.pidControl.d]);

  useEffect(() => {
    setConnectionUrl(config.connectionUrl);
  }, [config.connectionUrl]);

  useEffect(() => {
    setSteeringSensitivity([config.steeringSensitivity]);
    setSteeringCommandStep([config.steeringCommandStep]);
    setPidKp([config.pidControl.p]);
    setPidKi([config.pidControl.i]);
    setPidKd([config.pidControl.d]);
  }, [
    config.pidControl.d,
    config.pidControl.i,
    config.pidControl.p,
    config.steeringCommandStep,
    config.steeringSensitivity,
  ]);

  const normalizedUrl = connectionUrl.trim();
  const isUrlDirty = normalizedUrl !== config.connectionUrl;
  const canSaveUrl =
    isUrlDirty &&
    (normalizedUrl.length === 0 || isValidWebSocketUrl(normalizedUrl));
  const actualDrivingMode =
    autonomousControlTelemetry?.driving_mode ?? pendingDrivingMode ?? 'manual';
  const isAutonomousMode = actualDrivingMode === 'autonomous';

  return (
    <div className='flex w-full max-w-5xl flex-col gap-8 overflow-y-auto px-2 py-8'>
      <div>
        <h1 className='text-3xl font-bold'>Configuracoes</h1>
        <p className='mt-2 text-sm text-muted-foreground'>
          Esta tela controla apenas o contrato suportado hoje pelo
          `autonomous_car_v3`.
        </p>
      </div>

      <div className='grid gap-6 lg:grid-cols-2'>
        <section className='rounded-2xl border p-6'>
          <div className='flex items-center justify-between'>
            <div>
              <h2 className='text-xl font-semibold'>Conexao do veiculo</h2>
              <p className='text-sm text-muted-foreground'>
                WebSocket compartilhado em modo `client:control`.
              </p>
            </div>
            <ModeToggle showSelectedTheme />
          </div>

          <Separator className='my-4' />

          <div className='space-y-4'>
            <div className='space-y-2'>
              <Label htmlFor='vehicle-server-url'>Servidor WebSocket</Label>
              <Input
                id='vehicle-server-url'
                placeholder={DEFAULT_VEHICLE_CONNECTION_URL}
                value={connectionUrl}
                onChange={(event) => setConnectionUrl(event.target.value)}
              />
              <p className='text-xs text-muted-foreground'>
                Estado atual: {formatConnectionStateLabel(connectionState)}
              </p>
            </div>

            <div className='flex gap-3'>
              <Button
                onClick={() => saveConnectionUrl(normalizedUrl)}
                disabled={!canSaveUrl}
              >
                Salvar conexao
              </Button>
              <Button
                variant='outline'
                onClick={() => saveConnectionUrl('')}
                disabled={!config.connectionUrl}
              >
                Desconectar
              </Button>
            </div>
          </div>
        </section>

        <section className='rounded-2xl border p-6'>
          <div>
            <h2 className='text-xl font-semibold'>Estado operacional</h2>
            <p className='text-sm text-muted-foreground'>
              A telemetria do backend e a fonte de verdade do estado do carro.
            </p>
          </div>

          <Separator className='my-4' />

          <div className='space-y-4 text-sm'>
            <div>
              <p className='text-xs text-muted-foreground'>Modo atual</p>
              <p className='font-semibold'>
                {formatDrivingModeLabel(actualDrivingMode)}
              </p>
              {pendingDrivingMode && (
                <p className='text-xs text-muted-foreground'>
                  Troca de modo em sincronizacao
                </p>
              )}
            </div>

            <div className='flex items-center justify-between rounded-lg border p-4'>
              <div>
                <Label className='text-sm font-medium'>Modo manual</Label>
                <p className='text-xs text-muted-foreground'>
                  Desative para colocar o carro em modo autonomo.
                </p>
              </div>
              <Switch
                checked={actualDrivingMode === 'manual'}
                disabled={!isConnected}
                onCheckedChange={(checked) =>
                  setDrivingMode(checked ? 'manual' : 'autonomous')
                }
              />
            </div>

            <div className='rounded-lg border p-4'>
              <div className='flex flex-wrap gap-3'>
                <Button
                  onClick={startAutonomous}
                  disabled={!isConnected || !isAutonomousMode}
                >
                  Start autonomo
                </Button>
                <Button
                  variant='outline'
                  onClick={stopAutonomous}
                  disabled={!isConnected}
                >
                  Stop autonomo
                </Button>
              </div>

              <div className='mt-3 space-y-1 text-xs text-muted-foreground'>
                <p>
                  Estado de tracking:{' '}
                  {autonomousControlTelemetry
                    ? formatTrackingStateLabel(
                        autonomousControlTelemetry.tracking_state
                      )
                    : 'Aguardando telemetria'}
                </p>
                <p>
                  Motivo da ultima parada:{' '}
                  {autonomousControlTelemetry
                    ? formatStopReasonLabel(
                        autonomousControlTelemetry.stop_reason
                      )
                    : 'Aguardando telemetria'}
                </p>
                {pendingAutonomousCommand && (
                  <p>Comando autonomo pendente: {pendingAutonomousCommand}</p>
                )}
              </div>
            </div>
          </div>
        </section>
      </div>

      <section className='rounded-2xl border p-6'>
        <div>
          <h2 className='text-xl font-semibold'>Ajustes manuais</h2>
          <p className='text-sm text-muted-foreground'>
            Alteracoes aplicadas via `config:*` e reaplicadas ao reconectar.
          </p>
        </div>

        <Separator className='my-4' />

        <div className='grid gap-8 lg:grid-cols-2'>
          <div className='space-y-6'>
            <div className='space-y-3'>
              <Label>
                Sensibilidade da direcao: {steeringSensitivity[0].toFixed(2)}
              </Label>
              <Slider
                value={steeringSensitivity}
                onValueChange={setSteeringSensitivity}
                onValueCommit={(value) =>
                  updateRuntimeSetting('steering.sensitivity', value[0])
                }
                min={0.1}
                max={1}
                step={0.05}
              />
            </div>

            <div className='space-y-3'>
              <Label>
                Passo do comando de direcao: {steeringCommandStep[0].toFixed(2)}
              </Label>
              <Slider
                value={steeringCommandStep}
                onValueChange={setSteeringCommandStep}
                onValueCommit={(value) =>
                  updateRuntimeSetting('steering.command_step', value[0])
                }
                min={0.05}
                max={1}
                step={0.05}
              />
            </div>
          </div>

          <div className='space-y-6'>
            <div className='space-y-3'>
              <Label>Kp: {pidKp[0].toFixed(2)}</Label>
              <Slider
                value={pidKp}
                onValueChange={setPidKp}
                onValueCommit={(value) =>
                  updateRuntimeSetting('autonomous.pid.kp', value[0])
                }
                min={0}
                max={1}
                step={0.01}
              />
            </div>

            <div className='space-y-3'>
              <Label>Ki: {pidKi[0].toFixed(2)}</Label>
              <Slider
                value={pidKi}
                onValueChange={setPidKi}
                onValueCommit={(value) =>
                  updateRuntimeSetting('autonomous.pid.ki', value[0])
                }
                min={0}
                max={0.5}
                step={0.01}
              />
            </div>

            <div className='space-y-3'>
              <Label>Kd: {pidKd[0].toFixed(2)}</Label>
              <Slider
                value={pidKd}
                onValueChange={setPidKd}
                onValueCommit={(value) =>
                  updateRuntimeSetting('autonomous.pid.kd', value[0])
                }
                min={0}
                max={1}
                step={0.01}
              />
            </div>
          </div>
        </div>
      </section>
    </div>
  );
};

export default Settings;
