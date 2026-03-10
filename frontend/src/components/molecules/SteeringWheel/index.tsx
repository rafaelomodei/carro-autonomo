import { GiSteeringWheel } from 'react-icons/gi';

interface SteeringWheelProps {
  steeringCommand: number;
}

const SteeringWheel = ({ steeringCommand }: SteeringWheelProps) => {
  const angle = Math.round(steeringCommand * 90);

  return (
    <div className='flex flex-col items-center justify-center gap-3 rounded-2xl border bg-white/80 p-6 text-center shadow-sm dark:bg-background/80'>
      <div
        className='text-7xl transition-transform duration-200 ease-in-out'
        style={{
          transform: `rotate(${angle}deg)`,
        }}
      >
        <GiSteeringWheel />
      </div>
      <div className='space-y-1'>
        <p className='text-sm font-semibold'>Comando de direcao</p>
        <p className='text-2xl font-bold'>{steeringCommand.toFixed(2)}</p>
        <p className='text-xs text-muted-foreground'>Rotacao visual: {angle}°</p>
      </div>
    </div>
  );
};

export default SteeringWheel;
