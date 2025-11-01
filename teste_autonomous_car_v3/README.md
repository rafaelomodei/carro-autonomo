# Teste Autonomous Car V3

Este diretório contém um utilitário simples em Bash para enviar comandos de
movimentação para o servidor RedSocket a partir do teclado.

## Como usar

```bash
./run.sh
```

Por padrão o script tenta se conectar ao host `192.168.15.164` na porta
`8080`. Use as variáveis de ambiente `HOST` e `PORT` para personalizar o alvo:

```bash
HOST=127.0.0.1 PORT=9001 ./run.sh
```
```

Comandos disponíveis dentro do programa:

- **W**: envia o comando `forward` enquanto a tecla estiver sendo repetida.
- **S**: envia o comando `reverse` enquanto a tecla estiver sendo repetida.
- **A**: envia o comando `turn_left` enquanto a tecla estiver sendo repetida.
- **B**: envia o comando `turn_right` enquanto a tecla estiver sendo repetida.
- **Q**: encerra o programa.

Cada envio é exibido no terminal para facilitar a inspeção durante os testes.
