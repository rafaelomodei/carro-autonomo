import { useEffect, useState } from 'react';
import { WebSocketService } from '@/services/websocketService';

const WEBSOCKET_URL = 'ws://localhost:8080'; // URL do WebSocket

export const useWebSocket = () => {
  const [videoBlob, setVideoBlob] = useState<Blob | null>(null);
  const [vehicleData, setVehicleData] = useState<any>(null);
  const [isConnected, setIsConnected] = useState(false);
  const [webSocketService, setWebSocketService] = useState<WebSocketService | null>(null);

  useEffect(() => {
    const service = new WebSocketService(WEBSOCKET_URL);

    const handleVideoData = (videoData: Blob) => {
      setVideoBlob(videoData);
    };

    const handleVehicleData = (vehicleData: any) => {
      setVehicleData(vehicleData);
    };

    const handleConnectionStatusChange = (status: boolean) => {
      setIsConnected(status); // Atualiza o estado de conexão
    };

    service.connect(handleVideoData, handleVehicleData, handleConnectionStatusChange);
    setWebSocketService(service);

    return () => {
      service.disconnect();
    };
  }, []);

  const sendCommand = (command: any) => {
    if (webSocketService) {
      webSocketService.sendCommand(command);
    }
  };

  return {
    videoBlob,
    vehicleData,
    isConnected, // Agora o status de conexão está disponível
    sendCommand,
  };
};
