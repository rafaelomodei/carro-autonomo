export class WebSocketService {
    private socket: WebSocket | null = null;
    private readonly url: string;
  
    constructor(url: string) {
      this.url = url;
    }
  
    connect(
      onVideoData: (videoData: Blob) => void,
      onVehicleData: (vehicleData: any) => void,
      onConnectionStatusChange: (status: boolean) => void // Callback para informar mudanças de status
    ): void {
      if (!this.socket || this.socket.readyState === WebSocket.CLOSED) {
        this.socket = new WebSocket(this.url);
  
        this.socket.onopen = () => {
          console.log('WebSocket connection opened.');
          onConnectionStatusChange(true); // Informa que a conexão foi aberta
        };
  
        this.socket.onmessage = (event) => {
          if (typeof event.data === 'string') {
            const data = JSON.parse(event.data);
            onVehicleData(data); // Se for JSON, tratamos como dados do veículo.
          } else if (event.data instanceof Blob) {
            onVideoData(event.data); // Se for Blob, tratamos como vídeo.
          }
        };
  
        this.socket.onerror = (error) => {
          console.error('WebSocket error: ', error);
        };
  
        this.socket.onclose = () => {
          console.log('WebSocket connection closed.');
          onConnectionStatusChange(false); // Informa que a conexão foi fechada
        };
      }
    }
  
    sendCommand(command: any): void {
      if (this.socket && this.socket.readyState === WebSocket.OPEN) {
        this.socket.send(JSON.stringify(command));
        console.log('Comando enviado:', command);
      } else {
        console.error('WebSocket não está aberto para enviar comandos.');
      }
    }
  
    disconnect(): void {
      if (this.socket) {
        this.socket.close();
        this.socket = null;
      }
    }
  }
  