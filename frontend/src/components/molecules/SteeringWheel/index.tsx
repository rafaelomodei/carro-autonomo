import React from 'react';
import { GiSteeringWheel } from 'react-icons/gi';

interface SteeringWheelProps {
  angle: number;
}

const SteeringWheel: React.FC<SteeringWheelProps> = ({ angle }) => {
  return (
    <div className='absolute right-4 mt-24 flex items-center gap-4 bg-white dark:bg-background p-2 rounded-lg'>
      <div
        className='text-6xl transition-transform duration-200 ease-in-out'
        style={{
          transform: `rotate(${angle}deg)`,
        }}
      >
        <GiSteeringWheel />
      </div>
      <p className='text-sm font-bold'>Ângulo: {angle}°</p>
    </div>
  );
};

export default SteeringWheel;
