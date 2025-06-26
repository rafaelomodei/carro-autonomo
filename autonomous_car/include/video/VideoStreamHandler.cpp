#include "VideoStreamHandler.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <vector>
#include <cstring>     // memcpy

/* ------------------------------------------------------------------ */
/*  Fallback: calcula o tamanho do buffer de entrada se a macro nova  */
/*  não existir na versão do SDK                                       */
#ifndef EI_CLASSIFIER_RAW_FRAME_SIZE
#define EI_CLASSIFIER_RAW_FRAME_SIZE \
    (EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT * EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME)
#endif
/* ------------------------------------------------------------------ */

VideoStreamHandler::VideoStreamHandler(
        int cameraIndex,
        const std::function<void(const std::string &)> &sendFrameCallback)
    : cameraIndex(cameraIndex),
      sendFrameCallback(sendFrameCallback),
      isStreaming(false) {}

VideoStreamHandler::~VideoStreamHandler() {
    stopStreaming();
}

void VideoStreamHandler::startStreaming() {
    if (isStreaming) return;
    isStreaming = true;
    streamingThread = std::thread(&VideoStreamHandler::streamLoop, this);
}

void VideoStreamHandler::stopStreaming() {
    if (!isStreaming) return;
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

    cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M','J','P','G'));
    cap.set(cv::CAP_PROP_FRAME_WIDTH,  640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    cap.set(cv::CAP_PROP_FPS,          30);

    if (!cap.isOpened()) {
        std::cerr << "Erro ao abrir a câmera idx=" << cameraIndex << "\n";
        return;
    }
    std::cout << "Câmera aberta com sucesso! Índice: " << cameraIndex << "\n";

    /* ------------ buffer de features ------------ */
    static float features[EI_CLASSIFIER_RAW_FRAME_SIZE];

    cv::Mat frame;
    while (isStreaming) {

        cap >> frame;
        if (frame.empty()) continue;

        /* RGB resize */
        cv::Mat frameRGB;
        cv::cvtColor(frame, frameRGB, cv::COLOR_BGR2RGB);
        cv::resize(frameRGB, frameRGB,
                   cv::Size(EI_CLASSIFIER_INPUT_WIDTH,
                            EI_CLASSIFIER_INPUT_HEIGHT));

        /* copia pixel a pixel normalizado 0-1 */
        size_t idx = 0;
        for (int y = 0; y < EI_CLASSIFIER_INPUT_HEIGHT; ++y)
            for (int x = 0; x < EI_CLASSIFIER_INPUT_WIDTH;  ++x)
                for (int c = 0; c < 3; ++c)
                    features[idx++] = frameRGB.at<cv::Vec3b>(y,x)[c] / 255.0f;

        auto get_feature_data = [](size_t off, size_t len, float *out)->int8_t{
            std::memcpy(out, features + off, len * sizeof(float));
            return 0;
        };

        ei::signal_t signal{ EI_CLASSIFIER_RAW_FRAME_SIZE, get_feature_data };
        ei_impulse_result_t result{};
        if (run_classifier(&signal, &result, /*debug=*/false) != EI_IMPULSE_OK)
            continue;

        /* desenha caixas */
        for (size_t i = 0; i < result.bounding_boxes_count; ++i) {
            const auto &box = result.bounding_boxes[i];
            if (box.value < 0.5f) continue;
            cv::rectangle(frame,
                          {box.x, box.y},
                          {box.x + box.width, box.y + box.height},
                          {0,255,0}, 2);
            cv::putText(frame, box.label,
                        {box.x, box.y - 10},
                        cv::FONT_HERSHEY_SIMPLEX, 0.5,
                        {0,255,0}, 2);
        }

        /* serializa para JPEG + Base64 bruto */
        std::vector<uchar> buf;
        cv::imencode(".jpg", frame, buf);
        std::string b64(buf.begin(), buf.end());
        if (sendFrameCallback) sendFrameCallback(b64);

        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    cap.release();
}
