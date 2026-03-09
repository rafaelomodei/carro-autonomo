# Road Segmentation Lab

Protótipo standalone em **C++17 + OpenCV** para validar a segmentação da via do carro antes da integração no `autonomous_car_v3`.

## Objetivo

- Processar **imagem**, **vídeo** ou **câmera**.
- Segmentar a pista escura sobre fundo claro como **região coesa**.
- Detectar as duas bordas laterais **reais** da via por scanlines horizontais.
- Excluir explicitamente o capô usando uma máscara fixa.
- Exibir um painel de debug `2x2` com as etapas do pipeline.
- Calcular métricas para o controle futuro:
  - `lane_center_ratio` em `[0,1]`
  - `steering_error_normalized` em `[-1,1]`

## Build

```bash
./build.sh
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
- ROI poligonal trapezoidal para focar no corredor útil
- máscara fixa do capô
- blur leve
- segmentação configurável, com `GRAY_THRESHOLD` como padrão
- morfologia `closing + opening`
- seleção do componente principal da pista
- extração das bordas reais por scanlines
- preenchimento da área da pista e score de confiança na saída anotada

Parâmetros públicos:

- `LANE_RESIZE_ENABLED`
- `LANE_TARGET_WIDTH`
- `LANE_TARGET_HEIGHT`
- `LANE_ROI_TOP_RATIO`
- `LANE_ROI_BOTTOM_RATIO`
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

## Saída anotada

A janela final desenha:

- borda esquerda em azul
- borda direita em amarelo
- linha central da pista em verde
- área da pista preenchida com transparência
- linha central da imagem
- erro lateral e score de confiança

## Testes

Depois do build:

```bash
cd build
ctest --output-on-failure
```
