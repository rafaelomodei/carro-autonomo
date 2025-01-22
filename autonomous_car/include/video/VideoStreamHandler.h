
#ifndef VIDEO_STREAM_HANDLER_H
#define VIDEO_STREAM_HANDLER_H

#include <atomic>
#include <functional>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>

class VideoStreamHandler {
public:
  VideoStreamHandler(int cameraIndex, const std::function<void(const std::string &)> &sendFrameCallback);
  ~VideoStreamHandler();

  void startStreaming();
  void stopStreaming();

private:
  void streamLoop();

  int                                      cameraIndex;
  std::function<void(const std::string &)> sendFrameCallback;
  std::atomic<bool>                        isStreaming;
  std::thread                              streamingThread;
};

#endif // VIDEO_STREAM_HANDLER_H
