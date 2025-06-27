#include "VideoStreamHandler.h"
#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <vector>

VideoStreamHandler::VideoStreamHandler(
    int                                             cameraIndex,
    const std::function<void(const std::string &)> &sendFrameCallback)
    : cameraIndex(cameraIndex),
      sendFrameCallback(sendFrameCallback),
      isStreaming(false) {}

VideoStreamHandler::~VideoStreamHandler() {
  stopStreaming();
}

void VideoStreamHandler::startStreaming() {
  if (isStreaming)
    return;

  isStreaming     = true;
  streamingThread = std::thread(&VideoStreamHandler::streamLoop, this);
}

void VideoStreamHandler::stopStreaming() {
  if (!isStreaming)
    return;
  isStreaming = false;
  if (streamingThread.joinable())
    streamingThread.join();
}

void VideoStreamHandler::streamLoop() {
  /* força V4L2 em vez de GStreamer */
  cv::VideoCapture cap;
  if (!cap.open(cameraIndex, cv::CAP_V4L2)) {
    std::cerr << "Aviso: falhou CAP_V4L2, tentando padrão\n";
    cap.open(cameraIndex);
  }

  cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
  cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
  cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
  cap.set(cv::CAP_PROP_FPS, 30);

  if (!cap.isOpened()) {
    std::cerr << "Erro ao abrir a câmera idx=" << cameraIndex << "\n";
    return;
  }
  std::cout << "Câmera aberta com sucesso! Índice: " << cameraIndex << "\n";

  cv::Mat frame;
  while (isStreaming) {
    cap >> frame;
    if (frame.empty())
      continue;

    std::vector<uchar> buf;
    cv::imencode(".jpg", frame, buf);
    std::string data(buf.begin(), buf.end());
    if (sendFrameCallback)
      sendFrameCallback(data);

    std::this_thread::sleep_for(std::chrono::milliseconds(30));
  }
  cap.release();
}
