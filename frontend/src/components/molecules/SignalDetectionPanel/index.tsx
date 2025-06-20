'use client';

import Image from 'next/image';
import { Badge } from '@/components/ui/badge';
import { TrafficSignal } from '@/utils/constants/trafficSigns';

interface SignalItem extends TrafficSignal {
  color?: 'green' | 'orange' | 'red' | 'gray';
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
      {signs.map((item, index) => (
        <div key={index} className='flex items-center gap-2'>
          <Badge className={`rounded-md ${getBadgeColor(item.confidence!)}`}>
            {item.confidence}%
          </Badge>
          <p>|</p>
          <p>{item.label}</p>
          {item.iconUrl && (
            <Image src={item.iconUrl} alt={item.label} width={28} height={28} />
          )}
        </div>
      ))}
    </div>
  );
};

export default SignalDetectionPanel;
