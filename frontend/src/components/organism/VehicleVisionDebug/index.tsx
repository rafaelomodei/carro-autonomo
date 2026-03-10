'use client';

import { useState } from 'react';
import Image from 'next/image';
import { useRouter } from 'next/navigation';
import { BiSolidJoystick } from 'react-icons/bi';
import { Button } from '@/components/ui/button';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Toggle } from '@/components/ui/toggle';
import { useVisionDebugStream } from '@/hooks/useVisionDebugStream';
import {
  formatConnectionStateLabel,
  formatStopReasonLabel,
  formatTrackingStateLabel,
  formatVisionDebugViewLabel,
  VISION_DEBUG_VIEW_IDS,
  VisionDebugViewId,
} from '@/lib/autonomous-car';
import { useVehicleConfig } from '@/providers/VehicleConfigProvider';
import VehicleLogs from '@/components/organism/VehicleLogs';

const DEFAULT_SELECTED_VIEWS: VisionDebugViewId[] = ['dashboard'];

const formatNumber = (value: number, digits = 2) => value.toFixed(digits);

const formatTimestampLabel = (timestamp: number | null) =>
  timestamp ? new Date(timestamp).toLocaleTimeString('pt-BR') : 'Aguardando';

const viewDescriptions: Record<VisionDebugViewId, string> = {
  raw: 'Camera normal sem overlays.',
  preprocess: 'ROI e preprocessamento da segmentacao.',
  mask: 'Mascara binaria gerada pelo pipeline.',
  annotated: 'Saida anotada com referencias e faixa.',
  dashboard: 'Painel completo igual ao debug do Raspberry.',
};

const VisionFrameCard = ({
  frameDataUrl,
  isConnected,
  lastFrameLabel,
  view,
  width,
  height,
}: {
  frameDataUrl: string | null;
  isConnected: boolean;
  lastFrameLabel: string;
  view: VisionDebugViewId;
  width?: number;
  height?: number;
}) => (
  <Card className='overflow-hidden border-border/70 bg-card/70'>
    <CardHeader className='space-y-2 border-b border-border/60 pb-4'>
      <div className='flex items-start justify-between gap-3'>
        <div>
          <CardTitle className='text-lg'>
            {formatVisionDebugViewLabel(view)}
          </CardTitle>
          <p className='mt-1 text-sm text-muted-foreground'>
            {viewDescriptions[view]}
          </p>
        </div>
        <div className='text-right text-xs text-muted-foreground'>
          <p>{width && height ? `${width}x${height}` : 'Sem frame'}</p>
          <p>Atualizado: {lastFrameLabel}</p>
        </div>
      </div>
    </CardHeader>
    <CardContent className='p-0'>
      <div className='flex aspect-video items-center justify-center bg-slate-950'>
        {frameDataUrl ? (
          <Image
            alt={`View ${formatVisionDebugViewLabel(view)}`}
            className='h-full w-full object-contain'
            height={height ?? 720}
            src={frameDataUrl}
            unoptimized
            width={width ?? 1280}
          />
        ) : (
          <div className='px-6 text-center text-sm text-slate-300'>
            {isConnected
              ? 'Aguardando o primeiro frame desta view.'
              : 'Selecione a view e aguarde a conexao do stream.'}
          </div>
        )}
      </div>
    </CardContent>
  </Card>
);

const VehicleVisionDebug = () => {
  const router = useRouter();
  const {
    autonomousControlTelemetry,
    config,
    connectionState,
    lastTelemetryAt,
    roadSegmentationTelemetry,
  } = useVehicleConfig();
  const [selectedViews, setSelectedViews] = useState<VisionDebugViewId[]>(
    DEFAULT_SELECTED_VIEWS
  );
  const {
    connectionState: visionConnectionState,
    frames,
    isConnected: isVisionConnected,
    lastFrameAt,
  } = useVisionDebugStream(config.connectionUrl, selectedViews);

  const lastTelemetryLabel = formatTimestampLabel(lastTelemetryAt);
  const lastFrameLabel = formatTimestampLabel(lastFrameAt);
  const activeViews = VISION_DEBUG_VIEW_IDS.filter((view) =>
    selectedViews.includes(view)
  );

  const toggleView = (view: VisionDebugViewId, pressed: boolean) => {
    setSelectedViews((currentViews) => {
      if (pressed) {
        return currentViews.includes(view) ? currentViews : [...currentViews, view];
      }

      return currentViews.filter((currentView) => currentView !== view);
    });
  };

  return (
    <div className='flex h-full w-full flex-col gap-6 overflow-y-auto px-2 py-4'>
      <div className='flex flex-wrap items-start justify-between gap-4'>
        <div className='max-w-3xl'>
          <h1 className='text-3xl font-bold'>Central de visao</h1>
          <p className='mt-2 text-sm text-muted-foreground'>
            O stream e sob demanda: o Raspberry so codifica as visoes que voce
            selecionar abaixo.
          </p>
        </div>

        <Button
          className='gap-3'
          variant='outline'
          onClick={() => router.push('/joystick')}
        >
          <BiSolidJoystick className='h-5 w-5' />
          Abrir joystick
        </Button>
      </div>

      <div className='grid gap-4 md:grid-cols-2 xl:grid-cols-4'>
        <Card className='border-border/70 bg-card/70'>
          <CardHeader className='pb-3'>
            <CardTitle className='text-base'>Controle WS</CardTitle>
          </CardHeader>
          <CardContent className='space-y-1 text-sm text-muted-foreground'>
            <p>{formatConnectionStateLabel(connectionState)}</p>
            <p>Ultima telemetria: {lastTelemetryLabel}</p>
          </CardContent>
        </Card>

        <Card className='border-border/70 bg-card/70'>
          <CardHeader className='pb-3'>
            <CardTitle className='text-base'>Stream de visao</CardTitle>
          </CardHeader>
          <CardContent className='space-y-1 text-sm text-muted-foreground'>
            <p>{formatConnectionStateLabel(visionConnectionState)}</p>
            <p>Ultimo frame: {lastFrameLabel}</p>
          </CardContent>
        </Card>

        <Card className='border-border/70 bg-card/70'>
          <CardHeader className='pb-3'>
            <CardTitle className='text-base'>Fonte atual</CardTitle>
          </CardHeader>
          <CardContent className='space-y-1 text-sm text-muted-foreground'>
            <p>{roadSegmentationTelemetry?.source ?? 'Aguardando backend'}</p>
            <p>
              Lane found:{' '}
              {roadSegmentationTelemetry?.lane_found ? 'sim' : 'nao'}
            </p>
          </CardContent>
        </Card>

        <Card className='border-border/70 bg-card/70'>
          <CardHeader className='pb-3'>
            <CardTitle className='text-base'>Servidor</CardTitle>
          </CardHeader>
          <CardContent className='space-y-1 text-sm text-muted-foreground'>
            <p className='break-all'>
              {config.connectionUrl || 'Servidor nao configurado'}
            </p>
            <p>{selectedViews.length} view(s) selecionada(s)</p>
          </CardContent>
        </Card>
      </div>

      <Card className='border-border/70 bg-card/70'>
        <CardHeader className='pb-4'>
          <CardTitle className='text-lg'>Views transmitidas</CardTitle>
          <p className='text-sm text-muted-foreground'>
            A conexao dedicada de stream usa `client:telemetry` e atualiza a
            assinatura sempre que a selecao muda.
          </p>
        </CardHeader>
        <CardContent className='flex flex-wrap gap-3'>
          {VISION_DEBUG_VIEW_IDS.map((view) => (
            <Toggle
              key={view}
              pressed={selectedViews.includes(view)}
              size='sm'
              variant='outline'
              onPressedChange={(pressed) => toggleView(view, pressed)}
            >
              {formatVisionDebugViewLabel(view)}
            </Toggle>
          ))}
        </CardContent>
      </Card>

      <div className='grid gap-6 xl:grid-cols-[1.6fr_0.95fr]'>
        <section className='space-y-4'>
          {activeViews.length === 0 ? (
            <Card className='border-dashed border-border/70 bg-card/50'>
              <CardContent className='flex min-h-72 items-center justify-center p-10 text-center text-sm text-muted-foreground'>
                Selecione ao menos uma view para abrir o socket de stream e
                comecar a receber os frames.
              </CardContent>
            </Card>
          ) : (
            <div className='grid gap-4 xl:grid-cols-2'>
              {activeViews.map((view) => {
                const frame = frames[view];
                const frameDataUrl = frame
                  ? `data:${frame.mime};base64,${frame.data}`
                  : null;

                return (
                  <VisionFrameCard
                    key={view}
                    frameDataUrl={frameDataUrl}
                    height={frame?.height}
                    isConnected={isVisionConnected}
                    lastFrameLabel={frame ? formatTimestampLabel(frame.timestamp_ms) : lastFrameLabel}
                    view={view}
                    width={frame?.width}
                  />
                );
              })}
            </div>
          )}
        </section>

        <aside className='space-y-4'>
          <VehicleLogs />

          <Card className='border-border/70 bg-card/70'>
            <CardHeader className='pb-3'>
              <CardTitle className='text-base'>Controle autonomo</CardTitle>
            </CardHeader>
            <CardContent className='space-y-1 text-sm text-muted-foreground'>
              {autonomousControlTelemetry ? (
                <>
                  <p>
                    Tracking:{' '}
                    {formatTrackingStateLabel(
                      autonomousControlTelemetry.tracking_state
                    )}
                  </p>
                  <p>
                    Start:{' '}
                    {autonomousControlTelemetry.autonomous_started
                      ? 'ativo'
                      : 'parado'}
                  </p>
                  <p>
                    Stop reason:{' '}
                    {formatStopReasonLabel(
                      autonomousControlTelemetry.stop_reason
                    )}
                  </p>
                  <p>Movimento: {autonomousControlTelemetry.motion_command}</p>
                  <p>
                    Preview error:{' '}
                    {formatNumber(autonomousControlTelemetry.preview_error)}
                  </p>
                </>
              ) : (
                <p>Aguardando snapshot de controle.</p>
              )}
            </CardContent>
          </Card>

          <Card className='border-border/70 bg-card/70'>
            <CardHeader className='pb-3'>
              <CardTitle className='text-base'>PID e pista</CardTitle>
            </CardHeader>
            <CardContent className='space-y-1 text-sm text-muted-foreground'>
              {autonomousControlTelemetry ? (
                <>
                  <p>
                    Lane disponivel:{' '}
                    {autonomousControlTelemetry.lane_available ? 'sim' : 'nao'}
                  </p>
                  <p>
                    Confidence:{' '}
                    {formatNumber(autonomousControlTelemetry.confidence_score)}
                  </p>
                  <p>
                    Heading:{' '}
                    {formatNumber(
                      autonomousControlTelemetry.heading_error_rad
                    )}
                  </p>
                  <p>
                    Curvatura:{' '}
                    {formatNumber(
                      autonomousControlTelemetry.curvature_indicator_rad
                    )}
                  </p>
                  <p>PID P: {formatNumber(autonomousControlTelemetry.pid.p)}</p>
                  <p>PID I: {formatNumber(autonomousControlTelemetry.pid.i)}</p>
                  <p>PID D: {formatNumber(autonomousControlTelemetry.pid.d)}</p>
                  <p>
                    Output:{' '}
                    {formatNumber(autonomousControlTelemetry.pid.output)}
                  </p>
                </>
              ) : (
                <p>Aguardando diagnostico do PID.</p>
              )}
            </CardContent>
          </Card>

          <Card className='border-border/70 bg-card/70'>
            <CardHeader className='pb-3'>
              <CardTitle className='text-base'>Segmentacao atual</CardTitle>
            </CardHeader>
            <CardContent className='space-y-1 text-sm text-muted-foreground'>
              {roadSegmentationTelemetry ? (
                <>
                  <p>
                    Lane ratio:{' '}
                    {formatNumber(roadSegmentationTelemetry.lane_center_ratio)}
                  </p>
                  <p>
                    Steering error:{' '}
                    {formatNumber(
                      roadSegmentationTelemetry.steering_error_normalized
                    )}
                  </p>
                  <p>
                    Offset:{' '}
                    {formatNumber(roadSegmentationTelemetry.lateral_offset_px)} px
                  </p>
                  <p>
                    Heading valid:{' '}
                    {roadSegmentationTelemetry.heading_valid ? 'sim' : 'nao'}
                  </p>
                  <p>
                    Curvature valid:{' '}
                    {roadSegmentationTelemetry.curvature_valid ? 'sim' : 'nao'}
                  </p>
                </>
              ) : (
                <p>Aguardando telemetria de segmentacao.</p>
              )}
            </CardContent>
          </Card>

          <Card className='border-border/70 bg-card/70'>
            <CardHeader className='pb-3'>
              <CardTitle className='text-base'>Trajetoria projetada</CardTitle>
            </CardHeader>
            <CardContent className='space-y-2 text-sm text-muted-foreground'>
              {autonomousControlTelemetry?.projected_path?.length ? (
                autonomousControlTelemetry.projected_path.map((point, index) => (
                  <div
                    key={`${point.x}-${point.y}-${index}`}
                    className='rounded-lg border border-border/60 px-3 py-2'
                  >
                    <p className='font-medium text-foreground'>Ponto {index + 1}</p>
                    <p>X: {formatNumber(point.x, 3)}</p>
                    <p>Y: {formatNumber(point.y, 3)}</p>
                  </div>
                ))
              ) : (
                <p>Aguardando pontos publicados pelo backend.</p>
              )}
            </CardContent>
          </Card>
        </aside>
      </div>
    </div>
  );
};

export default VehicleVisionDebug;
