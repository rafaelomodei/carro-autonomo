#include "WebSocketManager.h"
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <memory>
#include <rapidjson/document.h>
#include <thread>

WebSocketManager::WebSocketManager(int port)
    : port(port),
      acceptor(ioContext, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)) {}

WebSocketManager::~WebSocketManager() {
  ioContext.stop();
}

void WebSocketManager::start() {
  std::cout << "Iniciando o servidor WebSocket na porta " << port << "..." << std::endl;
  acceptConnections();
  ioContext.run();
}

void WebSocketManager::setOnMessageCallback(const std::function<void(const std::string &)> &callback) {
  onMessageCallback = callback;
}

void WebSocketManager::acceptConnections() {
  acceptor.async_accept([this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
    if (!ec) {
      std::cout << "Nova conexão aceita!" << std::endl;

      auto ws = std::make_shared<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>>(std::move(socket));
      handleSession(ws);
    } else {
      std::cerr << "Erro ao aceitar conexão: " << ec.message() << std::endl;
    }

    // Continuar aceitando conexões
    acceptConnections();
  });
}

void WebSocketManager::sendFrame(const std::string &frameData) {
    for (auto it = activeSessions.begin(); it != activeSessions.end();) {
        auto &session = *it;
        if (session->is_open()) {
            try {
                session->text(true);
                session->write(boost::asio::buffer(frameData));
                ++it; // Avançar para a próxima sessão apenas se não houver erros
            } catch (const std::exception &e) {
                std::cerr << "Erro ao enviar frame: " << e.what() << std::endl;
                it = activeSessions.erase(it); // Remove a sessão com erro
            }
        } else {
            it = activeSessions.erase(it); // Remove a sessão fechada
        }
    }
}

void WebSocketManager::handleSession(std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> ws) {
    try {
        ws->accept();
        activeSessions.push_back(ws); // Adicionar sessão apenas após a aceitação

        std::thread([ws, this]() {
            try {
                for (;;) {
                    boost::beast::flat_buffer buffer;
                    ws->read(buffer);

                    std::string message = boost::beast::buffers_to_string(buffer.data());
                    if (onMessageCallback) {
                        onMessageCallback(message);
                    }
                }
            } catch (const boost::beast::system_error &se) {
                // Tratamento específico para erros do Boost.Beast
                if (se.code() != boost::beast::websocket::error::closed) {
                    std::cerr << "Erro na sessão: " << se.what() << std::endl;
                }
            } catch (const std::exception &e) {
                std::cerr << "Erro na sessão: " << e.what() << std::endl;
            }

            // Remover sessão ao sair
            auto it = std::find(activeSessions.begin(), activeSessions.end(), ws);
            if (it != activeSessions.end()) {
                activeSessions.erase(it);
            }
        }).detach();
    } catch (const std::exception &e) {
        std::cerr << "Erro ao aceitar sessão: " << e.what() << std::endl;
    }
}

