'use client';

import {
  createContext,
  useContext,
  useState,
  ReactNode,
  Dispatch,
  SetStateAction,
} from 'react';

export enum DestinationType {
  POINT_A = 'A',
  POINT_B = 'B',
  POINT_C = 'C',
}

interface DestinationContextProps {
  selectedDestination: DestinationType | null;
  setDestination: Dispatch<SetStateAction<DestinationType | null>>;
}

const DestinationContext = createContext<DestinationContextProps>(
  {} as DestinationContextProps
);
export const useDestination = () => useContext(DestinationContext);

export const DestinationProvider = ({ children }: { children: ReactNode }) => {
  const [selectedDestination, setDestination] = useState<DestinationType | null>(null);

  return (
    <DestinationContext.Provider value={{ selectedDestination, setDestination }}>
      {children}
    </DestinationContext.Provider>
  );
};
