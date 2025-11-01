# Teste Autonomous Car V3

Este diretório contém um utilitário simples em Bash para enviar comandos de
movimentação para o servidor WebSocket do Autonomous Car diretamente a partir
do teclado.

## Como usar

```bash
./run.sh
```

Por padrão o script tenta se conectar ao host `192.168.15.164` na porta
`8080`. Use as variáveis de ambiente `HOST`, `PORT` e `INTERVAL` para
personalizar o alvo e a cadência de reenvio do comando ativo:

```bash
HOST=127.0.0.1 PORT=9001 INTERVAL=0.2 ./run.sh
```
```

Comandos disponíveis dentro do programa:

- **W**: ativa o comando `forward`.
- **S**: ativa o comando `backward`.
- **A**: ativa o comando `left`.
- **D**: ativa o comando `right`.
- **Barra de espaço**: envia `stop` e interrompe o reenvio automático.
- **Q**: encerra o programa.

O comando ativo é reenviado automaticamente a cada intervalo (padrão de 100
ms) até que outro comando seja escolhido ou que um `stop` seja enviado.
Cada envio é exibido no terminal para facilitar a inspeção durante os testes.
