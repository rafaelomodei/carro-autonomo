import { isValidWebSocketUrl } from '@/lib/websocket';
import { useEffect, useRef, useState } from 'react';

type CommandPayload = {
  type: string;
  payload: Array<Record<string, unknown>>;
};

// type Direction = 'forward' | 'backward' | 'left' | 'right';

export const useCar = (url?: string) => {
  const socket = useRef<WebSocket | null>(null);
  const [isConnected, setIsConnected] = useState(false);
  const [messages, setMessages] = useState<string[]>([]);

  useEffect(() => {
    if (!isValidWebSocketUrl(url) || !url) return;

    socket.current = new WebSocket(url);

    socket.current.onopen = () => {
      setIsConnected(true);
      console.log('Conexão WebSocket estabelecida.');
    };

    socket.current.onclose = () => {
      setIsConnected(false);
      console.log('Conexão WebSocket encerrada.');
    };

    socket.current.onmessage = (event) => {
      console.log('Mensagem recebida:', event.data);
      setMessages((prev) => [...prev, event.data]);
    };

    socket.current.onerror = (error) => {
      console.error('Erro no WebSocket:', error);
    };

    return () => {
      socket.current?.close();
    };
  }, [url]);

  const sendMessage = (payload: CommandPayload) => {
    if (socket.current?.readyState === WebSocket.OPEN) {
      socket.current.send(JSON.stringify(payload));
    } else {
      console.error('WebSocket não está conectado.');
    }
  };

  // Mapeia teclas para comandos e ajusta o formato do payload
  const handleKeyDown = (event: KeyboardEvent) => {
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

    const command = keyMap[event.key.toLowerCase()];
    if (command) {
      sendMessage(command);
    }
  };

  useEffect(() => {
    window.addEventListener('keydown', handleKeyDown);
    return () => {
      window.removeEventListener('keydown', handleKeyDown);
    };
  }, [handleKeyDown]);

  return { isConnected, messages, url };
};
