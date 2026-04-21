#include "transport/VehicleWebSocketClient.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>

namespace traffic_sign_service::transport {

VehicleWebSocketClient::VehicleWebSocketClient(std::string url) : url_{std::move(url)} {}

VehicleWebSocketClient::~VehicleWebSocketClient() { stop(); }

void VehicleWebSocketClient::setMessageHandler(MessageHandler handler) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    message_handler_ = std::move(handler);
}

void VehicleWebSocketClient::setOpenHandler(OpenHandler handler) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    open_handler_ = std::move(handler);
}

void VehicleWebSocketClient::start() {
    if (running_.exchange(true)) {
        return;
    }

    worker_thread_ = std::thread(&VehicleWebSocketClient::runLoop, this);
}

void VehicleWebSocketClient::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    reconnect_cv_.notify_all();

    Client *client = nullptr;
    websocketpp::connection_hdl connection;
    bool connected = false;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        client = active_client_;
        connection = active_connection_;
        connected = connected_;
    }

    if (client) {
        websocketpp::lib::error_code ec;
        if (connected) {
            client->close(connection, websocketpp::close::status::normal, "", ec);
        }
        client->stop();
    }

    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
}

bool VehicleWebSocketClient::sendText(const std::string &payload) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (!connected_ || active_client_ == nullptr) {
        return false;
    }

    websocketpp::lib::error_code ec;
    active_client_->send(active_connection_, payload, websocketpp::frame::opcode::text, ec);
    return !ec;
}

void VehicleWebSocketClient::runLoop() {
    std::size_t reconnect_attempt = 0;

    while (running_.load()) {
        Client client;
        client.clear_access_channels(websocketpp::log::alevel::all);
        client.clear_error_channels(websocketpp::log::elevel::all);
        client.init_asio();

        std::cout << "[ws] tentando conectar em " << url_;
        if (reconnect_attempt > 0) {
            std::cout << " (tentativa " << (reconnect_attempt + 1) << ")";
        }
        std::cout << std::endl;

        client.set_open_handler([this, &client, &reconnect_attempt](websocketpp::connection_hdl hdl) {
            OpenHandler open_handler;
            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                active_client_ = &client;
                active_connection_ = hdl;
                connected_ = true;
                open_handler = open_handler_;
            }

            reconnect_attempt = 0;
            std::cout << "[ws] conectado em " << url_ << std::endl;
            if (open_handler) {
                open_handler();
            }
        });

        client.set_message_handler([this](websocketpp::connection_hdl,
                                          Client::message_ptr message) {
            MessageHandler handler;
            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                handler = message_handler_;
            }

            if (handler) {
                handler(message->get_payload());
            }
        });

        client.set_close_handler([this, &client](websocketpp::connection_hdl hdl) {
            std::string close_reason;
            websocketpp::close::status::value close_code = websocketpp::close::status::blank;
            try {
                const auto connection = client.get_con_from_hdl(hdl);
                close_reason = connection->get_remote_close_reason();
                close_code = connection->get_remote_close_code();
            } catch (const std::exception &) {
            }

            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                if (active_client_ == &client) {
                    connected_ = false;
                }
            }

            std::cout << "[ws] conexao encerrada";
            if (close_code != websocketpp::close::status::blank) {
                std::cout << " (codigo " << close_code << ")";
            }
            if (!close_reason.empty()) {
                std::cout << ": " << close_reason;
            }
            std::cout << std::endl;
        });

        client.set_fail_handler([this, &client](websocketpp::connection_hdl hdl) {
            std::string error_message = "falha desconhecida";
            try {
                const auto connection = client.get_con_from_hdl(hdl);
                error_message = connection->get_ec().message();
            } catch (const std::exception &) {
            }

            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                if (active_client_ == &client) {
                    connected_ = false;
                }
            }

            std::cerr << "[ws] falha ao conectar em " << url_ << ": " << error_message
                      << std::endl;
        });

        websocketpp::lib::error_code ec;
        const auto connection = client.get_connection(url_, ec);
        if (ec) {
            std::cerr << "Falha ao preparar conexao WebSocket: " << ec.message() << std::endl;
            waitBeforeReconnect(reconnect_attempt++);
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            active_client_ = &client;
            active_connection_ = connection->get_handle();
            connected_ = false;
        }

        client.connect(connection);

        try {
            client.run();
        } catch (const std::exception &ex) {
            std::cerr << "[ws] excecao no loop WebSocket: " << ex.what() << std::endl;
        }

        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            if (active_client_ == &client) {
                active_client_ = nullptr;
                active_connection_ = websocketpp::connection_hdl{};
                connected_ = false;
            }
        }

        if (!running_.load()) {
            break;
        }

        std::cout << "[ws] aguardando antes de reconectar..." << std::endl;
        waitBeforeReconnect(reconnect_attempt++);
    }
}

void VehicleWebSocketClient::waitBeforeReconnect(std::size_t reconnect_attempt) {
    static const std::array<std::chrono::milliseconds, 5> kReconnectBackoff = {
        std::chrono::milliseconds(500),
        std::chrono::milliseconds(1000),
        std::chrono::milliseconds(2000),
        std::chrono::milliseconds(4000),
        std::chrono::milliseconds(8000),
    };

    const auto delay = kReconnectBackoff[std::min(reconnect_attempt, kReconnectBackoff.size() - 1)];
    std::unique_lock<std::mutex> lock(reconnect_mutex_);
    reconnect_cv_.wait_for(lock, delay, [this] { return !running_.load(); });
}

} // namespace traffic_sign_service::transport
