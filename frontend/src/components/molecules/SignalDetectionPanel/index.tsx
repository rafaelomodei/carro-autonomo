'use client';

import Image from 'next/image';
import { Badge } from '@/components/ui/badge';
import { TrafficSignId } from '@/lib/autonomous-car';
import { getTrafficSignCatalogItem } from '@/utils/constants/trafficSigns';

interface SignalItem {
  sign_id: TrafficSignId;
  display_label: string;
  confidence_score: number;
  caption?: string;
}

interface SignalDetectionPanelProps {
  signs: SignalItem[];
}

const getBadgeColor = (confidence: number): string => {
  if (confidence >= 90) return '!bg-green-600 !text-white';
  if (confidence >= 60) return '!bg-yellow-400 !text-black';
  if (confidence >= 50) return '!bg-orange-500 !text-white';

  return '!bg-red-600 !text-white';
};

const SignalDetectionPanel: React.FC<SignalDetectionPanelProps> = ({
  signs,
}) => {
  return (
    <div className='flex flex-col gap-2'>
      {signs.map((item, index) => {
        const catalogItem = getTrafficSignCatalogItem(item.sign_id);
        const confidence = Math.round(item.confidence_score * 100);

        return (
          <div key={`${item.sign_id}-${index}`} className='flex items-center gap-2'>
            <Badge className={`rounded-md ${getBadgeColor(confidence)}`}>
              {confidence}%
            </Badge>
            <p>|</p>
            <p>{item.display_label}</p>
            {catalogItem.iconUrl && (
              <Image
                src={catalogItem.iconUrl}
                alt={catalogItem.label}
                width={28}
                height={28}
              />
            )}
            {item.caption ? (
              <p className='text-xs text-muted-foreground'>{item.caption}</p>
            ) : null}
          </div>
        );
      })}
    </div>
  );
};

export default SignalDetectionPanel;
