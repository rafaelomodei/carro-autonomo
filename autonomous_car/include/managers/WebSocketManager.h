#ifndef WEBSOCKET_MANAGER_H
#define WEBSOCKET_MANAGER_H

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class WebSocketManager {
public:
  explicit WebSocketManager(int port);
  ~WebSocketManager();

  void start();
  void setOnMessageCallback(const std::function<void(const std::string &)> &callback);

private:
  void acceptConnections();
  void handleSession(std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> ws);

  int                                      port;
  boost::asio::io_context                  ioContext;
  boost::asio::ip::tcp::acceptor           acceptor;
  std::function<void(const std::string &)> onMessageCallback;
};

#endif // WEBSOCKET_MANAGER_H
