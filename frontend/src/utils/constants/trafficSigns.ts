import { IMAGES } from '@/assets';

export type SignalType = 'VERTICAL' | 'HORIZONTAL';

export interface TrafficSignal {
  id: string;
  label: string;
  type: SignalType;
  iconUrl?: string;
  confidence?: number;
}

export const TRAFFIC_SIGNS: TrafficSignal[] = [
  {
    id: 'stop',
    label: 'Parada obrigatória',
    iconUrl: IMAGES.trafficSigns.r1ParadaObrigatoria,
    type: 'VERTICAL',
  },
  {
    id: 'parking',
    label: 'Estacionamento',
    iconUrl: IMAGES.trafficSigns.r6bEstacionamentoRegulamentado,
    type: 'VERTICAL',
  },
  {
    id: 'dest_a',
    label: 'Destino A',
    iconUrl: '/icons/destino-a.png',
    type: 'VERTICAL',
  },
  {
    id: 'dest_b',
    label: 'Destino B',
    iconUrl: '/icons/destino-b.png',
    type: 'VERTICAL',
  },
  {
    id: 'dest_c',
    label: 'Destino C',
    iconUrl: '/icons/destino-c.png',
    type: 'VERTICAL',
  },
  {
    id: 'left_lane',
    label: 'Faixa à esquerda',
    iconUrl: '/icons/left-lane.png',
    type: 'HORIZONTAL',
  },
  {
    id: 'right_lane',
    label: 'Faixa à direita',
    iconUrl: '/icons/right-lane.png',
    type: 'HORIZONTAL',
  },
];

export const mockRecognizedSignals: Record<
  'VERTICAL' | 'HORIZONTAL',
  TrafficSignal[]
> = {
  VERTICAL: [
    {
      id: 'stop',
      label: 'Parada obrigatória',
      iconUrl: IMAGES.trafficSigns.r1ParadaObrigatoria,
      type: 'VERTICAL',
      confidence: 46,
    },
  ],
  HORIZONTAL: [
    {
      id: 'left_lane',
      label: 'Faixa à esquerda',
      type: 'HORIZONTAL',
      confidence: 92,
    },
    {
      id: 'right_lane',
      label: 'Faixa à direita',
      type: 'HORIZONTAL',
      confidence: 85,
    },
  ],
};
