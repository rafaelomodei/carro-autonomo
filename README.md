# Carro Autônomo

Este repositório hospeda um projeto de **TCC (Trabalho de Conclusão de Curso)** que visa a construção de um pequeno veículo autônomo. O objetivo é combinar visão computacional e controle de motores para permitir que o carro navegue de forma autônoma em pistas simples.

## Visão Geral

O sistema é baseado em um *Raspberry Pi*, responsável por capturar imagens da câmera, processar as informações de visão e controlar a direção e velocidade dos motores via controle PID. Para classificação de imagens e reconhecimento de sinalizações utiliza-se o [Edge Impulse](https://www.edgeimpulse.com), enquanto a detecção de faixas é feita com algoritmos do OpenCV.

Entre as funcionalidades previstas estão:

- Reconhecimento de placas de trânsito (ex: "Pare", "Siga", "Direita"/"Esquerda");
- Detecção de faixas de rolamento para manutenção na pista;
- Ajuste automático de direção e velocidade por meio de PID;
- Interface web para monitoramento e configuração do veículo.

O desenvolvimento ainda está em andamento e novas funcionalidades podem ser adicionadas ao longo do tempo.

## Estrutura do Repositório

- **`autonomous_car/`** &ndash; Aplicação em C++ que roda no Raspberry Pi. Faz captura de vídeo, expõe um servidor WebSocket e integra modelos do Edge Impulse.
- **`autonomous_car_v2/`** &ndash; Versão simplificada com foco na transmissão via WebSocket e controle dos motores com `pigpio`.
- **`autonomous_car_v3/`** &ndash; Nova versão em C++17 com servidor WebSocket próprio e controle de motores via WiringPi.
- **`frontend/`** &ndash; Projeto em Next.js/React responsável pela interface web.
- **`edgeImpulse/`** &ndash; Arquivos de modelos e dados exportados do Edge Impulse.
- **`testPy/`** &ndash; Scripts de testes em Python para validação de GPIO e outros componentes.

Cada pasta possui seu próprio README com instruções detalhadas.

## Como Executar

Para configurar e compilar a aplicação em C++:

```bash
cd autonomous_car
./setup.sh    # instala dependências
./build.sh    # gera executáveis
./start.sh    # inicia o programa
```

A interface web pode ser iniciada com:

```bash
cd frontend
npm install   # ou pnpm install
npm run dev   # inicia em modo de desenvolvimento
```

## Status do Projeto

Este é um projeto de TCC em desenvolvimento. Os códigos e diagramas neste repositório evoluirão conforme novas etapas forem concluídas.

## Contribuições

Sugestões e contribuições são bem-vindas! Abra uma *issue* ou *pull request* para discutir melhorias.

