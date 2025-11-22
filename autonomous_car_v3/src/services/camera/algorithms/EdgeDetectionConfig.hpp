#pragma once

#include <opencv2/core.hpp>

namespace autonomous_car::services::camera::algorithms {

/**
 * @brief Configurações de detecção de bordas ajustáveis via variáveis de ambiente.
 *
 * - EDGE_CANNY_THRESHOLD1: limiar inferior do detector de Canny (double).
 * - EDGE_CANNY_THRESHOLD2: limiar superior do detector de Canny (double).
 * - EDGE_CANNY_APERTURE: tamanho do kernel Sobel (3, 5 ou 7).
 * - EDGE_CANNY_L2GRADIENT: se "true", usa a norma L2 para o gradiente.
 * - EDGE_BLUR_KERNEL: kernel opcional (ímpar) aplicado antes do Canny para reduzir ruído.
 */
struct EdgeDetectionConfig {
    double canny_threshold1 = 40.0;
    double canny_threshold2 = 120.0;
    int canny_aperture = 3;
    bool canny_l2gradient = false;
    cv::Size blur_kernel = cv::Size(3, 3);

    static EdgeDetectionConfig LoadFromEnv();
};

} // namespace autonomous_car::services::camera::algorithms
