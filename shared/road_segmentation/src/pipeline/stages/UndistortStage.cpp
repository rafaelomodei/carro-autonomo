#include "pipeline/stages/UndistortStage.hpp"

#include <filesystem>

#include <opencv2/calib3d.hpp>
#include <opencv2/core/persistence.hpp>

namespace road_segmentation_lab::pipeline::stages {

UndistortStage::UndistortStage(const std::string &calibration_file) {
    if (calibration_file.empty()) {
        status_message_ = "Calibracao desativada";
        return;
    }

    if (!std::filesystem::exists(calibration_file)) {
        status_message_ = "Arquivo de calibracao nao encontrado";
        return;
    }

    cv::FileStorage storage(calibration_file, cv::FileStorage::READ);
    if (!storage.isOpened()) {
        status_message_ = "Falha ao abrir arquivo de calibracao";
        return;
    }

    storage["camera_matrix"] >> camera_matrix_;
    if (camera_matrix_.empty()) {
        storage["cameraMatrix"] >> camera_matrix_;
    }

    storage["dist_coeffs"] >> dist_coeffs_;
    if (dist_coeffs_.empty()) {
        storage["distortion_coefficients"] >> dist_coeffs_;
    }
    if (dist_coeffs_.empty()) {
        storage["distCoeffs"] >> dist_coeffs_;
    }

    if (camera_matrix_.empty() || dist_coeffs_.empty()) {
        status_message_ = "Parametros de calibracao invalidos";
        camera_matrix_.release();
        dist_coeffs_.release();
        return;
    }

    active_ = true;
    status_message_ = "Calibracao ativa";
}

cv::Mat UndistortStage::apply(const cv::Mat &frame) const {
    if (frame.empty()) {
        return cv::Mat();
    }

    if (!active_) {
        return frame.clone();
    }

    cv::Mat corrected;
    cv::undistort(frame, corrected, camera_matrix_, dist_coeffs_);
    return corrected;
}

bool UndistortStage::isActive() const noexcept { return active_; }

const std::string &UndistortStage::statusMessage() const noexcept { return status_message_; }

} // namespace road_segmentation_lab::pipeline::stages
