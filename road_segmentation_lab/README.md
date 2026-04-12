# Road Segmentation Lab

Aplicacao standalone em **C++17 + OpenCV** para validar a segmentacao da via do carro usando o mesmo core compartilhado consumido pelo `autonomous_car_v3`.

## Objetivo

- Processar **imagem**, **vídeo** ou **câmera**.
- Segmentar a pista escura sobre fundo claro como **região coesa**.
- Detectar as duas bordas laterais **reais** da via por scanlines horizontais.
- Excluir explicitamente o capô usando uma máscara fixa.
- Exibir um painel de debug `2x2` com as etapas do pipeline.
- Calcular métricas para o controle futuro:
  - `lane_center_ratio` em `[0,1]`
  - `steering_error_normalized` em `[-1,1]`
  - referencias `far`, `mid` e `near` para lookahead futuro
  - `heading_error_rad` e `curvature_indicator_rad`

## Build

```bash
./build.sh
```

O laboratorio continua standalone, mas agora compila o core compartilhado em `../shared/road_segmentation`.
O script usa `Ninja` por padrao e executa o build com paralelismo automatico.

Se precisar instalar no Raspberry Pi:

```bash
sudo apt install ninja-build
```

Tambem e possivel ajustar a quantidade de jobs:

```bash
BUILD_JOBS=4 ./build.sh
```

## Execução

Com câmera:

```bash
./run.sh
```

Com vídeo ou imagem:

```bash
./run.sh --input /caminho/arquivo.mp4
./run.sh --input /caminho/imagem.png
```

Selecionando outra câmera:

```bash
./run.sh --camera-index 1
```

Usando outro `.env`:

```bash
./run.sh --input /caminho/video.mp4 --config /caminho/road_segmentation_lab.env
```

## Atalhos

- `q` ou `ESC`: sair
- `p`: pausar/retomar vídeo ou câmera
- `n`: avançar um frame quando pausado
- `r`: recarregar o `.env` sem reiniciar

## Configuração

O arquivo padrão fica em [config/road_segmentation_lab.env](/media/omodei/arquivos/omodei/Documentos/projetos/carro-autonomo/road_segmentation_lab/config/road_segmentation_lab.env).

Pipeline atual:

- ROI retangular para cortar a metade inferior da imagem
- faixas horizontais configuraveis `far`, `mid` e `near` dentro da ROI util
- ROI poligonal trapezoidal para focar no corredor útil
- máscara fixa do capô
- blur leve
- segmentação configurável, com `GRAY_THRESHOLD` como padrão
- morfologia `closing + opening`
- seleção do componente principal da pista
- extração das bordas reais por scanlines
- extração da `centerline` e agregação em tres referencias programaveis
- preenchimento da área da pista e score de confiança na saída anotada

Parâmetros públicos:

- `LANE_RESIZE_ENABLED`
- `LANE_TARGET_WIDTH`
- `LANE_TARGET_HEIGHT`
- `LANE_ROI_TOP_RATIO`
- `LANE_ROI_BOTTOM_RATIO`
- `LANE_REFERENCE_FAR_TOP_RATIO`
- `LANE_REFERENCE_FAR_BOTTOM_RATIO`
- `LANE_REFERENCE_MID_TOP_RATIO`
- `LANE_REFERENCE_MID_BOTTOM_RATIO`
- `LANE_REFERENCE_NEAR_TOP_RATIO`
- `LANE_REFERENCE_NEAR_BOTTOM_RATIO`
- `LANE_ROI_POLYGON_TOP_WIDTH_RATIO`
- `LANE_ROI_POLYGON_BOTTOM_WIDTH_RATIO`
- `LANE_ROI_POLYGON_CENTER_X_RATIO`
- `LANE_HOOD_MASK_ENABLED`
- `LANE_HOOD_MASK_WIDTH_RATIO`
- `LANE_HOOD_MASK_HEIGHT_RATIO`
- `LANE_HOOD_MASK_CENTER_X_RATIO`
- `LANE_HOOD_MASK_BOTTOM_OFFSET_RATIO`
- `LANE_GAUSSIAN_ENABLED`
- `LANE_GAUSSIAN_KERNEL`
- `LANE_GAUSSIAN_SIGMA`
- `LANE_CLAHE_ENABLED`
- `LANE_SEGMENTATION_MODE`
- `LANE_HSV_LOW`
- `LANE_HSV_HIGH`
- `LANE_GRAY_THRESHOLD`
- `LANE_ADAPTIVE_BLOCK_SIZE`
- `LANE_ADAPTIVE_C`
- `LANE_MORPH_ENABLED`
- `LANE_MORPH_KERNEL`
- `LANE_MORPH_ITERATIONS`
- `LANE_MIN_CONTOUR_AREA`
- `LANE_CALIBRATION_FILE`

## Estratégias de segmentação

- `HSV_DARK`
- `GRAY_THRESHOLD`
- `LAB_DARK`
- `ADAPTIVE_GRAY`

## Lookahead triplet

As referencias `far`, `mid` e `near` sao sempre definidas **dentro da ROI atual**. Ou seja:

- `LANE_ROI_TOP_RATIO` e `LANE_ROI_BOTTOM_RATIO` escolhem quanto da imagem entra na analise
- as variaveis `LANE_REFERENCE_*` subdividem apenas essa ROI util

Defaults atuais:

- `far: 0.00 -> 0.33`
- `mid: 0.33 -> 0.66`
- `near: 0.66 -> 1.00`

Leitura operacional para o controle futuro:

- `near`: corrige o erro lateral imediato do carro
- `mid`: ajuda a alinhar a orientacao da trajetoria
- `far`: antecipa tendencia de curva

Convencoes de sinal:

- `lateral_offset_px` e `steering_error_normalized` positivos significam pista deslocada para a direita
- `heading_error_rad` positivo significa que a trajetoria aponta para a direita
- `curvature_indicator_rad` positivo significa tendencia de curva para a direita

Cada faixa retorna:

- ponto medio da `centerline` dentro da faixa
- `center_ratio`
- `lateral_offset_px`
- `steering_error_normalized`
- `sample_count`
- `valid`

As metricas derivadas publicas sao:

- `heading_error_rad`, calculado com o segmento `near -> mid`
- `curvature_indicator_rad`, calculado pela diferenca angular entre `near -> mid` e `mid -> far`

## Saída anotada

A janela final desenha:

- bandas horizontais `FAR`, `MID` e `NEAR` sobre a ROI
- pontos de referencia `FAR`, `MID` e `NEAR`
- borda esquerda em azul
- borda direita em amarelo
- linha central da pista em verde
- área da pista preenchida com transparência
- linha central da imagem
- erro lateral, heading, curvature e score de confiança

## Testes

Depois do build:

```bash
cd build
ctest --output-on-failure
```
