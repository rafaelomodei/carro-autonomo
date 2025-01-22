#include "VideoStreamHandler.h"
#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <vector>

VideoStreamHandler::VideoStreamHandler(int cameraIndex, const std::function<void(const std::string &)> &sendFrameCallback)
    : cameraIndex(cameraIndex), sendFrameCallback(sendFrameCallback), isStreaming(false) {}

VideoStreamHandler::~VideoStreamHandler() {
  stopStreaming();
}

void VideoStreamHandler::streamLoop() {
  cv::VideoCapture cap(cameraIndex);

  // Configuração adicional
  cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
  cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
  cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
  cap.set(cv::CAP_PROP_FPS, 30);

  if (!cap.isOpened()) {
    std::cerr << "Erro ao abrir a câmera no índice: " << cameraIndex << std::endl;
    return;
  }

  std::cout << "Câmera aberta com sucesso! Índice: " << cameraIndex << std::endl;

  cv::Mat frame;
  while (isStreaming) {
    cap >> frame;
    if (frame.empty()) {
      std::cerr << "Frame vazio capturado, continuando..." << std::endl;
      continue;
    }

    // Converte o frame para JPEG
    std::vector<unsigned char> buffer;
    cv::imencode(".jpg", frame, buffer);

    // Converte para string e envia via callback
    std::string frameBase64(buffer.begin(), buffer.end());
    if (sendFrameCallback) {
      sendFrameCallback(frameBase64);
    }

    // Limitar taxa de quadros
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
  }

  cap.release();
}

void VideoStreamHandler::stopStreaming() {
  if (isStreaming) {
    isStreaming = false;
    if (streamingThread.joinable()) {
      streamingThread.join();
    }
  }
}

void VideoStreamHandler::streamLoop() {
  cv::VideoCapture cap(cameraIndex);
  if (!cap.isOpened()) {
    std::cerr << "Erro ao abrir a câmera." << std::endl;
    return;
  }

  cv::Mat frame;
  while (isStreaming) {
    cap >> frame; // Captura o frame da câmera
    if (frame.empty()) {
      std::cerr << "Frame vazio capturado." << std::endl;
      continue;
    }

    // Converte o frame para JPEG
    std::vector<unsigned char> buffer;
    cv::imencode(".jpg", frame, buffer);

    // Converte o buffer para base64
    std::string frameBase64 = std::string(buffer.begin(), buffer.end());

    // Envia o frame usando o callback
    if (sendFrameCallback) {
      sendFrameCallback(frameBase64);
    }

    // Adiciona um pequeno delay para limitar a taxa de quadros
    std::this_thread::sleep_for(std::chrono::milliseconds(30)); // ~33 FPS
  }

  cap.release();
}
