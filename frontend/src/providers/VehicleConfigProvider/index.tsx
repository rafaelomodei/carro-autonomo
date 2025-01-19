'use client';

import { CarFront, Sparkles } from 'lucide-react';
import {
  createContext,
  useContext,
  useState,
  ReactNode,
  Dispatch,
  SetStateAction,
} from 'react';

interface IDriveMode {
  id: string;
  label: string;
  icon: JSX.Element;
}

export const DriveMode: Record<'MANUAL' | 'AUTONOMOUS', IDriveMode> = {
  MANUAL: {
    id: 'MANUAL',
    label: 'Condução manual',
    icon: <CarFront className='w-4 h-4' />,
  },
  AUTONOMOUS: {
    id: 'AUTONOMOUS',
    label: 'Condução autônoma',
    icon: <Sparkles className='w-4 h-4' />,
  },
};

export interface VehicleConfig {
  speedLimit: number;
  driveMode: IDriveMode;
  carConnection?: string;
}

interface VehicleConfigContextProps {
  config: VehicleConfig;
  setConfig: Dispatch<SetStateAction<VehicleConfig>>;
}

const VehicleConfigContext = createContext<VehicleConfigContextProps>(
  {} as VehicleConfigContextProps
);

export const useVehicleConfig = () => useContext(VehicleConfigContext);

export const VehicleConfigProvider = ({
  children,
}: {
  children: ReactNode;
}) => {
  const [url, setUrl] = useState<string>('');

  const [config, setConfig] = useState<VehicleConfig>({
    speedLimit: 50,
    driveMode: DriveMode.MANUAL,
    carConnection: undefined,
  });

  return (
    <VehicleConfigContext.Provider value={{ config, setConfig }}>
      {children}
    </VehicleConfigContext.Provider>
  );
};
