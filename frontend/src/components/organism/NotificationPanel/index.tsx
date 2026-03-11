import Image from 'next/image';
import { Label } from '@/components/ui/label';
import { useVehicleConfig } from '@/providers/VehicleConfigProvider';
import { getTrafficSignCatalogItem } from '@/utils/constants/trafficSigns';

const NotificationPanel = () => {
  const { trafficSignDetectionTelemetry } = useVehicleConfig();
  const activeDetection = trafficSignDetectionTelemetry?.active_detection ?? null;

  if (!activeDetection) {
    return (
      <div className='flex min-h-16 min-w-72 items-center justify-center rounded-md bg-foreground-black p-4 my-8 dark:bg-foreground-light'>
        <Label className='text-lg font-bold text-white dark:text-black'>
          Nenhuma sinalizacao confirmada
        </Label>
      </div>
    );
  }

  const sign = getTrafficSignCatalogItem(activeDetection.sign_id);
  const confidence = Math.round(activeDetection.confidence_score * 100);

  return (
    <div className='flex items-center justify-center gap-4 rounded-md w-fit min-w-72 p-4 my-8 min-h-16 bg-foreground-black dark:bg-foreground-light'>
      <Image
        width={60}
        height={60}
        src={sign.iconUrl}
        alt={sign.label}
      />

      <div className='flex flex-col'>
        <Label className='text-lg font-bold'>{activeDetection.display_label}</Label>
        <p className='text-sm text-white/80 dark:text-black/70'>
          Confirmada com {confidence}% de confianca
        </p>
      </div>
    </div>
  );
};

export { NotificationPanel };
