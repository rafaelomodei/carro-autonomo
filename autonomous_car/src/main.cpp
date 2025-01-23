#include "commands/CommandProcessor.h"
#include "managers/WebSocketManager.h"
#include "video/VideoStreamHandler.h"
#include <iostream>
#include <csignal>

int main() {


  int port = 8080;

  CommandProcessor commandProcessor;
  WebSocketManager wsManager(port);

  wsManager.setOnMessageCallback([&commandProcessor](const std::string &message) {
    commandProcessor.processCommand(message);
  });

  VideoStreamHandler videoStreamHandler(4, [&wsManager](const std::string &frameData) {
    wsManager.sendFrame(frameData);
  });

  videoStreamHandler.startStreaming();
  wsManager.start();

  return 0;
}
