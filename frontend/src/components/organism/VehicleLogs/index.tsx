'use client';

import { useState } from 'react';
import { ChevronDown, ChevronUp, MapPin } from 'lucide-react';
import { useDestination } from '@/providers/DestinationProvider';
import { useVehicleConfig } from '@/providers/VehicleConfigProvider';
import {
  formatConnectionStateLabel,
  formatDrivingModeLabel,
  formatStopReasonLabel,
  formatTrafficSignDetectorStateLabel,
  formatTrackingStateLabel,
  TelemetryReference,
} from '@/lib/autonomous-car';

const formatNumber = (value: number, digits = 2) => value.toFixed(digits);

const ReferenceSummary = ({
  label,
  reference,
}: {
  label: string;
  reference: TelemetryReference;
}) => (
  <div className='rounded-lg border p-3'>
    <div className='flex items-center justify-between'>
      <p className='font-semibold'>{label}</p>
      <span className='text-xs text-muted-foreground'>
        {reference.valid ? 'valida' : 'indisponivel'}
      </span>
    </div>
    <div className='mt-2 space-y-1 text-xs text-muted-foreground'>
      <p>Erro: {formatNumber(reference.steering_error_normalized)}</p>
      <p>Offset: {formatNumber(reference.lateral_offset_px)} px</p>
      {typeof reference.applied_weight === 'number' && (
        <p>Peso aplicado: {formatNumber(reference.applied_weight)}</p>
      )}
    </div>
  </div>
);

const VehicleLogs = () => {
  const [expanded, setExpanded] = useState(true);
  const { selectedDestination } = useDestination();
  const {
    autonomousControlTelemetry,
    connectionState,
    lastTelemetryAt,
    pendingAutonomousCommand,
    pendingDrivingMode,
    roadSegmentationTelemetry,
    trafficSignDetectionTelemetry,
  } = useVehicleConfig();

  const currentDrivingMode =
    autonomousControlTelemetry?.driving_mode ?? pendingDrivingMode ?? 'manual';
  const lastTelemetryLabel = lastTelemetryAt
    ? new Date(lastTelemetryAt).toLocaleTimeString('pt-BR')
    : 'Aguardando';

  return (
    <div className='rounded-2xl border bg-white/90 p-4 shadow-sm dark:bg-background/90'>
      <button
        type='button'
        className='flex w-full items-center justify-between text-left'
        onClick={() => setExpanded((current) => !current)}
      >
        <h2 className='font-bold'>Estado do veiculo</h2>
        {expanded ? (
          <ChevronUp className='h-4 w-4' />
        ) : (
          <ChevronDown className='h-4 w-4' />
        )}
      </button>

      {expanded && (
        <div className='mt-4 space-y-4 text-sm'>
          {selectedDestination && (
            <div>
              <p className='text-xs text-muted-foreground'>Destino selecionado</p>
              <div className='mt-1 flex items-center gap-2'>
                <MapPin className='h-4 w-4' />
                <span>{`Ponto ${selectedDestination}`}</span>
              </div>
            </div>
          )}

          <div className='grid gap-3 md:grid-cols-2'>
            <div className='rounded-lg border p-3'>
              <p className='text-xs text-muted-foreground'>Conexao</p>
              <p className='mt-1 font-semibold'>
                {formatConnectionStateLabel(connectionState)}
              </p>
              <p className='mt-1 text-xs text-muted-foreground'>
                Ultima telemetria: {lastTelemetryLabel}
              </p>
            </div>

            <div className='rounded-lg border p-3'>
              <p className='text-xs text-muted-foreground'>Modo atual</p>
              <p className='mt-1 font-semibold'>
                {formatDrivingModeLabel(currentDrivingMode)}
              </p>
              {pendingDrivingMode && (
                <p className='mt-1 text-xs text-muted-foreground'>
                  Troca de modo pendente
                </p>
              )}
            </div>
          </div>

          {autonomousControlTelemetry ? (
            <>
              <div className='grid gap-3 md:grid-cols-2'>
                <div className='rounded-lg border p-3'>
                  <p className='text-xs text-muted-foreground'>Rastreamento</p>
                  <p className='mt-1 font-semibold'>
                    {formatTrackingStateLabel(
                      autonomousControlTelemetry.tracking_state
                    )}
                  </p>
                  <p className='mt-1 text-xs text-muted-foreground'>
                    Start:{' '}
                    {autonomousControlTelemetry.autonomous_started
                      ? 'ativo'
                      : 'parado'}
                  </p>
                  {pendingAutonomousCommand && (
                    <p className='mt-1 text-xs text-muted-foreground'>
                      Comando autonomo pendente: {pendingAutonomousCommand}
                    </p>
                  )}
                </div>

                <div className='rounded-lg border p-3'>
                  <p className='text-xs text-muted-foreground'>Parada atual</p>
                  <p className='mt-1 font-semibold'>
                    {formatStopReasonLabel(autonomousControlTelemetry.stop_reason)}
                  </p>
                  <p className='mt-1 text-xs text-muted-foreground'>
                    Movimento: {autonomousControlTelemetry.motion_command}
                  </p>
                </div>
              </div>

              <div className='grid gap-3 md:grid-cols-3'>
                <ReferenceSummary
                  label='Near'
                  reference={autonomousControlTelemetry.references.near}
                />
                <ReferenceSummary
                  label='Mid'
                  reference={autonomousControlTelemetry.references.mid}
                />
                <ReferenceSummary
                  label='Far'
                  reference={autonomousControlTelemetry.references.far}
                />
              </div>
            </>
          ) : (
            <div className='rounded-lg border border-dashed p-3 text-sm text-muted-foreground'>
              Aguardando telemetria de controle autonomo para preencher os logs.
            </div>
          )}

          {roadSegmentationTelemetry && (
            <div className='rounded-lg border p-3'>
              <p className='text-xs text-muted-foreground'>Segmentacao</p>
              <div className='mt-2 grid gap-2 text-xs text-muted-foreground md:grid-cols-2'>
                <p>Fonte: {roadSegmentationTelemetry.source}</p>
                <p>
                  Pista encontrada:{' '}
                  {roadSegmentationTelemetry.lane_found ? 'sim' : 'nao'}
                </p>
                <p>
                  Confianca:{' '}
                  {formatNumber(roadSegmentationTelemetry.confidence_score)}
                </p>
                <p>
                  Erro lateral:{' '}
                  {formatNumber(
                    roadSegmentationTelemetry.steering_error_normalized
                  )}
                </p>
              </div>
            </div>
          )}

          {trafficSignDetectionTelemetry && (
            <div className='rounded-lg border p-3'>
              <p className='text-xs text-muted-foreground'>Sinalizacao</p>
              <div className='mt-2 grid gap-2 text-xs text-muted-foreground md:grid-cols-2'>
                <p>
                  Estado:{' '}
                  {formatTrafficSignDetectorStateLabel(
                    trafficSignDetectionTelemetry.detector_state
                  )}
                </p>
                <p>Deteccoes brutas: {trafficSignDetectionTelemetry.raw_detections.length}</p>
                <p>
                  Candidate:{' '}
                  {trafficSignDetectionTelemetry.candidate?.display_label ?? 'none'}
                </p>
                <p>
                  Active:{' '}
                  {trafficSignDetectionTelemetry.active_detection?.display_label ??
                    'none'}
                </p>
              </div>
            </div>
          )}
        </div>
      )}
    </div>
  );
};

export default VehicleLogs;
