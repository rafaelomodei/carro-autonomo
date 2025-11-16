#pragma once

#include <opencv2/core.hpp>

namespace autonomous_car::services::camera {

/**
 * @brief Configurações de processamento de pista controladas por variáveis de ambiente.
 *
 * Cada campo possui uma variável correspondente. Ao alterar o valor da env não é preciso
 * recompilar o binário, basta reiniciar o processo para que o `LaneDetector` utilize os
 * novos parâmetros.
 *
 * - LANE_ROI_KEEP_RATIO: porcentagem inferior do frame mantida no recorte (0-1).
 * - LANE_GAUSSIAN_KERNEL: tamanho do kernel gaussiano (ex.: "5" ou "5x7").
 * - LANE_GAUSSIAN_SIGMA: sigma aplicado no eixo X para o blur gaussiano.
 * - LANE_HSV_LOW: valores HSV mínimos para destacar o EVA (formato "H,S,V").
 * - LANE_HSV_HIGH: valores HSV máximos usados na máscara (formato "H,S,V").
 * - LANE_MORPH_KERNEL: tamanho do kernel morfológico (ex.: "9" ou "9x11").
 * - LANE_MORPH_ITERATIONS: quantidade de iterações da operação morfológica.
 * - LANE_MIN_CONTOUR_AREA: área mínima do contorno considerada como pista (px²).
 */
struct LaneFilterConfig {
    double roi_keep_ratio = 0.5;
    cv::Size gaussian_kernel = cv::Size(5, 5);
    double gaussian_sigma = 1.2;
    cv::Scalar hsv_low = cv::Scalar(0, 0, 0);
    cv::Scalar hsv_high = cv::Scalar(180, 110, 200);
    cv::Size morph_kernel = cv::Size(5, 5);
    int morph_iterations = 1;
    double min_contour_area = 400.0;

    static LaneFilterConfig LoadFromEnv();
};

} // namespace autonomous_car::services::camera

