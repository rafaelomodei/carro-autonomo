'use client';

import { CarFront, Sparkles } from 'lucide-react';
import {
  createContext,
  useContext,
  useEffect,
  useRef,
  useState,
  ReactNode,
  Dispatch,
  SetStateAction,
} from 'react';
import { isValidWebSocketUrl } from '@/lib/websocket';

interface IDriveMode {
  id: string;
  label: string;
  icon: JSX.Element;
}

interface IPidControl {
  p: number;
  i: number;
  d: number;
}

export const VEHICLE_CAR_CONNECTION_URL = '';

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
  pidControl: IPidControl;
}

interface VehicleConfigContextProps {
  config: VehicleConfig;
  setConfig: Dispatch<SetStateAction<VehicleConfig>>;
  isConnected: boolean;
  currentFrame: string | null;
  sendMessage: (payload: CommandPayload) => void;
}

const VehicleConfigContext = createContext<VehicleConfigContextProps>(
  {} as VehicleConfigContextProps
);

export const useVehicleConfig = () => useContext(VehicleConfigContext);

type CommandPayload = {
  type: string;
  payload: Array<Record<string, unknown>>;
};

export const VehicleConfigProvider = ({
  children,
}: {
  children: ReactNode;
}) => {
  const [config, setConfig] = useState<VehicleConfig>({
    speedLimit: 50,
    driveMode: DriveMode.MANUAL,
    carConnection: undefined,
    steeringSensitivity: 0.5,
    pidControl: {
      p: 0.1,
      i: 0.1,
      d: 0.1,
    },
  });

  const socket = useRef<WebSocket | null>();
  const [isConnected, setIsConnected] = useState(false);
  const [currentFrame, setCurrentFrame] = useState<string | null>(null);

  useEffect(() => {
    if (!isValidWebSocketUrl(config.carConnection) || !config.carConnection)
      return;

    if (!socket.current) {
      socket.current = new WebSocket(config.carConnection);

      socket.current.onopen = () => {
        setIsConnected(true);
        console.log('Conexão WebSocket estabelecida.');
      };

      socket.current.onclose = () => {
        setIsConnected(false);
        console.log('Conexão WebSocket encerrada.');
        socket.current = null; // Reinicia a instância ao desconectar
      };

      socket.current.onmessage = async (event) => {
        const data = event.data;

        if (data instanceof Blob) {
          // Converte o Blob para uma string base64
          const reader = new FileReader();
          reader.onload = () => {
            const base64data = reader.result as string;
            setCurrentFrame(base64data);
          };
          reader.readAsDataURL(data); // Converte o Blob para uma URL de dados base64
        } else if (typeof data === 'string') {
          console.warn('Mensagem inesperada recebida como string:', data);
        }
      };

      socket.current.onerror = (error) => {
        console.error('Erro no WebSocket:', error);
      };
    }

    return () => {
      if (socket.current) {
        socket.current.close();
        socket.current = null;
      }
    };
  }, [config.carConnection]);

  const sendMessage = (payload: CommandPayload) => {
    console.info('Status do websocket: ', socket.current);
    if (socket.current?.readyState === WebSocket.OPEN) {
      socket.current.send(JSON.stringify(payload));
    } else {
      console.error('WebSocket não está conectado.');
    }
  };

  const handleKeyDown = (event: KeyboardEvent) => {
    console.info('Tecla prescionada: ', event.key);

    const keyMap: Record<string, CommandPayload> = {
      w: { type: 'commands', payload: [{ action: 'accelerate', speed: 200 }] }, // Frente
      s: { type: 'commands', payload: [{ action: 'accelerate', speed: -200 }] }, // Ré
      a: { type: 'commands', payload: [{ action: 'turn', direction: 'left' }] }, // Virar à esquerda
      d: {
        type: 'commands',
        payload: [{ action: 'turn', direction: 'right' }],
      }, // Virar à direita
      ' ': { type: 'commands', payload: [{ action: 'accelerate', speed: 0 }] }, // Parar (aceleração 0)
    };

    const command = keyMap[event.key?.toLowerCase()];
    if (command) {
      sendMessage(command);
    }
  };

  useEffect(() => {
    // Garante que o listener seja configurado globalmente
    window.addEventListener('keydown', handleKeyDown);
    return () => {
      window.removeEventListener('keydown', handleKeyDown);
    };
  }, []);

  return (
    <VehicleConfigContext.Provider
      value={{
        config,
        setConfig,
        isConnected,
        currentFrame,
        sendMessage,
      }}
    >
      {children}
    </VehicleConfigContext.Provider>
  );
};
