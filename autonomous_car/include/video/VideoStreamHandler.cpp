#include "VideoStreamHandler.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <vector>

VideoStreamHandler::VideoStreamHandler(int cameraIndex, const std::function<void(const std::string &)> &sendFrameCallback)
    : cameraIndex(cameraIndex), sendFrameCallback(sendFrameCallback), isStreaming(false) {}

VideoStreamHandler::~VideoStreamHandler() {
    stopStreaming();
}

void VideoStreamHandler::startStreaming() {
    if (isStreaming)
        return;

    isStreaming = true;
    streamingThread = std::thread(&VideoStreamHandler::streamLoop, this);
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

        cv::Mat frameRGB;
        cv::cvtColor(frame, frameRGB, cv::COLOR_BGR2RGB);
        cv::resize(frameRGB, frameRGB, cv::Size(EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT));

        static float features[EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT * EI_CLASSIFIER_INPUT_FRAMES];
        size_t idx = 0;
        for (int y = 0; y < EI_CLASSIFIER_INPUT_HEIGHT; y++) {
            for (int x = 0; x < EI_CLASSIFIER_INPUT_WIDTH; x++) {
                for (int c = 0; c < 3; c++) {
                    features[idx++] = frameRGB.at<cv::Vec3b>(y, x)[c] / 255.0f;
                }
            }
        }

        auto get_feature_data = [](size_t offset, size_t length, float *out_ptr) -> int8_t {
            memcpy(out_ptr, features + offset, length * sizeof(float));
            return 0;
        };

        ei::signal_t signal;
        signal.total_length = sizeof(features) / sizeof(float);
        signal.get_data = get_feature_data;

        ei_impulse_result_t result = { 0 };
        EI_IMPULSE_ERROR ei_status = run_classifier(&signal, &result, false);
        if (ei_status != EI_IMPULSE_OK) {
            std::cerr << "Erro na inferência: " << ei_status << std::endl;
            continue;
        }

        for (size_t i = 0; i < result.bounding_boxes_count; i++) {
            auto box = result.bounding_boxes[i];
            if (box.value > 0.5) {
                cv::rectangle(frame, cv::Point(box.x, box.y),
                              cv::Point(box.x + box.width, box.y + box.height),
                              cv::Scalar(0, 255, 0), 2);
                cv::putText(frame, box.label, cv::Point(box.x, box.y - 10),
                            cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 2);
            }
        }

        std::vector<unsigned char> buffer;
        cv::imencode(".jpg", frame, buffer);

        std::string frameBase64(buffer.begin(), buffer.end());
        if (sendFrameCallback) {
            sendFrameCallback(frameBase64);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }

    cap.release();
}
