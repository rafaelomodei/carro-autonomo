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
  steeringSensitivity: number; // varia entre 0 a 1
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
  const [config, setConfig] = useState<VehicleConfig>({
    speedLimit: 50,
    driveMode: DriveMode.MANUAL,
    carConnection: 'ws://192.168.3.113:8080',
    steeringSensitivity: 0.5,
  });

  return (
    <VehicleConfigContext.Provider value={{ config, setConfig }}>
      {children}
    </VehicleConfigContext.Provider>
  );
};
