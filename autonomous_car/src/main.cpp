#include "commands/CommandProcessor.h"
#include "managers/WebSocketManager.h"
#include <iostream>

int main() {
  int port = 8080;

  CommandProcessor commandProcessor;

  WebSocketManager wsManager(port);

  wsManager.setOnMessageCallback([&commandProcessor](const std::string &message) {
    commandProcessor.processCommand(message);
  });

  wsManager.start();

  return 0;
}
