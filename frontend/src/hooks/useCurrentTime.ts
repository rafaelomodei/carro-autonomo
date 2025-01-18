import { getCurrentFormattedTime } from '@/lib/time';
import { useState, useEffect } from 'react';

export const useCurrentTime = () => {
  const [currentTime, setCurrentTime] = useState<string>(
    getCurrentFormattedTime()
  );

  useEffect(() => {
    const interval = setInterval(() => {
      setCurrentTime(getCurrentFormattedTime());
    }, 1000);

    return () => clearInterval(interval);
  }, []);

  return currentTime;
};
