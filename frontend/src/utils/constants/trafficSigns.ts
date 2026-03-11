import type { TrafficSignId } from '@/lib/autonomous-car';
import { IMAGES } from '@/assets';

export interface TrafficSignalCatalogItem {
  id: TrafficSignId;
  label: string;
  iconUrl: string;
}

export const TRAFFIC_SIGN_CATALOG: Record<TrafficSignId, TrafficSignalCatalogItem> = {
  stop: {
    id: 'stop',
    label: 'Parada obrigatoria',
    iconUrl: IMAGES.trafficSigns.r1ParadaObrigatoria,
  },
  turn_left: {
    id: 'turn_left',
    label: 'Vire a esquerda',
    iconUrl: IMAGES.trafficSigns.r3bVireAEsquerda,
  },
  turn_right: {
    id: 'turn_right',
    label: 'Vire a direita',
    iconUrl: IMAGES.trafficSigns.r3aVireADireita,
  },
};

export const getTrafficSignCatalogItem = (
  signId: TrafficSignId
): TrafficSignalCatalogItem => TRAFFIC_SIGN_CATALOG[signId];
